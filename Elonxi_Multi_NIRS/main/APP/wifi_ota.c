/**
 ****************************************************************************************************
* @file        wifi_ota.c
* @author      zz
* @version     V1.0
* @date        2025-04-15
* @brief       OTA for WiFi
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


static const char *TAG = "OTA";
#define OTA_URL "http://192.168.218.10:31311/Elonxi_NIRS.bin"

static int32_t nvs_ota_flag;
static uint8_t g_ota_flag;


/*****************************************************************************
  * Function:	  
  * 		 wifi_get_ota_flag_nvs
  * Description: 
  * 		 get ota flag for nvs
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void wifi_get_ota_flag_nvs(void)
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
        printf("Reading ota flag from NVS ... ");
        
        err = nvs_get_i32(my_handle, "ota_flag", &nvs_ota_flag);

        switch (err) 
        {
            case ESP_OK:
                printf("Done\n");
                printf("nvs_ota_flag = %" PRIu32 "\n", nvs_ota_flag);
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
  * 		 wifi_set_ota_flag_nvs
  * Description: 
  * 		 set ota flag for nvs
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void wifi_set_ota_flag_nvs(void)
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
        // Write
        printf("Updating ota flag in NVS ... ");
        err = nvs_set_i32(my_handle, "ota_flag", nvs_ota_flag);
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
  * 		 app_wifi_get_ota_flag
  * Description: 
  * 		 app get ota flag 
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
uint8_t app_wifi_get_ota_flag(void)
{
    return g_ota_flag;
}


/*****************************************************************************
  * Function:	  
  * 		 app_wifi_set_ota_flag
  * Description: 
  * 		 app set ota flag 
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_wifi_set_ota_flag(uint8_t flag)
{
    g_ota_flag = flag;
}


/*****************************************************************************
  * Function:	  
  * 		 wifi_ota_task
  * Description: 
  * 		 ota task for wifi
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) 
    {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            int *disconnect_reason = (int *)evt->data;
            if (disconnect_reason != NULL)
            {
                ESP_LOGE(TAG, "Disconnected! Reason: %s (code %d)", 
                      esp_err_to_name(*disconnect_reason), 
                      *disconnect_reason);
            } 
            else 
            {
                ESP_LOGE(TAG, "Disconnected with unknown reason");
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}



/*****************************************************************************
  * Function:	  
  * 		 wifi_ota_task
  * Description: 
  * 		 ota task for wifi
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void wifi_ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA update from %s", OTA_URL);
    
    esp_http_client_config_t config = {
        .url = OTA_URL,
        .event_handler = http_event_handler,
        .keep_alive_enable = true,
        .timeout_ms = 5000,
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
    };
    
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        goto ota_end;
    }
    
    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed");
        goto ota_end;
    }
    
    // 验证新固件版本（可选）
    ESP_LOGI(TAG, "New firmware version: %s", app_desc.version);
    
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // 显示下载进度
        int progress = esp_https_ota_get_image_len_read(https_ota_handle) * 100 / 
                      esp_https_ota_get_image_size(https_ota_handle);
        ESP_LOGI(TAG, "Download progress: %d%%", progress);
        LED_B_TOGGLE();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        ESP_LOGE(TAG, "Complete data was not received");
    } else {
        err = esp_https_ota_finish(https_ota_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OTA update successful, restarting...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        }
    }
    
ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG, "OTA update failed");
    vTaskDelete(NULL);
}