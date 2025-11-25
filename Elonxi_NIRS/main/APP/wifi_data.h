/**
 ****************************************************************************************************
* @file        wifi_data.h
* @author      zz
* @version     V1.0
* @date        2025-01-23
* @brief       WIFI UDP
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __WIFI_DATA_H
#define __WIFI_DATA_H

#include <stdio.h>
#include <string.h>
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



// /*---------------------------------------------------------------------
// Macros       				  				
// -----------------------------------------------------------------------*/
// #define uploadInterval        100

// #define SAMPLE_BYTES          3
// #define SAMPLE_CHANNEL        2


// /*
// [间隔100ms]
// 101-1K
// 201-2K
// 401-4K
// b0-b5预留
// b0-leadoff
// */
// #define SAMPLE_RATE_EMG_1K       101//4000/(1000/uploadInterval) 100
// #define SAMPLE_RATE_EMG_2K       201
// #define SAMPLE_RATE_EMG_4K       401

// /*
// [间隔50ms]
// 50-1K
// 100-2K
// 200-4K
// */
// //#define SAMPLE_RATE_EMG       50//4000/(1000/uploadInterval) 100

// #define DEVICE_TYPE_ID        0x10 

/*---------------------------------------------------------------------
Macros       				  				
-----------------------------------------------------------------------*/
#define uploadInterval              100

#define NIRS_CHANNEL                1
#define NIRS_BYTES                  16

#define SAMPLE_RATE_NIRS_100        10       
#define SAMPLE_RATE_NIRS_50         5
#define SAMPLE_RATE_NIRS_20         2
#define SAMPLE_RATE_NIRS_10         1

#define DEVICE_TYPE_ID          0x50
/*----------------------------------------------------------------------
 Structure and enum           										
-----------------------------------------------------------------------*/
enum 
{
    RF_HEAD = 0x5A,
    RF_APP_HEAD = 0xA5,
    RF_SYN=0x10,
    RF_MARK=0x20,
    RF_OTA = 0x5F,//0xF4,
    RF_IMU_START = 0x5E,
    RF_IMU_STOP = 0x5D,
    RF_BINDING = 0x5C,
    RF_POWEROFF = 0x5B,

    RF_START=0xA0,
    RF_STOP=0xA1,
    RF_REPACK=0xF0,
    RF_INFO=0xF1,
    RF_CON_ROUTER = 0xF2,
    RF_DR = 0xF3,
    RF_REPACK_MULTI=0xF5,
    RF_REPACK_IMU_MULTI=0xF6, 
};

enum 
{
    UPAAP_NONE=0,
    UPAPP_REPACK,
    UPAPP_INFO,
    UPAPP_IMU,
};

enum
{
    TYPE_MSG = 0,
    TYPE_DATA,
};


/* 声明函数 */
uint16_t CRC16(const uint8_t* pData, uint32_t uLen);
int udpSendSensorData(uint8_t type);
int packetSendMessage(uint8_t *data,uint8_t *sn, uint8_t synFlag);
#endif
