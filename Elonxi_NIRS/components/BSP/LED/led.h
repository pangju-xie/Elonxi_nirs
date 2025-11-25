/**
 ****************************************************************************************************
* @file        led.h
* @author      zz
* @date        2024-09-09
* @brief       led
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __LED_H_
#define __LED_H_

#include "driver/gpio.h"


/* 引脚定义 */
#define POWER_KEY_PIN    (GPIO_NUM_4)
#define POWER_HOLD_PIN   (GPIO_NUM_3)

#define LED_R_PIN        (GPIO_NUM_41)
#define LED_G_PIN        (GPIO_NUM_42)
#define LED_B_PIN        (GPIO_NUM_40)

/* 引脚的输出的电平状态 */
enum GPIO_OUTPUT_STATE
{
    PIN_RESET,
    PIN_SET
};

/* LED端口定义 */
#define LED_R(x)          do { x ?                                 \
                             gpio_set_level(LED_R_PIN, PIN_SET) :  \
                             gpio_set_level(LED_R_PIN, PIN_RESET); \
                          } while(0)  /* LED翻转 */

#define LED_G(x)          do { x ?                                 \
                             gpio_set_level(LED_G_PIN, PIN_SET) :  \
                             gpio_set_level(LED_G_PIN, PIN_RESET); \
                          } while(0)  /* LED翻转 */

#define LED_B(x)          do { x ?                                 \
                             gpio_set_level(LED_B_PIN, PIN_SET) :  \
                             gpio_set_level(LED_B_PIN, PIN_RESET); \
                          } while(0)  /* LED翻转 */


/* 开关机电源控制引脚 */
#define POWER_ON    gpio_set_level(POWER_HOLD_PIN, PIN_SET)
#define POWER_OFF   gpio_set_level(POWER_HOLD_PIN, PIN_RESET)

/* LED取反定义 */
#define LED_R_TOGGLE()    do { gpio_set_level(LED_R_PIN, !gpio_get_level(LED_R_PIN)); } while(0)  /* LED翻转 */
#define LED_G_TOGGLE()    do { gpio_set_level(LED_G_PIN, !gpio_get_level(LED_G_PIN)); } while(0)  /* LED翻转 */
#define LED_B_TOGGLE()    do { gpio_set_level(LED_B_PIN, !gpio_get_level(LED_B_PIN)); } while(0)  /* LED翻转 */

/* 函数声明*/
void led_init(void);    /* 初始化LED */
void led_set(char color,uint8_t level);
void power_key_init(void);
void power_key_exti_init(void);
uint8_t app_power_key_onoff(void);
#endif
