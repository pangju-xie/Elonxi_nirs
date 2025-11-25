/**
 ****************************************************************************************************
* @file        adc1.c
* @author      xfw
* @version     V1.0
* @date        2025-05-13
* @brief       adc config
* @license     ELONXI
****************************************************************************************************
*/

#include <esp_sleep.h>
#include "adc1.h"
#include "esp_adc_cal.h"
#include "led.h"


static esp_adc_cal_characteristics_t *adc_chars;

/**
 * @brief       初始化ADC
 * @param       无
 * @retval      无
 */
void adc_init(void)
{
    adc_digi_pattern_config_t adc1_digi_pattern_config;         /* ADC1配置句柄 */
    adc_digi_configuration_t adc1_init_config;                  /* ADC1初始化句柄 */
    
    /* 配置ADC1 */
    adc1_digi_pattern_config.atten = ADC_ATTEN_DB_12;           /* 配置ADC衰减程度 */
    adc1_digi_pattern_config.channel = ADC_ADCX_CHY;            /* 配置ADC通道 */
    adc1_digi_pattern_config.unit = ADC_UNIT_1;                 /* 配置ADC单元 */
    adc1_digi_pattern_config.bit_width = ADC_BITWIDTH_12;       /* 配置ADC位宽 */
    adc1_init_config.adc_pattern = &adc1_digi_pattern_config;   /* 配置将要使用的每个ADC参数 */
    adc_digi_controller_configure(&adc1_init_config);           /* 配置ADC1 */

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, adc_chars);  // 校准
}

/**
 * @brief       获取ADC转换且进行均值滤波后的结果
 * @param       ch      : 通道号, 0~9
 * @param       times   : 获取次数
 * @retval      通道ch的times次转换结果平均值
 */
uint32_t adc_get_result_average(uint32_t ch, uint32_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;

    for (t = 0; t < times; t++) /* 获取times次数据 */
    {
        temp_val += adc1_get_raw(ch);
        vTaskDelay(5);
    }

    return temp_val / times;    /* 返回平均值 */
}


/*****************************************************************************
  * Function:	  
  * 		 adc_get_voltage_val
  * Description: 
  * 		 adc get voltage
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 uint16_t
*****************************************************************************/
uint32_t adc_get_voltage_val(void)
{
    uint16_t adcdata;
    uint32_t voltage;

    adcdata = adc_get_result_average(ADC_ADCX_CHY, 20);

    voltage = esp_adc_cal_raw_to_voltage(adcdata, adc_chars)*2;

    //voltage = (float)adcdata * (3.3 / 4095) * 2;    
    printf("voltage = %d %ldmV\r\n",adcdata, voltage); 
    return voltage;
}


/*****************************************************************************
  * Function:	  
  * 		 app_get_battery_level
  * Description: 
  * 		 app get battery level
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
uint8_t app_get_battery_level(void)
{
    uint32_t adc_val = 0;
    uint8_t  bat_level = 0;


    adc_val = adc_get_voltage_val();

    if(BAT_LEVEL_5 <= adc_val)
    {
        bat_level = 100;
    }
    else if(BAT_LEVEL_4 <= adc_val && BAT_LEVEL_5 > adc_val)
    {
        bat_level = 80;
    }
    else if(BAT_LEVEL_3 <= adc_val && BAT_LEVEL_4 > adc_val)
    {
        bat_level = 60;
    }
    else if(BAT_LEVEL_2 <= adc_val && BAT_LEVEL_3 > adc_val)
    {
        bat_level = 40;
    }
    else if(BAT_LEVEL_1 <= adc_val && BAT_LEVEL_2 > adc_val)
    {
        bat_level = 20;
    }
    else
    { 
        bat_level = 0;
    }

    return bat_level;
}