/**
 ****************************************************************************************************
* @file        app_data.c
* @author      zz
* @version     V1.0
* @date        2025-04-25
* @brief       data for app
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
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "lwip/err.h"   
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "esp_ping.h"
#include "lwip/ip4_addr.h"  
#include "app.h"
#include "wifi_data.h"
#include "led.h"

/*---------------------------------------------------------------------
Global variables         				  				
-----------------------------------------------------------------------*/

nvs_handle_t my_handle;

/*****************************************************************************
  * Function:	  
  * 		 app_get_rf_nirs_dr_index_nvs
  * Description: 
  * 		 get rf nirs dr index for nvs
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_get_rf_nirs_dr_index_nvs(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else 
    {
        //Read
        printf("Reading nirs dr from NVS ... ");
        
        err = nvs_get_i32(my_handle, "rf_nirs_dr_index", &g_app_var.rf_nirs_dr_index);

        switch (err) 
        {
            case ESP_OK:
                printf("Done\n");
                printf("g_app_var.rf_nirs_dr_index = %" PRIu32 "\n", g_app_var.rf_nirs_dr_index);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }

    // Close
    nvs_close(my_handle);
}



/*****************************************************************************
  * Function:	  
  * 		 app_set_rf_nirs_dr_index_nvs
  * Description: 
  * 		 set rf nirs dr index for nvs
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_set_rf_nirs_dr_index_nvs(void)
{
    
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else 
    {
        // Write
        printf("Updating nirs dr in NVS ... ");
        err = nvs_set_i32(my_handle, "rf_nirs_dr_index", g_app_var.rf_nirs_dr_index);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    }

    // Close
    nvs_close(my_handle);
}



/*****************************************************************************
  * Function:	  
  * 		 app_get_rf_nirs_dr
  * Description: 
  * 		 app get rf nirs dr index
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
uint8_t app_get_rf_nirs_dr(void)
{
    return g_app_var.rf_nirs_dr_index;
}


/*****************************************************************************
  * Function:	  
  * 		 app_set_rf_nirs_dr
  * Description: 
  * 		 app set rf nirs dr index
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_set_rf_nirs_dr(uint8_t flag)
{
    g_app_var.rf_nirs_dr_index = flag;
}

