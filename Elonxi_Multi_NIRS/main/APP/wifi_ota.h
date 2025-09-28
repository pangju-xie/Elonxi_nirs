/**
 ****************************************************************************************************
* @file        wifi_ota.h
* @author      zz
* @version     V1.0
* @date        2025-04-15
* @brief       OTA for WiFi
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __WIFI_OTA_H
#define __WIFI_OTA_H

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


/*----------------------------------------------------------------------
 Structure and enum           										
-----------------------------------------------------------------------*/



/* 声明函数 */
void wifi_ota_task(void *pvParameter);
void wifi_set_ota_flag_nvs(void);
void wifi_get_ota_flag_nvs(void);
uint8_t app_wifi_get_ota_flag(void);
void app_wifi_set_ota_flag(uint8_t flag);
#endif
