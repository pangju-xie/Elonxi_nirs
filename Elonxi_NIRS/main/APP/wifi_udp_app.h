/**
 ****************************************************************************************************
* @file        wifi_udp_app.h
* @author      zz
* @version     V1.0
* @date        2025-01-23
* @brief       WIFI UDP TO APP
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __WIFI_UDP_APP_H
#define __WIFI_UDP_APP_H

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



/*---------------------------------------------------------------------
Macros       				  				
-----------------------------------------------------------------------*/
//#define DEVICE_VERSION        0x00000001
#define DEVICE_VERSION        0x0004


#define DEFAULT_UPAPP_ADDR    "192.168.218.10"     //hospcld.gianta.com.cn"  // "192.168.0.50"
#define DEFAULT_UPAPP_PORT    22341

/*----------------------------------------------------------------------
 Structure and enum           										
-----------------------------------------------------------------------*/



/* 声明函数 */
void createUdpUpAppTask(void);
void sendToUpAppInfo(void);
void sendToUpAppImu(void);
int udpUpAppSendData(uint8_t *data, int len);
#endif
