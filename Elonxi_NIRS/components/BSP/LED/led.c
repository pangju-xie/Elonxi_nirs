/**
 ****************************************************************************************************
* @file        led.c
* @author      zz
* @date        2024-09-09
* @brief       led
* @license     交浦科技
****************************************************************************************************
*/

/*----------------------------------------------------------------------
 Include										
-----------------------------------------------------------------------*/
#include "esp_attr.h"  // 包含 IRAM_ATTR 宏
#include "led.h"
#include "esp_log.h"
//#include "NIRS.h"



uint8_t  g_power_key_flag= 0;
static char* TAG = "GPIO";


/*****************************************************************************
  * Function:	  
  * 		 led_init
  * Description: 
  * 		 rgb led initialization
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void led_init(void)
{
    gpio_config_t gpio_led_r_conf = {0};
    gpio_config_t gpio_led_g_conf = {0};
    gpio_config_t gpio_led_b_conf = {0};
    

    gpio_led_r_conf.intr_type = GPIO_INTR_DISABLE;          /* 失能引脚中断 */
    gpio_led_r_conf.mode = GPIO_MODE_INPUT_OUTPUT;          /* 输入输出模式 */
    gpio_led_r_conf.pull_up_en = GPIO_PULLUP_DISABLE;       /* 失能上拉 */
    gpio_led_r_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   /* 失能下拉 */
    gpio_led_r_conf.pin_bit_mask = 1ull << LED_R_PIN;       /* 设置的引脚的位掩码 */
    gpio_config(&gpio_led_r_conf);                          /* 配置GPIO */

    gpio_led_g_conf.intr_type = GPIO_INTR_DISABLE;          /* 失能引脚中断 */
    gpio_led_g_conf.mode = GPIO_MODE_INPUT_OUTPUT;          /* 输入输出模式 */
    gpio_led_g_conf.pull_up_en = GPIO_PULLUP_DISABLE;       /* 失能上拉 */
    gpio_led_g_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   /* 失能下拉 */
    gpio_led_g_conf.pin_bit_mask = 1ull << LED_G_PIN;       /* 设置的引脚的位掩码 */
    gpio_config(&gpio_led_g_conf); 

    gpio_led_b_conf.intr_type = GPIO_INTR_DISABLE;          /* 失能引脚中断 */
    gpio_led_b_conf.mode = GPIO_MODE_INPUT_OUTPUT;          /* 输入输出模式 */
    gpio_led_b_conf.pull_up_en = GPIO_PULLUP_DISABLE;       /* 失能上拉 */
    gpio_led_b_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   /* 失能下拉 */
    gpio_led_b_conf.pin_bit_mask = 1ull << LED_B_PIN;       /* 设置的引脚的位掩码 */
    gpio_config(&gpio_led_b_conf); 

    LED_R(0);                                                 /* 打开LED */
    LED_G(1);                                                 /* OFF LED */
    LED_B(1);                                                 /* OFF LED */
}


/*****************************************************************************
  * Function:	  
  * 		 key_exti_gpio_isr_handler
  * Description: 
  * 		 power key exit handler
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
static void IRAM_ATTR key_exti_gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;

    //if(gpio_num == POWER_KEY_PIN)
    if(gpio_get_level(POWER_KEY_PIN) == 0)
    {
        g_power_key_flag = 1;
    }
    else
    {
        g_power_key_flag = 0;
    }
}


/*****************************************************************************
  * Function:	  
  * 		 app_power_key_onoff
  * Description: 
  * 		 power key on/off 
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
uint8_t app_power_key_onoff(void)
{
    static uint8_t  count = 0;

    if(g_power_key_flag)
    {
        count++;
        
        if(count > 1)
        {
            count = 0;
            LED_R(1);
            LED_G(1);
            LED_B(1);
            POWER_OFF;
            //NIRS_Stop();
            ESP_LOGI(TAG, "POWER_OFF");
            return 1;
        }
    }
    else
    {
        count = 0;
    }
    return 0;
}


/*****************************************************************************
  * Function:	  
  * 		 power_key_exti_init
  * Description: 
  * 		 power key exti init 
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void power_key_exti_init(void)
{
    gpio_config_t gpio_power_key_init_struct;

    gpio_power_key_init_struct.mode = GPIO_MODE_INPUT;                    /* 选择为输入模式 */
    gpio_power_key_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;           /* 上拉使能 */
    gpio_power_key_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;      /* 下拉失能 */
    gpio_power_key_init_struct.intr_type = GPIO_INTR_NEGEDGE;             /* 下降沿触发 */
    gpio_power_key_init_struct.pin_bit_mask = 1ull << POWER_KEY_PIN;       /* 配置BOOT按键引脚 */
    gpio_config(&gpio_power_key_init_struct);                             /* 配置使能 */
    
    /* 注册中断服务 */
    gpio_install_isr_service(0); //ESP_INTR_FLAG_EDGE
    
    /* 设置GPIO的中断回调函数 */
    gpio_isr_handler_add(POWER_KEY_PIN, key_exti_gpio_isr_handler, (void*) POWER_KEY_PIN);
    gpio_intr_enable(POWER_KEY_PIN);
}



/*****************************************************************************
  * Function:	  
  * 		 power_key_init
  * Description: 
  * 		 key power off/on init
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void power_key_init(void)
{
    gpio_config_t gpio_power_key_conf = {0};

    gpio_power_key_conf.intr_type = GPIO_INTR_DISABLE;          /* 失能引脚中断 */
    gpio_power_key_conf.mode = GPIO_MODE_INPUT_OUTPUT;          /* 输入输出模式 */
    gpio_power_key_conf.pull_up_en = GPIO_PULLUP_DISABLE;       /* 失能上拉 */
    gpio_power_key_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   /* 失能下拉 */
    gpio_power_key_conf.pin_bit_mask = 1ull << POWER_HOLD_PIN;       /* 设置的引脚的位掩码 */
    gpio_config(&gpio_power_key_conf); 

    POWER_ON;

    power_key_exti_init();
}



/*****************************************************************************
  * Function:	  
  * 		 led_set
  * Description: 
  * 		 rgb led set or reset
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void led_set(char color,uint8_t level)
{
    switch (color)
    {
        case 'R':
            if(PIN_SET == level)
            {
                LED_R(1);
                LED_G(1);
                LED_B(1);
            }
            else
            {
                LED_R(0);
                LED_G(1);
                LED_B(1);
            }
            break;
        case 'G':
            if(PIN_SET == level)
            {
                LED_R(1);
                LED_G(1);
                LED_B(1);
            }
            else
            {
                LED_R(1);
                LED_G(0);
                LED_B(1);
            }
            break;
        case 'B':
            if(PIN_SET == level)
            {
                LED_R(1);
                LED_G(1);
                LED_B(1);
            }
            else
            {
                LED_R(1);
                LED_G(1);
                LED_B(0);
            }
            break;
        default:
            break;
    }
}