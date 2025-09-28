/**
 ****************************************************************************************************
* @file        app.h
* @author      zz
* @version     V1.0
* @date        2024-09-12
* @brief       WIFI UDP
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __APP_H
#define __APP_H

/*---------------------------------------------------------------------
 Include      				  				
-----------------------------------------------------------------------*/
#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "filters.h"

#define BYTE0(dwTemp)       ( *( (char *)(&dwTemp)	  ) )
#define BYTE1(dwTemp)       ( *( (char *)(&dwTemp) + 1) )
#define BYTE2(dwTemp)       ( *( (char *)(&dwTemp) + 2) )
#define BYTE3(dwTemp)       ( *( (char *)(&dwTemp) + 3) )

/*---------------------------------------------------------------------
Macros       				  				
-----------------------------------------------------------------------*/
#define SET_BIT(num, n)    ((num) |= (1U << (n)))
#define CLEAR_BIT(num, n)  ((num) &= ~(1U << (n)))
#define GET_BIT(num, n)    (((num) >> (n)) & 1U)

//MDNS
//#define  _MDNS_


/*----------------------------------------------------------------------
 Structure and enum           										
-----------------------------------------------------------------------*/
typedef struct 
{
    bool isWiFi_connected;   //WiFi
    bool isBat_low;   //low battery
    uint8_t isPC_connected;
    uint8_t isRF;
    uint8_t synFlag;
    uint8_t *payload;
    uint8_t mac[7];
    uint8_t serialNumber[9];
    uint32_t rf_syn_packcnt;  //RF 同步时间戳
    uint8_t  abs_rssi;
    uint8_t markerID[8];
    //emg
    uint8_t isEMG_channel_1;
    uint8_t isEMG_channel_2;
    uint8_t emg_dr;
    uint16_t emg_dr_pack;
    int32_t rf_emg_dr_index;
    uint8_t emg_sendFlag;
    uint32_t emg_packet_counter;
    uint8_t emg_sd_count;
    uint8_t emg_sd_write_flag;
    uint8_t emg_sd_read_buffer[2500];
    uint8_t emg_sd_ready_packcnt;
    uint8_t emg_leadoff;
    //nirs
    uint8_t is_NIRS;
    uint8_t nirs_dr;
    uint16_t nirs_dr_pack;
    int32_t rf_nirs_dr_index;
    uint8_t nirs_sendFlag;
    uint32_t nirs_packet_counter;
    uint8_t nirs_sd_count;
    uint8_t nirs_sd_write_flag;
    uint8_t nirs_sd_read_buffer[2500];
    uint32_t nirs_sd_ready_packcnt;	//写入数据包编号
    //imu
    uint8_t isIMU;
    uint8_t imu_sd_count;
    uint8_t imu_sd_write_flag;
    uint8_t imu_sd_read_buffer[512];
    uint32_t imu_sd_ready_packcnt;
}APP_GLOBAL;

typedef enum
{
    TYPE_WIFI_STATUS = 0xB0,
    TYPE_PARA_CONFIG = 0xB1,
    TYPE_EMG_DR = 0xB2,
    TYPE_START_STOP = 0xB3,
    TYPE_EMG_DATA = 0xB4,
    TYPE_IMU_DATA = 0xB5,
    TYPE_ADD_PACKET = 0xB9,
    TYPE_ERROR_UPLOAD = 0xBF
}DATA_TYPE;

typedef enum
{
    PACKET_TX_HEADER = 0xA0, 
    PACKET_RX_HEADER = 0xA5, 
    PACKET_TAIL_1 = 0x0D,
    PACKET_TAIL_2 = 0x0A
}PACKET_DATA;

typedef enum
{
    ERROR_UNKNOW = 0x00,
    ERROR_PACKET_HEAD_TAIL = 0x01,
    ERROR_PACKET_XOR = 0x02
}ERROR_TYPE;

typedef enum
{
    EMG_CHANNEL_1 = 0x01,
    EMG_CHANNEL_2 = 0x02
}EMG_CHANNEL;

typedef enum
{
    DEVICE_MINI_EMG = 0x01,
    DEVICE_MINI_NIRS= 0x01,
}DEVICE_TYPE;

typedef enum
{
    DR_125 = 0x00,
    DR_250 = 0x01,
    DR_500 = 0x02,
    DR_1K = 0x03,
    DR_2K = 0x04,
    DR_4K = 0x05, 
}EMG_DR;



extern APP_GLOBAL g_app_var;
/*----------------------------------------------------------------------
 Local functions           										
-----------------------------------------------------------------------*/
void app_nvs_init(void);
void app_start_task(void);
void app_esp32_init(void);
void udp_send_data(const uint8_t* data, size_t len);
void app_check_battery_level(void);
#endif
