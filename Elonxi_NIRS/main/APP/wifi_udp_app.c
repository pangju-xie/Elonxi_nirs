/**
 ****************************************************************************************************
* @file        wifi_udp_app.c
* @author      zz
* @version     V1.0
* @date        2025-02-12
* @brief       WIFI UDP TO APP
* @license     ELONXI
****************************************************************************************************
*/


#include <stdio.h>
#include <string.h>
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

#include "sdmmc.h"
#include "app.h"
#include "wifi_data.h"
#include "NIRS.h"
#include "adc1.h"
#include "wifi_udp_app.h"
#include "i2c_mpu9250.h"


int udpUpAppSock=-1;
struct sockaddr_in upAppAddr;

IMU_PARA  g_struct_imu_para;
/*****************************************************************************
  * Function:	  
  * 		 packageInfoMessage
  * Description: 
  * 		 send  packet information message to app
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
int packageInfoMessage(uint8_t *data, uint8_t *sn)
{
    uint16_t crc=0;
    uint8_t i = 0;
    uint8_t bat = app_get_battery_level();

    //head
    data[i++] = 0x5A;

    //length
    data[i++] = 0x0;
    data[i++] = 0x13;

    //cmd
    data[i++] = 0xF1;

    //sn
    memcpy(data+i, sn, 8);
    i+=8;

    //battery
    data[i++] = bat;

    //version
    //data[i++] = (DEVICE_VERSION>>24)&0xFF;
    //data[i++] = (DEVICE_VERSION>>16)&0xFF;
    data[i++] = (DEVICE_VERSION>>8)&0xFF;
    data[i++] = DEVICE_VERSION&0xFF;

    //leadoff
    //data[i++] = g_app_var.leadoff;

    //rssi
    data[i++] = g_app_var.abs_rssi;

    //reserved
    memset(data+i, 0, 7);
    i+=7;

    //modify length
    data[1] = (i>>8)&0xFF;
    data[2] = i&0xFF;

    //crc
    crc = CRC16(data, i);
    data[i++] = (crc >> 8)&0xFF;
    data[i++] = crc&0xFF;
    
    return i;
}

/*****************************************************************************
  * Function:	  
  * 		 packageImuMessage
  * Description: 
  * 		 send  packet imu message to app
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
int packageImuMessage(uint8_t *data, uint8_t *sn, uint8_t length, uint32_t stamp)
{
    uint16_t crc=0;
    uint8_t i = 0;
    

    //head
    data[i++] = 0x5A;

    //length
    data[i++] = 0x0;
    data[i++] = 0xDB;

    //cmd
    data[i++] = DEVICE_TYPE_ID+0x2; //0x12 imu

    //package number
    data[i++] = 0x00;
    data[i++] = 0x02;

    //sn
    memcpy(data+i, sn, 8);
    i+=8;

    //stamp
    data[i++] = (stamp >> 24)&0xFF;
    data[i++] = (stamp >> 16)&0xFF;
    data[i++] = (stamp >> 8)&0xFF;
    data[i++] = stamp&0xFF;

    //interval
    data[i++] = 0x01; //ms <= 200ms

    //imu data
    memcpy(data+i, g_struct_imu_para.imu_data, length);  //g_struct_imu_para
    i += length;

    //crc
    crc = CRC16(data, i);
    data[i++] = (crc >> 8)&0xFF;
    data[i++] = crc&0xFF;
    
    return i;
}

/*****************************************************************************
  * Function:	  
  * 		 udpUpAppSendData
  * Description: 
  * 		 Initialize the UDP socket.
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 Return the socket descriptor on success, or -1 on failure.
*****************************************************************************/
int udp_upAppSocketInit(void)
{
	int sock = -1;

	printf("udp host_ip = %s; port = %d\n", DEFAULT_UPAPP_ADDR, DEFAULT_UPAPP_PORT);

	upAppAddr.sin_addr.s_addr = inet_addr(DEFAULT_UPAPP_ADDR);
	upAppAddr.sin_family = AF_INET;
	upAppAddr.sin_port = htons(DEFAULT_UPAPP_PORT);
	
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	if(sock < 0) 
  {
      ESP_LOGE("udp_app", "Unable to create socket: errno %d", errno);
      return -1;
  }	

	return sock;
}



/*****************************************************************************
  * Function:	  
  * 		 udpUpAppSendData
  * Description: 
  * 		 Directly  send  udp data to app
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
int udpUpAppSendData(uint8_t *data, int len)
{
	int err;
  int tmpdata[1024] = {0};
  if(memcmp(tmpdata, data, len)==0){
    ESP_LOGE("UDP PACK DATA", "the data in buffer is all 0,something wrong.");
    err = 0;
  }
  else{
	  err = sendto(udpUpAppSock, data, len, 0, (struct sockaddr *)&upAppAddr, sizeof(upAppAddr));
    if (err < 0) 
    {
      if(errno != 12) 
      {
        ESP_LOGE("TAG", "Error occurred during sending: errno %d", errno);
      }
    }
  }
	//printf("udp send data len = %d\n",len);

  return err;
}


/*****************************************************************************
  * Function:	  
  * 		 sendToUpApp
  * Description: 
  * 		 send  data to app
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
int sendToUpApp(uint8_t type)
{
    uint16_t len=0;

    if(type == UPAPP_INFO)
    {
        len = packageInfoMessage(g_app_var.payload, g_app_var.serialNumber);
    }
    else if(type == UPAPP_REPACK)
    {
        //len = packetSendData(sDb.payload, (uint8_t*)" ", 1, sDb.repackStamp);
    }
    else if(type == UPAPP_IMU)
    {
        len = packageImuMessage(g_app_var.payload, g_app_var.serialNumber, IMU_PACK_LEN, g_imu_packcnt);
    }
    int ret = udpUpAppSendData(g_app_var.payload, len);

    if(ret < 0)
    {
        printf("send upApp data fail!\n");
        return -1;
    }

    return len;
}


/*****************************************************************************
  * Function:	  
  * 		 sendToUpAppInfo
  * Description: 
  * 		 send  data to app for information
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void sendToUpAppInfo(void)
{
    //if(g_app_var.isRF)
    {
        sendToUpApp(UPAPP_INFO);
    }
}

/*****************************************************************************
  * Function:	  
  * 		 sendToUpAppImu
  * Description: 
  * 		 send  data to app for imu data
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void sendToUpAppImu(void)
{
    sendToUpApp(UPAPP_IMU);

    if(g_app_var.imu_sd_write_flag)
    {
        g_app_var.imu_sd_write_flag = 0;
        //printf("IMU SD WRITE cnt = %ld!\n",g_app_var.imu_sd_ready_packcnt);	
        app_sdmmc_write_sectors(G_nirs_data.imu_sd_buffer,IMU_START_BLOCK+g_app_var.imu_sd_ready_packcnt,IMU_BLOCK_LEN*2); 
    }
}


/*****************************************************************************
  * Function:	  
  * 		 createUdpUpAppTask
  * Description: 
  * 		 create socket to APP task
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void createUdpUpAppTask(void)
{
	if(!g_app_var.isWiFi_connected)
  {
      //vTaskDelay(1000/portTICK_PERIOD_MS);
      printf("wifi is not enabled!\n");	// wifi is not enabled
      return;	
	}

	if(udpUpAppSock == -1)
  {
      printf("create udp upApp socket!\n");
      udpUpAppSock = udp_upAppSocketInit();
	}

  //vTaskDelay(50);
  //xTaskCreate(udpUpAppClientTask, "udpUpApp_client", 4096, NULL, 5, NULL);
}


