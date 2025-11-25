/**
 ****************************************************************************************************
* @file        adc1.hs
* @author      zz
* @version     V1.0
* @date        2024-09-23
* @brief       adc config
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __ADC_H_
#define __ADC_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define  BAT_LEVEL_0   3200  
#define  BAT_LEVEL_1   3300  
#define  BAT_LEVEL_2   3400  
#define  BAT_LEVEL_3   3600  
#define  BAT_LEVEL_4   3800 
#define  BAT_LEVEL_5   3950  

#define  ADC_ADCX_CHY   ADC1_CHANNEL_0 


/* 函数声明 */
void adc_init(void);                                            /* 初始化ADC */
uint32_t adc_get_result_average(uint32_t ch, uint32_t times);   /* 获取ADC转换且进行均值滤波后的结果 */
uint32_t adc_get_voltage_val(void);
uint8_t app_get_battery_level(void);

#endif
