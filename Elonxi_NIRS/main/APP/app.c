/**
 ****************************************************************************************************
* @file        app.c
* @author      zz
* @version     V1.0
* @date        2024-09-12
* @brief       WIFI UDP
* @license     ELONXI
****************************************************************************************************
*/


/*----------------------------------------------------------------------
 Include										
-----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <esp_sleep.h>
#include "esp_pm.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_slave.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "lwip/err.h"   
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "esp_task_wdt.h"

#include "esp_mac.h"
#include "app.h"
#include "mdns.h"
#include "esp_err.h"
#include "led.h"
#include "NIRS.h"
#include "adc1.h"

#include "wifi_data.h"
#include "cmt2300_rx_tx.h"
#include "filters.h"
#include "i2c_mpu9250.h"
#include "sdmmc.h"
#include "wifi_udp_app.h"
#include "wifi_ota.h"
#include "app_data.h"


/*---------------------------------------------------------------------
Macros       				  				                                                                                                                                                                                                                                                                                                                                    
-----------------------------------------------------------------------*/
#if defined(_MDNS_)
#define DEFAULT_SSID          "ELONXI-WiFi-Test"     /* 链接wifi名称 */ 
#define DEFAULT_PWD           "012345678"       /* wifi密码 */ 
#define DEFAULT_IP_ADDR       "192.168.1.101" /* 默认PC IP */ //"192.168.218.10"  192.168.1.100
#define DEV_PORT               12341           /* 连接的本地端口号 */ //22341
#else
#define DEFAULT_SSID          "UTI_0219"    //"UTI_0219 UTI_0264 "//"ELONXI-WiFi" /* 链接wifi名称 */ //UTI_02AC
#define DEFAULT_PWD           "12345678"      //"12345678"//"012345678"   /* wifi密码 */ //12345678
#define DEFAULT_IP_ADDR       "192.168.218.1" /* 默认PC IP */ //"192.168.218.1"
#define DEV_PORT               12341           /* 连接的本地端口号 *///12341
#endif

#define LWIP_DEMO_RX_BUFSIZE   100    /* 最大接收数据长度 */
#define LWIP_SEND_THREAD_PRIO   ( tskIDLE_PRIORITY + 7 ) /* 发送数据线程优先级 */


/*---------------------------------------------------------------------
Global variables         				  				
-----------------------------------------------------------------------*/
APP_GLOBAL g_app_var;

esp_ip4_addr_t esp_ip_addr;              //esp IP地址，连接至WiFi后获取

volatile bool isPCConnected = false;     //是否连接至上位机
struct sockaddr_in dest_addr;            //主机（PC）ip地址
int g_sock_fd;                           //udp socket
struct sockaddr_in g_local_info;   

static const char *TAG = "APP";
//static int8_t g_rawdata[6] = {0x00};
                   
static void app_send_data_task(void *arg);
static void app_send_rf_task(void *pvParameters);

extern uint8_t CMT2300A_ReadGpio1(void);
extern uint8_t CMT2300A_ReadGpio2(void);

/*---------------------------------------------------------------------
Extern         				  				
-----------------------------------------------------------------------*/
//extern GLOBAL_PARA g_struct_para;

extern void mpu9250_init(void);

void app_read_sd_data_to_app_multi(uint32_t packcnt);
void app_read_sd_data_to_app_imu_multi(uint32_t packcnt);
/*****************************************************************************
  * Function:	  
  * 		 calculateXOR
  * Description: 
  * 		 xor calibration
  * Parameters:  
  * 		 [data,length]
  * Return: 	 
  * 		 xor value
*****************************************************************************/
uint8_t calculateXOR(uint8_t* data, uint16_t length) 
{
    uint8_t xorValue = 0;
    for(uint16_t i = 0; i < length; i++) 
    {
        xorValue ^= data[i];
    }
    return xorValue;
}


/*****************************************************************************
  * Function:	  
  * 		 udp_send_data
  * Description: 
  * 		 send data for udp
  * Parameters:  
  * 		 [data,len]
  * Return: 	 
  * 		 void
*****************************************************************************/

void udp_send_data(const uint8_t* data, size_t len) 
{
    if(!g_app_var.isPC_connected)
    {
        #if defined(_MDNS_)
        //return;
        #endif
    }

    int err = sendto(g_sock_fd, data, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) 
    {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", err);
    } 
}


/*****************************************************************************
  * Function:	  
  * 		 app_common_responsive
  * Description: 
  * 		 wifi responsive pc cmd
  *          A0 01 B0 00 01 00 10 0D 0A
  * Parameters:  
  * 		 [cmd]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_common_responsive(uint8_t sn, uint8_t data_type, uint8_t cmd)
{
    uint8_t send_data[9] = {0};
    uint8_t xor_val = 0;
    static uint8_t sn_num = 0;

    send_data[0] = PACKET_TX_HEADER;
    send_data[1] = DEVICE_MINI_NIRS;
    send_data[2] = data_type;

    if(data_type == TYPE_ERROR_UPLOAD)
    {
        send_data[3] = sn_num;
        if(0xFF > sn_num)
        {
            sn_num++;
        }
        else
        {
            sn_num = 0;
        }   
    }
    else
    {
        send_data[3] = sn;
    }

    send_data[4] = 0x01;//data size
    send_data[5] = cmd;

    xor_val = calculateXOR(send_data,6);
    
    send_data[6] = xor_val;
    send_data[7] = PACKET_TAIL_1;
    send_data[8] = PACKET_TAIL_2;

    udp_send_data(send_data,9);  
}


/*****************************************************************************
  * Function:	  
  * 		 app_wifi_status
  * Description: 
  * 		 download wifi status for pc 
  * Parameters:  
  * 		 [cmd,local_addr]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_wifi_status(uint8_t cmd,in_addr_t local_addr)
{
    dest_addr.sin_addr.s_addr = local_addr;

    if(!cmd)
    {
        g_app_var.isPC_connected = false; 
        led_set('B',PIN_RESET);
    }
    else
    {
        g_app_var.isPC_connected = true; 
        //led_set('G',PIN_RESET);
    }
}


/*****************************************************************************
  * Function:	  
  * 		 app_para_config
  * Description: 
  * 		 download config para for pc
  * Parameters:  
  * 		 [cmd]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_para_config(uint8_t cmd)
{
    g_app_var.para_config = cmd;

    // g_app_var.isEMG_channel_1 = GET_BIT(g_app_var.para_config, 0);
    // g_app_var.isEMG_channel_2 = GET_BIT(g_app_var.para_config, 1);
    g_app_var.isIMU = GET_BIT(g_app_var.para_config, 2);
    g_app_var.is_NIRS = GET_BIT(g_app_var.para_config, 5);

    ESP_LOGE(TAG, "[nirs=%02X imu=%02X] ", g_app_var.is_NIRS,g_app_var.isIMU);
}


/*****************************************************************************
  * Function:	  
  * 		 app_receive_task
  * Description: 
  * 		 WiFi 接收从PC发来的指令/数据
  * Parameters:  
  * 		 [pvParameters]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_receive_task(void *pvParameters) 
{  
    uint8_t rx_buffer[LWIP_DEMO_RX_BUFSIZE];    //WiFi接收数据
    int len = 0;

#if defined(_MDNS_)
    g_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(g_sock_fd < 0) 
    {
        // Error handling
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    // 设置套接字为非阻塞模式
    int opt = 1;
    if(ioctl(g_sock_fd, FIONBIO, &opt) < 0) 
    {
        // Error handling
        ESP_LOGE(TAG, "Unable to set socket FIONBIO: errno %d", errno);
        return;
    }

    struct sockaddr_in local_addr;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);//htonl(INADDR_ANY);
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(DEV_PORT);
    socklen_t socklen = sizeof(local_addr);
    bind(g_sock_fd, (struct sockaddr *)&local_addr, socklen);
    //printf("app_receive_task 111222\r\n");
    g_app_var.isPC_connected = true;

    //dest_addr.sin_addr.s_addr = local_addr.sin_addr.s_addr;
 #endif
    socklen_t addr_len = sizeof(dest_addr);
    while (1) 
    {
        //if(g_app_var.isRF)
        {
            len = recvfrom(g_sock_fd, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&dest_addr, &addr_len);
            //sendto(g_sock_fd, data, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            printf("len = %d\r\n",len);
            
            printf("udp rx:");
            for(int i = 0; i < len; i++)
            {
                printf("%02X ",rx_buffer[i]);
            }
            printf("\r\n");
        
            //printf("app_receive_task 22222\r\n");
            if(0 < len)
            {
                if(RF_APP_HEAD == rx_buffer[0]) //APP to device
                {
                    uint8_t cmd = rx_buffer[2];
                    uint8_t tmp_count = 0;
                    uint32_t pack_cnt = 0;
                    uint8_t imu_tmp_count = 0;
                    uint32_t imu_pack_cnt = 0;

                    switch(cmd)
                    {
                        case RF_REPACK_MULTI:
                            // if(memcmp(&rx_buffer[11],g_app_var.serialNumber, 8) == 0)
                            // {
                                tmp_count = rx_buffer[19];
                                for(int i = 0; i < tmp_count; i++) 
                                {
                                    uint8_t b0 = rx_buffer[20+4*i];
                                    uint8_t b1 = rx_buffer[21+4*i];
                                    uint8_t b2 = rx_buffer[22+4*i];
                                    uint8_t b3 = rx_buffer[23+4*i];

                                    pack_cnt = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;

                                    vTaskDelay(1);
                                    app_read_sd_data_to_app_multi(pack_cnt);
                                    ESP_LOGI("UDP RX", "Multi-1 pack_cnt=%ld %d", pack_cnt, tmp_count);
                                }

                                ESP_LOGI("UDP RX", "Supplemental Data Pack Multi");
                            // }
                            break;

                        case RF_REPACK_IMU_MULTI:
                            if(memcmp(&rx_buffer[11],g_app_var.serialNumber, 8) == 0)
                            {
                                imu_tmp_count = rx_buffer[19];
                                for(int i = 0; i < imu_tmp_count; i++) 
                                {
                                    uint8_t b0 = rx_buffer[20+4*i];
                                    uint8_t b1 = rx_buffer[21+4*i];
                                    uint8_t b2 = rx_buffer[22+4*i];
                                    uint8_t b3 = rx_buffer[23+4*i];

                                    imu_pack_cnt = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
                                    
                                    vTaskDelay(2);
                                    app_read_sd_data_to_app_imu_multi(imu_pack_cnt);
                                    //ESP_LOGI("UDP RX", "imu Multi-1 pack_cnt=%ld %d", imu_pack_cnt, imu_tmp_count);
                                }
                                ESP_LOGI("UDP RX", "Supplemental Data Pack IMU Multi");
                            }
                            break;

                        default :
                            break;
                    }
                }
            }
        }
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    free(rx_buffer);
    vTaskDelete(NULL);
}


/*****************************************************************************
  * Function:	  
  * 		 app_nvs_init
  * Description: 
  * 		 nvs flash init
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_nvs_init(void) 
{
    esp_err_t err = nvs_flash_init();
    ESP_LOGI(TAG, "nvs flash init done %d.", err);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_LOGI(TAG, "COME HERE.");
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}


/*****************************************************************************
  * Function:	  
  * 		 app_wifi_event_handler
  * Description: 
  * 		 wifi event handler
  * Parameters:  
  * 		 [arg,event_base,event_id,event_data]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    }

    if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) //ESP连接至wifi
    { 
        g_app_var.isWiFi_connected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("ESP32_WiFi", "Got IP: " IPSTR,  IP2STR(&event->ip_info.ip));
        ESP_LOGI("ESP32_WiFi", "Got GW: " IPSTR,  IP2STR(&event->ip_info.gw));
        esp_ip_addr = event->ip_info.ip;
        led_set('B',PIN_RESET);
    }

    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        g_app_var.isWiFi_connected = false;
        led_set('R',PIN_RESET);
        g_app_var.isRF = 0;
        g_app_var.synFlag = 0;
        ESP_LOGI("ESP32_WiFi", "WiFi Disconnected.");
    }
}


/*****************************************************************************
  * Function:	  
  * 		 lwip_wifi_start_mdns_service
  * Description: 
  * 		 mdns start service
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_wifi_start_mdns_service(void)
{
    // 初始化mDNS服务
    ESP_ERROR_CHECK( mdns_init() );

    // 设置mDNS主机名（例如 "esp32"）和实例名
    ESP_ERROR_CHECK( mdns_hostname_set("esp32") );
    ESP_ERROR_CHECK( mdns_instance_name_set("ESP32 MINI-NIRS") );

    // 添加一个mDNS服务
    // mdns_service_add : 根据服务类型、协议和端口添加服务，这里是添加一个HTTP服务
    // _http : 服务类型
    // _tcp : 服务协议
    // 80 : 服务端口
    ESP_ERROR_CHECK( mdns_service_add("MINI-NIRS", "_http", "_udp", DEV_PORT, NULL, 0) );

    // 可选：添加服务文本记录（key=value形式），用于提供额外信息
    mdns_txt_item_t serviceTxtData[] = 
    {
        {"board", "esp32"},
        {"u", "user"},
        {"p", "pass"}
    };
    ESP_ERROR_CHECK( mdns_service_txt_set("_http", "_udp", serviceTxtData, sizeof(serviceTxtData) / sizeof(serviceTxtData[0])) );
}


/*****************************************************************************
  * Function:	  
  * 		 app_wifi_and_mdns_conf
  * Description: 
  * 		 mdns and wifi configuration initialization
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_wifi_and_mdns_conf(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_sta_config_t cfg_sta = 
    {
        .ssid = DEFAULT_SSID,
        .password = DEFAULT_PWD,
    };
    
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *) &cfg_sta);
    esp_wifi_set_mode(WIFI_MODE_STA);

    dest_addr.sin_addr.s_addr = inet_addr(DEFAULT_IP_ADDR); //默认PC IP   192.168.31.251
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEV_PORT);   //默认PC端口

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, app_wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, app_wifi_event_handler, NULL, NULL);

    esp_wifi_start();
    //app_wifi_start_mdns_service();

#if 1//defined(_MDNS_)
    g_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(g_sock_fd < 0) 
    {
        // Error handling
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
#endif
}


/*****************************************************************************
  * Function:	  
  * 		 app_check_battery_level
  * Description: 
  * 		 app check battery level
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_check_battery_level(void)
{
    static uint16_t count = 0;
    static uint8_t count_low = 0;
//    static uint8_t count_send = 0;

    count++;
    if(600 <= count) //1min
    {
        count = 0;
        if(0 == app_get_battery_level())
        {
            g_app_var.isBat_low = true;
            sendToUpAppInfo();     
        }

        // count_send++;
        // if(5 < count_send) //5min
        // {
        //     count_send = 0;
        //     //sendToUpAppInfo();
        // }
    }  
    
    if(g_app_var.isBat_low)
    {
        if(30 < count_low)
        {
            count_low = 0;
             //esp_restart();

            // 进入深度睡眠模式
            //esp_deep_sleep_start();
            POWER_OFF;
        }
        else
        {
            count_low++;
            printf("count_low = %d\r\n",count_low);
        }
        LED_B(1);
        LED_G(1);
        LED_R_TOGGLE();
    }
}


/*****************************************************************************
  * Function:	  
  * 		 app_send_nirs_data
  * Description: 
  * 		 send nirs data
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_send_nirs_data(void)
{
    static uint16_t counter = 0;
    //static uint8_t tmp_counter = 0;

    if(nirs_get_ready_flag())   /*NIRS数据准备好*/
    {
        nirs_set_ready_flag(0);
        //ESP_LOGI(TAG, "send nirs data.");
        /*大小端转换*/
        int i = counter * NIRS_BYTES;
        G_nirs_data.SendData[i++] = (G_nirs_data.RawData[0]>>8)&0xff;
        G_nirs_data.SendData[i++] = (G_nirs_data.RawData[0]>>0)&0xff;
        G_nirs_data.SendData[i++] = (G_nirs_data.RawData[1]>>8)&0xff;
        G_nirs_data.SendData[i++] = (G_nirs_data.RawData[1]>>0)&0xff;
        
        for(int j = 0; j<3;j++){
            G_nirs_data.SendData[i++] = BYTE3(G_nirs_data.ConcData[j]);
            G_nirs_data.SendData[i++] = BYTE2(G_nirs_data.ConcData[j]);
            G_nirs_data.SendData[i++] = BYTE1(G_nirs_data.ConcData[j]);
            G_nirs_data.SendData[i++] = BYTE0(G_nirs_data.ConcData[j]);
        }
        counter++;
        if(g_app_var.nirs_dr_pack == counter)   //100ms
        {
            g_app_var.sendFlag = 1;
            counter = 0;
        }
    }
}


/*****************************************************************************
  * Function:	  
  * 		 app_send_udp_nirs_data
  * Description: 
  * 		 interval send nirs data for udp
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_send_udp_nirs_data(void)
{

    if(g_app_var.sendFlag)
    {
        g_app_var.sendFlag = 0;
        udpSendSensorData(TYPE_DATA);
        LED_G_TOGGLE();
    }

    if(g_app_var.sd_write_flag)
    {
        g_app_var.sd_write_flag = 0;
        app_sdmmc_write_sectors(G_nirs_data.sd_buffer,g_app_var.sd_ready_packcnt*SD_SECTOR_MUL,SD_SECTOR_MUL*NIRS_SD_INT); 
    }
}

int repackSendData(uint8_t *data, uint16_t length, uint32_t stamp)
{
    uint16_t i=0;
    uint16_t crc=0;
    //head
    data[i++] = 0x5A; 

    //length
    data[i++] = 0x0;
    data[i++] = 0x0;

    //cmd
    data[i++] = DEVICE_TYPE_ID +1; 

    //mark
    data[i++] = 0;//(sDb.seq>>8)&0xFF;
    data[i++] = 0x01;//sDb.seq&0xFF;

    //sn
    memcpy(data+i, g_app_var.serialNumber, 8);
    i += 8;

    data[i++] = (stamp >> 24)&0xFF;
	data[i++] = (stamp >> 16)&0xFF;
	data[i++] = (stamp >> 8)&0xFF;
	data[i++] = stamp&0xFF;

    //interval
    data[i++] = 0x01; //ms <= 200ms

    //data
    i += length;

    //printf("packet data= %d %d %d\r\n",data[2418],data[2419], data[2420]);

    //modify length
    data[1] = (i>> 8)&0xFF;
    data[2] = i&0xFF;

    //crc
    crc = CRC16(data, i);
    data[i++] = (crc >> 8)&0xFF;
	data[i++] = crc&0xFF;

    //printf("packet ii= %d\r\n",i);
    //send data
    return i;	

}

/****************************************************************************************
  * Function:	  
  * 		 app_read_sd_data_to_app
  * Description: 
  * 		 read sd data to app for wifi
  *          A5 17 F0 6D 61 72 6B 30 30 30 32 65 30 30 62 34 33 38 63 00 00 00 20 14 7D
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************************/
void app_read_sd_data_to_app(void)
{
    uint32_t packcnt = 0;
    uint32_t sd_packcnt = 0;
    int ret = 0; 
    //uint8_t buffer[2500] = {0}; //1024
    int len = 0; //155-100  75-50 59-25 43-12.5
    int index = app_get_rf_nirs_dr();
    switch(index){
        case 0: len=181;break;
        case 1: len=101;break;
        case 2: len= 53;break;
        case 3: len= 37;break;
        default:len=181;break;
    }

    memset(g_app_var.sd_read_buffer, 0x00, sizeof(g_app_var.sd_read_buffer)); 

    packcnt = (cmt2300_recev_buff[19] << 24) | (cmt2300_recev_buff[20] << 16) | (cmt2300_recev_buff[21] << 8) | (cmt2300_recev_buff[22]);
    if(packcnt>g_app_var.packet_counter){
        ESP_LOGE("to app", "packet = %ld has no data in sdmmc", packcnt);
        repackSendData(g_app_var.sd_read_buffer, len, packcnt);
        ret = ESP_OK;
    }
    else{
        ret = app_sdmmc_read_sectors_safe(g_app_var.sd_read_buffer,packcnt);
        sd_packcnt = (g_app_var.sd_read_buffer[14] << 24) | (g_app_var.sd_read_buffer[15] << 16) | (g_app_var.sd_read_buffer[16] << 8) | (g_app_var.sd_read_buffer[17]);
    }

    ESP_LOGI("TO APP", "packcnt= %ld, get_packcnt = %ld, ret=%d", packcnt, sd_packcnt, ret);

    if(ret != ESP_OK)
    {
        led_set('R',PIN_RESET);
        return;
    }
    else
    {
        led_set('R',PIN_RESET);
        LED_B_TOGGLE();
        udpUpAppSendData(g_app_var.sd_read_buffer, len);
        ESP_LOGI("REPACK DATA", "PACKCNT=%ld", packcnt);
    }

}

/*****************************************************************************
  * Function:	  
  * 		 app_get_nirs_dr
  * Description: 
  * 		 get nirs dr 
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_get_nirs_dr(void)
{
    // app_get_rf_nirs_dr_index_nvs();
    
    if(0 == g_app_var.rf_nirs_dr_index)
    {
        g_app_var.nirs_dr = SR_100;
        nirs_set_sprate(10/SAMPLE_RATE_NIRS_100);
        g_app_var.nirs_dr_pack = SAMPLE_RATE_NIRS_100;
    }
    else if(1 == g_app_var.rf_nirs_dr_index)
    {
        g_app_var.nirs_dr = SR_50;
        nirs_set_sprate(10/SAMPLE_RATE_NIRS_50);
        g_app_var.nirs_dr_pack = SAMPLE_RATE_NIRS_50;
    }
    else if(2 == g_app_var.rf_nirs_dr_index)
    {
        g_app_var.nirs_dr = SR_20;
        nirs_set_sprate(10/SAMPLE_RATE_NIRS_20);
        g_app_var.nirs_dr_pack = SAMPLE_RATE_NIRS_20;
    }
    else if(3 == g_app_var.rf_nirs_dr_index)
    {
        g_app_var.nirs_dr = SR_10;
        nirs_set_sprate(10/SAMPLE_RATE_NIRS_10);
        g_app_var.nirs_dr_pack = SAMPLE_RATE_NIRS_10;
    }
    else
    {
        g_app_var.nirs_dr = SR_50;
        nirs_set_sprate(10/SAMPLE_RATE_NIRS_50);
        g_app_var.nirs_dr_pack = SAMPLE_RATE_NIRS_50;
    }
    
    ESP_LOGI(TAG, "set nirs sample rate index: %d. sample rate: %d.", (int)g_app_var.rf_nirs_dr_index, nirs_get_sprate());
}

/****************************************************************************************
  * Function:	  
  * 		 app_read_sd_data_to_app_multi
  * Description: 
  * 		 read sd data to app for wifi
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************************/
void app_read_sd_data_to_app_multi(uint32_t packcnt)
{
    static uint8_t led_set_first = 1;
    int ret = 0; 
    int len = 0;
    switch(app_get_rf_nirs_dr()){
        case 0: len = 181; break;
        case 1: len = 101; break;
        case 2: len = 53; break;
        case 3: len = 37; break;
        default: len = 101;break;
    }

    memset(g_app_var.sd_read_buffer, 0x00, sizeof(g_app_var.sd_read_buffer)); 

    //packcnt = (cmt2300_recev_buff[19] << 24) | (cmt2300_recev_buff[20] << 16) | (cmt2300_recev_buff[21] << 8) | (cmt2300_recev_buff[22]);
    ret = app_sdmmc_read_sectors_safe(g_app_var.sd_read_buffer,packcnt);

    if(ret != ESP_OK)
    {
        led_set('R',PIN_RESET);
        return;
    }
    else
    {
        if(led_set_first)
        {
            led_set_first = 0;
            led_set('W',PIN_SET);
        }
        //ESP_LOGI("SEND PACK", "[%02X %02X %02X %02X]",g_app_var.sd_read_buffer[0],g_app_var.sd_read_buffer[1],g_app_var.sd_read_buffer[2],g_app_var.sd_read_buffer[3]);
        udpUpAppSendData(g_app_var.sd_read_buffer, len);
    }
}


/****************************************************************************************
  * Function:	  
  * 		 app_read_sd_data_to_app_multi
  * Description: 
  * 		 read sd data to app for wifi
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************************/
void app_read_sd_data_to_app_imu_multi(uint32_t packcnt)
{
    static uint8_t led_set_first = 1;
    int ret = 0; 
    int len = 221; //

    memset(g_app_var.imu_sd_read_buffer, 0x00, sizeof(g_app_var.imu_sd_read_buffer)); 

    ret = app_sdmmc_read_sectors_imu(g_app_var.imu_sd_read_buffer,packcnt);

    if(ret != ESP_OK)
    {
        led_set('R',PIN_RESET);
        return;
    }
    else
    {
        if(led_set_first)
        {
            led_set_first = 0;
            led_set('W',PIN_SET);
        }
        
        udpUpAppSendData(g_app_var.imu_sd_read_buffer, len);
    }
}


/*****************************************************************************
  * Function:	  
  * 		 app_set_nirs_dr
  * Description: 
  * 		 set nirs dr 
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_set_nirs_dr(void)
{
    g_app_var.rf_nirs_dr_index = (int32_t)cmt2300_recev_buff[19];
    // app_set_rf_nirs_dr_index_nvs();  
    //app_get_nirs_dr();
    led_set('R', PIN_RESET);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    led_set('B', PIN_RESET);
    // esp_restart();
}


/*****************************************************************************
  * Function:	  
  * 		 app_receive_rf_data
  * Description: 
  * 		 receive Router RF data
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_receive_rf_data(void)
{
    uint8_t  ret , cmt_rx_len ;
    // uint8_t buffer[5] = {0x5A,0x01,0x10,0x0D,0x0A};

  #if 0
    uint8_t buffer[5] = {0x5A,0x01,0x10,0x0D,0x0A};
    ret = CMT2300_Send_Buff(buffer,5);
    ESP_LOGI("CMT2300A", "send ret = %d",ret);

  #else 
    //ESP_LOGI("CMT2300A", "GPIO1=%d GPIO2=%d",CMT2300A_ReadGpio1(),CMT2300A_ReadGpio2());
    //ESP_LOGI("CMT2300A", "CMT2300A revice len=%d %d" ,cmt_rx_len,ret);

    //if(!g_app_var.isRF)
    {
        ret = CMT2300_Rece_buff(&cmt_rx_len); 
        if(ret == 1)
        {
            if(RF_HEAD == cmt2300_recev_buff[0]) //Router to device
            {
                if(0x10 == cmt2300_recev_buff[2]) //5A 0F 10 6D 61 72 6B 30 30 30 32 00 00 18 41 CA C1
                {
                    //g_app_var.isRF = 1;
                    //g_app_var.synFlag = 1;
                    //led_set('G',PIN_RESET);
                    g_app_var.rf_syn_packcnt = (cmt2300_recev_buff[11] << 24) | (cmt2300_recev_buff[12] << 16) | (cmt2300_recev_buff[13] << 8) | (cmt2300_recev_buff[14]);
                    ESP_LOGI("CMT2300A", "SYN cnt = %ld",g_app_var.rf_syn_packcnt);
                }
            }
            
            if(RF_APP_HEAD == cmt2300_recev_buff[0]) //APP to device
            {
                uint8_t cmd = cmt2300_recev_buff[2];
                switch(cmd)
                {
                    case RF_START:
                                    g_app_var.isRF = 1;
                                    g_app_var.synFlag = 1;
                                    //g_app_var.packet_counter = 0;
                                    led_set('G',PIN_RESET);
                                    ESP_LOGI("CMT2300A", "APP isRF = %d",g_app_var.isRF);
                                    NIRS_Start();
                                    break;
                    case RF_STOP:
                                    g_app_var.isRF = 0;
                                    g_app_var.synFlag = 0;
                                    //g_app_var.packet_counter = 0;
                                    led_set('G',PIN_SET);
                                    led_set('B',PIN_RESET);
                                    ESP_LOGI("CMT2300A", "APP isRF = %d",g_app_var.isRF);
                                    NIRS_Stop();
                                    break;
                    case RF_REPACK: 
                                    // if(memcmp(cmt2300_recev_buff+13,g_app_var.serialNumber, 8)==0){
                                        //判断传感器id与指令id是否相同
                                        app_read_sd_data_to_app();
                                        ESP_LOGI("CMT2300A", "RF Supplemental Data Pack");
                                    // }
                                    break;   
                    case RF_INFO:   
                                    ESP_LOGI("CMT2300A", "RF INFO");
                                    sendToUpAppInfo();
                                    break; 
                    case RF_DR:
                                    ESP_LOGI("CMT2300A", "RF DR");
                                    
                                    app_set_nirs_dr();
                                    break; 
                    case RF_OTA:
                                    ESP_LOGI("CMT2300A", "RF OTA");
                                    
                                    ESP_LOGI("OTA","Waiting update");
                                    vTaskDelay(1000);
                                    xTaskCreate(wifi_ota_task, "wifi_ota_task", 8192, NULL, 2, NULL);
                                    break;                            
                    case RF_IMU_START:    
                                    SetIMU(1);
                                    ESP_LOGI("CMT2300A", "RF IMU = 1");
                                    break; 
                    case RF_IMU_STOP:    
                                   SetIMU(0);
                                    ESP_LOGI("CMT2300A", "RF IMU = 0");
                                    break;        
                    case RF_POWEROFF: 
                                    POWER_OFF;
                                    break;

                    default : break;
                }
            }

        }
        else
        {

        }
    }
   #endif
}



/*****************************************************************************
  * Function:	  
  * 		 app_send_rf_task
  * Description: 
  * 		 rf task
  * Parameters:  
  * 		 [pvParameters]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_send_rf_task(void *pvParameters)
{
    pvParameters = pvParameters;

    while(1)
    {
        app_receive_rf_data();
        vTaskDelay(1);
    }

}


/*****************************************************************************
  * Function:	  
  * 		 app_send_data_task
  * Description: 
  * 		 start send data task
  * Parameters:  
  * 		 [pvParameters]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_send_data_task(void *pvParameters)
{
    pvParameters = pvParameters;

    
    //NIRS_Start();
    while (1)
    {    
        if(g_app_var.isRF)
        {
            if(g_app_var.synFlag)
            {
                udpSendSensorData(TYPE_MSG);
                g_app_var.synFlag = 0;  
            }
            else
            {
                app_send_nirs_data();
            }
            app_send_udp_nirs_data();
        }
        else
        {
            vTaskDelay(1/portTICK_PERIOD_MS);
            //printf(" send data rf close;%d.\r\n", g_app_var.isRF);
            if(!g_app_var.isWiFi_connected)
            {
                led_set('R',PIN_RESET);
            }
        } 
   }
   vTaskDelete(NULL);	
}


/*****************************************************************************
  * Function:	  
  * 		 nirs_task_handler
  * Description: 
  * 		 start nirs handler task
  * Parameters:  
  * 		 [pvParameters]
  * Return: 	 
  * 		 void
*****************************************************************************/
void nirs_task_handler(void *pvParameters){
    pvParameters = pvParameters;    
    int received_data;
    while(1){
        if(xQueueReceive(xTimerQueue, &received_data, 10/portTICK_PERIOD_MS) == pdTRUE){
            NIRS_Handler();
        }
        else{
            vTaskDelay(1/portTICK_PERIOD_MS);
        }
    }
}


void print_rssi(void) 
{
    //int8_t rssi = 0;
    wifi_ap_record_t ap_info;
    if(g_app_var.isWiFi_connected){
        if(esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){
            //ESP_LOGI(TAG, "RSSI : %d dBm", ap_info.rssi);
            g_app_var.abs_rssi = abs(ap_info.rssi);
        } 
    }else{
        ESP_LOGE(TAG, "无法获取 WiFi AP 信息");
    }
    
}



/*****************************************************************************
  * Function:	  
  * 		 app_common_task
  * Description: 
  * 		 start common task
  * Parameters:  
  * 		 [pvParameters]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_common_task(void *pvParameters)
{
    //static uint8_t count= 0;
    static uint8_t first_flag= 0;
    pvParameters = pvParameters; 

    //vTaskDelay(5000);
    
    while(1)
    {
        createUdpUpAppTask();
        vTaskDelay(1000/portTICK_PERIOD_MS);
        app_check_battery_level();
        print_rssi();
        //sendToUpAppInfo();
        //ESP_LOGI(TAG, "COME HERE.");
        if(app_power_key_onoff()){
            NIRS_Stop();
        }

        if(!first_flag)
        {
            first_flag = 1;
            if(0 == app_get_battery_level())
            {
                g_app_var.isBat_low = true; 
                led_set('R',PIN_RESET); 
                POWER_OFF;
            }

            //sendToUpAppInfo();
        }
    }
}
/*****************************************************************************
  * Function:	  
  * 		 app_imu_send_task
  * Description: 
  * 		 send imu data task
  * Parameters:  
  * 		 [pvParameters]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_imu_send_task(void)
{
    sendToUpAppImu();
}

/*****************************************************************************
  * Function:	  
  * 		 app_imu_task
  * Description: 
  * 		 recv imu data task
  * Parameters:  
  * 		 [pvParameters]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_imu_task(void *pvParameters)
{
    pvParameters = pvParameters; 
    vTaskDelay(5000/portTICK_PERIOD_MS);
    app_mpu9250_task();
}


/*****************************************************************************
  * Function:	  
  * 		 app_para_init
  * Description: 
  * 		 parameters init
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_get_esp_mac(void)
{
    esp_read_mac(g_app_var.mac, ESP_MAC_WIFI_STA);
    sprintf((char*)g_app_var.serialNumber, "%02x%02x%02x%02x", g_app_var.mac[2], g_app_var.mac[3], g_app_var.mac[4], g_app_var.mac[5]);
    printf("mac:[%02X %02X %02X %02X %02X %02X]\r\n",g_app_var.mac[0],g_app_var.mac[1],g_app_var.mac[2],g_app_var.mac[3],g_app_var.mac[4],g_app_var.mac[5]);
    //printf("mac_str:%s\r\n",g_app_var.serialNumber);
}

void vTaskExample(void) {
    TickType_t startTick, endTick;
    const TickType_t delay = pdMS_TO_TICKS(1000); // 延时 1000ms

    // 获取当前系统时钟节拍数
    startTick = xTaskGetTickCount();
    // 延时 1 秒
    vTaskDelay(delay);
    // 再次获取系统时钟节拍数
    endTick = xTaskGetTickCount();
    // 打印时间差
    printf("Elapsed ticks: %lu\n", endTick - startTick);
}

/*****************************************************************************
  * Function:	  
  * 		 app_para_init
  * Description: 
  * 		 parameters init
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_para_init(void)
{
#if defined(_MDNS_)
    g_app_var.isRF = 1;
    g_app_var.synFlag = 0;
#else
    g_app_var.isRF = 0;
#endif
    g_app_var.rf_nirs_dr_index = 1;
    app_get_nirs_dr();

    g_app_var.isWiFi_connected = false;
    g_app_var.isBat_low = false;

    app_get_esp_mac();
}



/*****************************************************************************
  * Function:	  
  * 		 app_start_task
  * Description: 
  * 		 create and start task
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_start_task(void)
{
    ESP_LOGI(TAG, "START APP TASK");
    xTaskCreate(app_common_task, "app_common_task", 3072, NULL, 3, NULL);
    xTaskCreate(app_send_rf_task, "app_send_rf_task", 8192, NULL, 6, NULL);
    xTaskCreate(app_send_data_task, "app_send_data_task", 10240, NULL, 7, NULL);
    xTaskCreate(nirs_task_handler, "nirs_task_handler", 3072, NULL, 8, NULL);
    xTaskCreate(app_imu_task, "app_imu_task", 6144, NULL, 4, NULL);
//    xTaskCreate(app_receive_task, "app_receive_task", 4096, NULL, 5, NULL);
    
}


/*****************************************************************************
  * Function:	  
  * 		 app_esp32_init
  * Description: 
  * 		 esp32 init
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_esp32_init(void)
{

    xTimerQueue = xQueueCreate(10, sizeof(int));
    g_app_var.payload = malloc(NIRS_DATA_LEN);
    app_nvs_init();

    power_key_init();
    led_init();  

    //NIRS config
    NIRS_Init();

    app_para_init();

    //gpio log level
    esp_log_level_set("gpio", ESP_LOG_ERROR); 
    esp_log_level_set("SDMMC", ESP_LOG_ERROR);                      

    //ads1292 config
    // EMG_ads1292_init(g_app_var.emg_dr);

    //mpu9250 config
    mpu9250_init();

    //adc config
    adc_init();
    app_get_battery_level();

    //cmt2300
    Cmt2300_Init();

    //sd card
    sd_card_init();

    //wifi config
    app_wifi_and_mdns_conf(); 

    //vTaskExample();



    
}