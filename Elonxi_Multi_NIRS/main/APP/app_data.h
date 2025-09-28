/**
 ****************************************************************************************************
* @file        app_data.h
* @author      zz
* @version     V1.0
* @date        2025-04-25
* @brief       OTA for WiFi
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __APP_DATA_H
#define __APP_DATA_H

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
void app_get_rf_nirs_dr_index_nvs(void);
void app_set_rf_nirs_dr_index_nvs(void);
uint8_t app_get_rf_nirs_dr(void);
#endif
