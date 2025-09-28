/**
 ****************************************************************************************************
* @file        main.c
* @author      zz
* @version     V1.0
* @date        2024-09-09
* @brief       WIFI UDP
* @license     ELONXI
****************************************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "led.h"
#include "wifi_data.h"
#include "app.h"

#include "led.h"
#include "adc1.h"


/*FreeRTOS*********************************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include <portmacro.h>
/******************************************************************************************************/


/*FreeRTOS配置*/
/* LED_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define LED_TASK_PRIO           10          /* 任务优先级 */
#define LED_STK_SIZE            2048        /* 任务堆栈大小 */
TaskHandle_t LEDTask_Handler;               /* 任务句柄 */


//static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{

    app_esp32_init();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    app_start_task();

}

// void vApplicationIdleHook(void)
// {
// 	esp_task_wdt_reset();
// }

