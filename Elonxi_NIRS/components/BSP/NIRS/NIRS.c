#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "driver/gptimer.h"
#include "driver/mcpwm_cap.h"
#include "driver/mcpwm_prelude.h"
#include "esp_private/esp_clk.h"

#include "esp_err.h"
#include "esp_log.h"

#include "NIRS.h"
#include "led.h"
#include "filters.h"

static int send_count = 0;
static const char* TAG = "NIRS";
volatile uint8_t nirs_timer_flag = 0;
QueueHandle_t xTimerQueue;
#define lnten		2.302585f
NIRS_CONTEXT g_nirs_ctx;
NIRS_DATA G_nirs_data;
static NIRS_DATA_CACHE s_data_cache;
uint8_t nirs_flag = 0;
uint8_t bat_flag = 0;

static TickType_t startTick, endTick;

MedianFilter* medfilter_red = NULL;
MedianFilter* medfilter_ir = NULL;

// 设置 MCPWM 捕获定时器的时钟频率（通常为 80 MHz）
#define MCPWM_TIMER_CLK  10000000UL
// MCPWM 相关句柄
static mcpwm_cap_timer_handle_t cap_timer = NULL;
static mcpwm_cap_channel_handle_t cap_channel = NULL;
// 用于记录捕获到的边沿时间戳
static uint32_t last_pos_edge = 0;
static uint32_t last_neg_edge = 0;
static int last_edge = -1;

// BiquadFilter lpf_hb;
// BiquadFilter lpf_hbo2;

// 单次遍历计算均值和方差（数值稳定）
float RemoveOutPoint(float* buf) {
    //计算均值和标准差
    double delta, m2 = 0.0;
    float mean, std = 0.0;
    mean = buf[0];
    
    for (int i = 1; i < 10; i++) {
        delta = buf[i] - mean;
        mean += delta / (i + 1);
        m2 += delta * (buf[i] - mean);
    }
    
    std = sqrt(m2 / 10);       // 总体方差
    printf("mean = %.2f, std = %.2f\r\n", mean, std);
    int ret = 10;
    for(int i = 0; i<10; i++){
        //查询并剔除离群点
        delta = buf[i]-mean;
        if(fabs(delta)>2*std){
            ret = ret-1;
            mean -= delta/ret;
            m2 -= delta*(buf[i]-mean);
            std = sqrt(m2/ret);
            printf("remove outline point: %.2f\r\n", buf[i]);
        }
    }
    return mean;
    // *variance = m2 / (n - 1); // 样本方差
}

//NIRS控制光源开闭
static void LED_CTRL_Init(void){
    gpio_config_t nirs_ctrl = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1<<RED_CTRL_PIN| 1<<IR_CTRL_PIN),
        .pull_down_en = 1,
        .pull_up_en = 0,
    };
    ESP_ERROR_CHECK(gpio_config(&nirs_ctrl));

    gpio_set_level(RED_CTRL_PIN, 0);
    gpio_set_level(IR_CTRL_PIN, 0);
}

//PWM驱动控制光源光强大小
static void LED_PWM_Init(void){
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel[2] = {
        {
            .speed_mode     = LEDC_MODE,
            .channel        = LEDC_CHANNEL_0,
            .timer_sel      = LEDC_TIMER,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = RED_PWM_PIN,
            .duty           = 0, // Set duty to 100%
            .hpoint         = 0
        },
        {
            .speed_mode     = LEDC_MODE,
            .channel        = LEDC_CHANNEL_1,
            .timer_sel      = LEDC_TIMER,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = IR_PWM_PIN,
            .duty           = 0, // Set duty to 100%
            .hpoint         = 0
        },
    };
    ESP_ERROR_CHECK(ledc_channel_config(ledc_channel));
}

//设置PWM占空比，
void SetPWMDuty(uint8_t duty){
    uint16_t nirs_pwm_duty = (uint16_t)((1<<13)*duty/100);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, nirs_pwm_duty);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, nirs_pwm_duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1);
}


//pwm输入捕获测试光强

static bool capture_cb(mcpwm_cap_channel_handle_t cap_ch,
                       const mcpwm_capture_event_data_t *edata,
                       void *user_data)
{
    // 若连续两个边沿类型一样，忽略
    if (edata->cap_edge == last_edge) {
        return true;
    }
    last_edge = edata->cap_edge;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        // 捕获到上升沿
        if (last_neg_edge > 0) {
            // 计算高电平时间、低电平时间和周期
            uint32_t low_time = edata->cap_value - last_neg_edge;
            uint32_t high_time = last_neg_edge - last_pos_edge;
            uint32_t period = high_time + low_time;

            if (period > 0) {
                // 计算频率 (Hz) 和占空比 (%)
                uint32_t freq = MCPWM_TIMER_CLK / period;
                // uint32_t duty = (high_time * 10000) / period; // 万分比
                G_nirs_data.pwm_cap.value += freq;
                G_nirs_data.pwm_cap.count++;
                
                // 输出频率和占空比
                // ESP_LOGI(TAG, "Freq: %d Hz, Duty: %.2f%%", freq, duty / 100.0f);
            }
        }
        last_pos_edge = edata->cap_value;
    } else if (edata->cap_edge == MCPWM_CAP_EDGE_NEG) {
        // 捕获到下降沿
        last_neg_edge = edata->cap_value;
    }

    return true; // 返回 true 表示中断处理成功
}


void PWM_Capture_Force_Cleanup(void)
{
    ESP_LOGI(TAG, "强制清理 MCPWM 捕获资源");
    
    // 强制清理所有可能的 MCPWM 捕获资源
    for (int group = 0; group < 2; group++) {
        mcpwm_cap_timer_handle_t timer;
        mcpwm_capture_timer_config_t timer_config = {
            .group_id = group,
            .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        };
        
        // 尝试创建定时器来检查是否被占用
        esp_err_t ret = mcpwm_new_capture_timer(&timer_config, &timer);
        if (ret == ESP_OK) {
            // 如果能创建，说明该组可用，立即删除
            mcpwm_del_capture_timer(timer);
            ESP_LOGI(TAG, "释放组 %d 资源", group);
        }
    }
}


void PWM_Capture_Init(void)
{
    // 先强制清理可能的残留资源
    PWM_Capture_Force_Cleanup();
    
    // 创建 MCPWM 捕获定时器
    mcpwm_capture_timer_config_t timer_config = {
        .group_id = 0,  // 自动选择可用组
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };
    
    esp_err_t ret = mcpwm_new_capture_timer(&timer_config, &cap_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建捕获定时器失败: 0x%x", ret);
        // 尝试使用预定义的组
        for (int group = 0; group < 2; group++) {
            timer_config.group_id = group;
            ret = mcpwm_new_capture_timer(&timer_config, &cap_timer);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "使用组 %d 成功", group);
                break;
            }
        }
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "所有 MCPWM 组都不可用");
        return;
    }

    // 创建 MCPWM 捕获通道
    mcpwm_capture_channel_config_t ch_cfg = {
        .gpio_num = PD_PIN,
        .prescale = 1,
        .flags.neg_edge = true,
        .flags.pos_edge = true,
    };
    
    ret = mcpwm_new_capture_channel(cap_timer, &ch_cfg, &cap_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建捕获通道失败: 0x%x", ret);
        mcpwm_del_capture_timer(cap_timer);
        cap_timer = NULL;
        return;
    }

    ESP_LOGI(TAG, "PWM 捕获初始化完成，GPIO%d", PD_PIN);

    // 注册中断回调函数
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = capture_cb,
    };
    ret = mcpwm_capture_channel_register_event_callbacks(cap_channel, &cbs, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "注册回调失败: 0x%x", ret);
    }

    // 启用 MCPWM 定时器和通道
    ret = mcpwm_capture_timer_enable(cap_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启用捕获定时器失败: 0x%x", ret);
        return;
    }
    
    ret = mcpwm_capture_channel_enable(cap_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启用捕获通道失败: 0x%x", ret);
        return;
    }

}

/**
 * @brief 启动 PWM 捕获
 *        启用 MCPWM 定时器和捕获通道
 */
void pwm_capture_start(void)
{
    G_nirs_data.pwm_cap.value = 0;
    G_nirs_data.pwm_cap.count = 0;
    esp_err_t ret;
    ret = mcpwm_capture_timer_start(cap_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动捕获定时器失败: 0x%x", ret);
        return;
    }

    // ESP_LOGI(TAG, "PWM 捕获已启动");
}
/**
 * @brief 停止 PWM 捕获
 *        禁用 MCPWM 通道与定时器并释放资源
 */
void pwm_capture_stop(void)
{
    esp_err_t ret;
    ret = mcpwm_capture_timer_stop(cap_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "停止捕获定时器失败: 0x%x", ret);
        return;
    }
    // ESP_LOGI(TAG, "PWM 捕获已停止");
}

//配置NIRS控制定时器
typedef struct {
    uint64_t event_count;
} example_queue_element_t;
static gptimer_handle_t gptimer = NULL;

static bool IRAM_ATTR gptimer_isr_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    g_nirs_ctx.collect_step +=1;
    if(g_nirs_ctx.collect_step == NIRS_TIME_PERIOD*g_nirs_ctx.semple_rate){
        g_nirs_ctx.collect_step = 0;
    }
    if( (g_nirs_ctx.collect_step == RED_ON *g_nirs_ctx.semple_rate )   ||
        (g_nirs_ctx.collect_step == RED_OFF*g_nirs_ctx.semple_rate )   ||
        (g_nirs_ctx.collect_step == IR_ON  *g_nirs_ctx.semple_rate )   ||
        (g_nirs_ctx.collect_step == IR_OFF *g_nirs_ctx.semple_rate )  ){
        nirs_timer_flag = 1;

        int event_data = g_nirs_ctx.collect_step;
        xQueueSendFromISR(xTimerQueue, &event_data, &high_task_awoken);
    }
    //NIRS_Handler();

    return (high_task_awoken == pdTRUE);
}

static void NIRS_Timer_Init(void){
    ESP_LOGI(TAG, "Create timer handle");
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_XTAL,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    
    gptimer_event_callbacks_t cbs = {
        .on_alarm = gptimer_isr_handler, // register user callback
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0, // counter will reload with 0 on alarm event
        .alarm_count = 1000, // period = 1s @resolution 1MHz
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_set_raw_count(gptimer, 0));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

}


void NIRS_Init(void){
    PWM_Capture_Init();
    LED_CTRL_Init();
    LED_PWM_Init();
    // PD_Init();
    NIRS_Timer_Init();

    memset(&g_nirs_ctx, 0, sizeof(NIRS_CONTEXT));
	g_nirs_ctx.state = NIRS_STATE_STOP;
	g_nirs_ctx.collect_state = COLLECT_PHASE_PAUSE;
	/*nirs计算相关参数*/
	g_nirs_ctx.nirs_struct.led[0].wl = 660;
	g_nirs_ctx.nirs_struct.led[0].Hb = 3.4409;
	g_nirs_ctx.nirs_struct.led[0].HbO2 = 0.3346;
	g_nirs_ctx.nirs_struct.led[0].DPF = 6.0;
    g_nirs_ctx.nirs_struct.led[0].rate = 0.8;   //0.8
	
	g_nirs_ctx.nirs_struct.led[1].wl = 905;
	g_nirs_ctx.nirs_struct.led[1].Hb = 0.8944;
	g_nirs_ctx.nirs_struct.led[1].HbO2 = 1.3452;
	g_nirs_ctx.nirs_struct.led[1].DPF = 6.0;
    g_nirs_ctx.nirs_struct.led[1].rate = 0.92;  //0.92
	
	g_nirs_ctx.nirs_struct.pd_len[0] = 3.0;   
	HBCalculateInit(g_nirs_ctx.nirs_struct.paraA, g_nirs_ctx.nirs_struct.paraB);
    //ESP_LOGI(TAG, "NIRS calculate paremeters value: %.2f,%.2f,%.2f,%.2f.", g_nirs_ctx.nirs_struct.paraA[0],g_nirs_ctx.nirs_struct.paraA[1],g_nirs_ctx.nirs_struct.paraA[2],g_nirs_ctx.nirs_struct.paraA[3]);

    nirs_get_sprate();
    g_nirs_ctx.basecount = 10;
    G_nirs_data.nirs_ready_flag = 0;
    memset(G_nirs_data.base, 0, sizeof(G_nirs_data.base)*2);
    memset(G_nirs_data.BaseConcData, 0, sizeof(G_nirs_data.BaseConcData));
    memset(G_nirs_data.ConcData, 0, sizeof(G_nirs_data.ConcData));
    memset(G_nirs_data.SendData, 0, sizeof(G_nirs_data.SendData));
}

/*****************************************************************************
  * Function:	  
  * 		 NIRS_Start
  * Description: 
  * 		 app start nirs sensor
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void NIRS_Start(void){
    ESP_LOGI(TAG, "nirs control start.");
    g_nirs_ctx.state = NIRS_STATE_START;
    medfilter_red = medianFilterInit(7);        //nirs时序控制还存在一定问题，间隔几个采样点会出错，添加中值滤波，可以暂时避免这种现象。
    medfilter_ir = medianFilterInit(7);

    g_nirs_ctx.basecount = 10;
    g_nirs_ctx.collect_step = NIRS_TIME_PERIOD*g_nirs_ctx.semple_rate-1;
    memset((void*)&G_nirs_data, 0, sizeof(G_nirs_data));
    SetPWMDuty(100);
    gpio_set_level(RED_CTRL_PIN, 1);
    gpio_set_level(IR_CTRL_PIN, 0);
    gptimer_start(gptimer);
    gptimer_set_raw_count(gptimer, 0);
    
    pwm_capture_start();
}

/*****************************************************************************
  * Function:	  
  * 		 NIRS_Stop
  * Description: 
  * 		 app stop nirs sensor
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void NIRS_Stop(void){
    g_nirs_ctx.state = NIRS_STATE_STOP;

    medianFilterFree(medfilter_red);
    medianFilterFree(medfilter_ir);
    gptimer_stop(gptimer);
    //gptimer_set_raw_count(gptimer, 0);
    SetPWMDuty(0);
    gpio_set_level(RED_CTRL_PIN, 0);
    gpio_set_level(IR_CTRL_PIN, 0);
    pwm_capture_stop();
    memset(G_nirs_data.BaseConcData, 0, sizeof(G_nirs_data.BaseConcData));
    memset(G_nirs_data.ConcData, 0, sizeof(G_nirs_data.ConcData));
    memset(G_nirs_data.SendData, 0, sizeof(G_nirs_data.SendData));
    ESP_LOGI(TAG, "nirs control stop.");

    endTick = xTaskGetTickCount();
    ESP_LOGI(TAG, "send %d time points data in %.2f seconds, is %.2fHz.", send_count, (endTick-startTick)/1000.0, send_count/((endTick-startTick)/1000.0));
    send_count = 0;
}

/*****************************************************************************
  * Function:	  
  * 		 NIRS_Handler
  * Description: 
  * 		 app handle nirs time 
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void NIRS_Handler(void){
    static int cur_cc = 0;
    if(NIRS_STATE_START != g_nirs_ctx.state){
        ESP_LOGI(TAG, "nirs state wrong, need to be NIRS_STATE_START.");
        return;
    }
    if(g_nirs_ctx.collect_step == RED_ON*g_nirs_ctx.semple_rate)//控制RED发光
    {
        // ESP_LOGI(TAG, "red on");
        gpio_set_level(RED_CTRL_PIN,1);
        pwm_capture_start();
        g_nirs_ctx.collect_state = COLLECT_PHASE_RED;
        if(cur_cc==1){
            cur_cc = 0;
            
            G_nirs_data.nirs_ready_flag = 1;
            
        }
    }
    else if (g_nirs_ctx.collect_step == RED_OFF*g_nirs_ctx.semple_rate)//控制RED关闭
    {
        // ESP_LOGI(TAG, "red off");
        gpio_set_level(RED_CTRL_PIN,0);
        pwm_capture_stop();
        G_nirs_data.RawData[0] = (uint16_t)(G_nirs_data.pwm_cap.value/G_nirs_data.pwm_cap.count);
        G_nirs_data.RawData[0] = medianFilterProcess(medfilter_red, G_nirs_data.RawData[0]);
        g_nirs_ctx.collect_state = COLLECT_PHASE_PAUSE;
    }
    else if (g_nirs_ctx.collect_step == IR_ON*g_nirs_ctx.semple_rate)//控制IR发光
    {
        // ESP_LOGI(TAG, "ir on");
        gpio_set_level(IR_CTRL_PIN,1);
        pwm_capture_start();
        g_nirs_ctx.collect_state = COLLECT_PHASE_IR;
    }
    else if (g_nirs_ctx.collect_step == IR_OFF*g_nirs_ctx.semple_rate)//控制IR关闭
    {
        // ESP_LOGI(TAG, "ir off");
        gpio_set_level(IR_CTRL_PIN,0);
        pwm_capture_stop();
        G_nirs_data.RawData[1] = (uint16_t)(G_nirs_data.pwm_cap.value/G_nirs_data.pwm_cap.count);
    
        G_nirs_data.RawData[1] = medianFilterProcess(medfilter_ir, G_nirs_data.RawData[1]);
        g_nirs_ctx.collect_state = COLLECT_PHASE_PAUSE;
        
    }
    if (g_nirs_ctx.collect_step == IR_OFF*g_nirs_ctx.semple_rate)//data ready
    {
        g_nirs_ctx.collect_state = COLLECT_PHASE_SEND_DATA;
        HBCalculate(G_nirs_data.RawData, G_nirs_data.ConcData, g_nirs_ctx.nirs_struct.paraA, g_nirs_ctx.nirs_struct.paraB);
            if(g_nirs_ctx.basecount){
                ESP_LOGI(TAG, "RED=%d, ir=%d, Hb=%.2f, HbO2=%.2f", G_nirs_data.RawData[0],G_nirs_data.RawData[1], G_nirs_data.ConcData[0], G_nirs_data.ConcData[1]);
                //去除偏差较大的值
                if(fabs(G_nirs_data.ConcData[0])>1000.0 || isnanf(G_nirs_data.ConcData[0])){
                    ESP_LOGI(TAG, "concdata has inf value, hb=%.2f, hbo2=%.2f",G_nirs_data.ConcData[0], G_nirs_data.ConcData[1]);
                    return ;
                }
                for(uint8_t j = 0; j<NIRS_SEND_BUF_NUM; j++){
                    G_nirs_data.base[j].buf[g_nirs_ctx.basecount-1] = G_nirs_data.ConcData[j];
                }
                
                if(1 == g_nirs_ctx.basecount){
                    //计算基线值，需要剔除离群点
                    for(uint8_t j = 0; j<NIRS_SEND_BUF_NUM; j++){
                        G_nirs_data.BaseConcData[j] = RemoveOutPoint(G_nirs_data.base[j].buf);;
                    }
                    ESP_LOGI(TAG, "baseHb=%.2f, baseHbO2=%.2f", G_nirs_data.BaseConcData[0], G_nirs_data.BaseConcData[1]);
                    //暂存好点
                    s_data_cache.intedata[0] = G_nirs_data.RawData[0] ;
                    s_data_cache.intedata[1] = G_nirs_data.RawData[1] ;
                    s_data_cache.ConcData[0] = 0.0f;
                    s_data_cache.ConcData[1] = 0.0f;
                    s_data_cache.ConcData[2] = 0.0f;
                    startTick = xTaskGetTickCount();
                }
                --g_nirs_ctx.basecount;
            }
            else{
                for(uint8_t j = 0; j<NIRS_SEND_BUF_NUM; j++){
                    G_nirs_data.ConcData[j] = G_nirs_data.BaseConcData[j] - G_nirs_data.ConcData[j];
                }
                G_nirs_data.ConcData[2]  = G_nirs_data.ConcData[0] + G_nirs_data.ConcData[1];
                // curtick=xTaskGetTickCount();
                // ESP_LOGI(TAG,"TIME TICK: %ld", curtick-pretick);
                // pretick = curtick;
                ESP_LOGI(TAG, "RED=%d,IR=%d,Hb=%.2f, HbO2=%.2f,tHb=%.2f", G_nirs_data.RawData[0], G_nirs_data.RawData[1], G_nirs_data.ConcData[0], G_nirs_data.ConcData[1], G_nirs_data.ConcData[2]);
                
                if(G_nirs_data.ConcData[0]>1000.0||G_nirs_data.ConcData[1]>1000.0){
                    ESP_LOGI(TAG, "concdata has inf value, hb=%.2f, hbo2=%.2f",G_nirs_data.ConcData[0], G_nirs_data.ConcData[1]);
                    G_nirs_data.RawData[0]  = s_data_cache.intedata[0];
                    G_nirs_data.RawData[1]  = s_data_cache.intedata[1];
                    G_nirs_data.ConcData[0] = s_data_cache.ConcData[0];
                    G_nirs_data.ConcData[1] = s_data_cache.ConcData[1];
                    G_nirs_data.ConcData[2] = s_data_cache.ConcData[2];
                }
                else{
                    s_data_cache.intedata[0] = G_nirs_data.RawData[0] ;
                    s_data_cache.intedata[1] = G_nirs_data.RawData[1] ;
                    s_data_cache.ConcData[0] = G_nirs_data.ConcData[0];
                    s_data_cache.ConcData[1] = G_nirs_data.ConcData[1];
                    s_data_cache.ConcData[2] = G_nirs_data.ConcData[2];   
                }
                send_count++;
                cur_cc = 1;
            }
    }
} 

void nirs_set_sprate(uint8_t spr){
    g_nirs_ctx.semple_rate = spr;
}

uint8_t nirs_get_sprate(void){
    //printf("current nirs sample rate: %d", g_nirs_ctx.semple_rate);
    return g_nirs_ctx.semple_rate;
}

uint8_t nirs_get_ready_flag(void){
    return G_nirs_data.nirs_ready_flag;
}

void nirs_set_ready_flag(uint8_t flag){
    G_nirs_data.nirs_ready_flag = flag;
}

COLLECT_PHASE get_nirs_collect_flag(void){
    return g_nirs_ctx.collect_state;
}
/*************************************************
 * @description			:	nirs计算参数计算
 * @param 				:	无
 * @return 				:	无
**************************************************/
void HBCalculateInit(float* paraA, float* paraB){
	
	//paraA: calculate Hb HbO2
	float wl_dpf[NIRS_SOURCE_NUM][2];
	float AT[2][NIRS_SOURCE_NUM];
	float AMulti[2][2];
	float AInv[2][2];
	float As[2][NIRS_SOURCE_NUM];
	
	for(int i=0; i<NIRS_SOURCE_NUM;i++){
		wl_dpf[i][0] = g_nirs_ctx.nirs_struct.led[i].Hb	  * g_nirs_ctx.nirs_struct.led[i].DPF;
		wl_dpf[i][1] = g_nirs_ctx.nirs_struct.led[i].HbO2 * g_nirs_ctx.nirs_struct.led[i].DPF;
	}
	
	for(int i=0; i<NIRS_SOURCE_NUM;i++){
		for(int j =0;j<2;j++){
			AT[j][i] = wl_dpf[i][j];
		}
	}
	
	for(int i=0;i<2;i++)
	{
		for(int j=0;j<2;j++)
		{
			AMulti[i][j]=0;
			for (int k = 0; k < NIRS_SOURCE_NUM; k++)
			{
				AMulti[i][j] += AT[i][k] * wl_dpf[k][j];
			}
		}
	}
	//paraA = inv(A.T * A)(2x2???)
	float rank_A = AMulti[0][0] * AMulti[1][1] - AMulti[0][1] * AMulti[1][0];
	AInv[0][0] = AMulti[1][1] / rank_A;
	AInv[0][1] = -AMulti[0][1] / rank_A;
	AInv[1][0] = -AMulti[1][0] / rank_A;
	AInv[1][1] = AMulti[0][0] / rank_A;
	//paraA
	for(int i = 0; i < 2; i++)
	{
		for (int j = 0; j < NIRS_SOURCE_NUM; j++)
		{
			As[i][j] = 0;
			for (int k = 0; k < 2; k++)
			{
				As[i][j] =As[i][j] + AInv[i][k] * AT[k][j];
			}
			paraA[i*NIRS_SOURCE_NUM+j] = As[i][j]* 1000;
		}
	}
	//paraB: calculate rso2
	#if (NIRS_DETECTOR_NUM == 2)
    {
		int t = 0;
		float deltaD = g_nirs_ctx.nirs_struct.pd_len[1]-g_nirs_ctx.nirs_struct.pd_len[0];
		for (int i = 0; i < NIRS_SOURCE_NUM-1; i++)
		{
			for (int j = i + 1; j < NIRS_SOURCE_NUM; j++)
			{
				float hb1 =		g_nirs_ctx.nirs_struct.led[i].Hb;
				float hb2 =		g_nirs_ctx.nirs_struct.led[j].Hb;
				float hbo21 =	g_nirs_ctx.nirs_struct.led[i].HbO2;
				float hbo22 =	g_nirs_ctx.nirs_struct.led[j].HbO2;
				paraB[t*5+0] = (hb1 * hbo22 - hb2 * hbo21) / ((hbo22 - hb2) * (hbo22 - hb2));
				paraB[t*5+1] = (hbo21 - hb1) / (hbo22 - hb2);
				paraB[t*5+2] = hb2 / (hbo22 - hb2);
				paraB[t*5+3] = 2 * deltaD / (g_nirs_ctx.nirs_struct.pd_len[i] * lnten);
				paraB[t*5+4] = 2 * deltaD / (g_nirs_ctx.nirs_struct.pd_len[j] * lnten);
				t++;
			}
		}
	}
    #endif
}

/*************************************************
 * @description			:	计算NIRS信号及rso2
 * @param 				:	无
 * @return 				:	无
**************************************************/
void HBCalculate(uint16_t* Data, float* res, float* paraA, float* paraB){
	float IntensityData[NIRS_SOURCE_NUM*NIRS_DETECTOR_NUM];
	uint8_t i;
    memset(res, 0, 16);
	for(i =0;i<NIRS_SOURCE_NUM*NIRS_DETECTOR_NUM; i++){
		IntensityData[i] = (float)log10((double)Data[i]);
	}
	#if(2 == NIRS_DETECTOR_NUM)
    {
		float deltaInte[NIRS_SOURCE_NUM];
		memset(deltaInte, 0, sizeof(float)*NIRS_SOURCE_NUM);
		
		float R[3];
		for(i = 0; i<NIRS_SOURCE_NUM; i++){
			deltaInte[i] = -IntensityData[i*NIRS_DETECTOR_NUM+0]+IntensityData[i*NIRS_DETECTOR_NUM+1];
		}
		#if (NIRS_SOURCE_NUM ==2)
		{
			R[0] = ((deltaInte[0] - paraB[3]) / (deltaInte[1] - paraB[4])) * ((deltaInte[0] - paraB[3]) / (deltaInte[1] - paraB[4]));
			res[2] = (paraB[0] / (R[0] - paraB[1]) - paraB[2]) * 100;
		}
		#elif (NIRS_SOURCE_NUM == 3)
		{
			R[0] = ((deltaInte[0] - paraB[3]) / (deltaInte[1] - paraB[4])) * ((deltaInte[0] - paraB[3]) / (deltaInte[1] - paraB[4]));
			res[2] = (paraB[0] / (R[0] - paraB[1]) - paraB[2]) * 100;

			R[1] = ((deltaInte[0] - paraB[8]) / (deltaInte[2] - paraB[9])) * ((deltaInte[0] - paraB[8]) / (deltaInte[2] - paraB[9]));
			res[2] += (paraB[5] / (R[1] - paraB[6]) - paraB[7]) * 100;
			
			R[2] = ((deltaInte[1] - paraB[13]) / (deltaInte[2] - paraB[14])) * ((deltaInte[1] - paraB[13]) / (deltaInte[2] - paraB[14]));
			res[2] += (paraB[10] / (R[2] - paraB[11]) - paraB[12]) * 100;
			res[2] /= 3;
		}
		#endif
		for(i = 0; i< NIRS_SOURCE_NUM; i++){
			res[0] +=	paraA[i]*deltaInte[i];
			res[1] +=	paraA[i+NIRS_SOURCE_NUM]*deltaInte[i];
		}
	}
	#else
    {
		for(i = 0; i< NIRS_SOURCE_NUM; i++){
			res[0] +=	paraA[i]*IntensityData[i];
			res[1] +=	paraA[i+NIRS_SOURCE_NUM]*IntensityData[i];
		}
        //ESP_LOGI(TAG, "RED=%d,IR=%d, inte_red=%.2f, inte_ir=%.2f, Hb=%.2f, HbO2=%.2f", Data[0], Data[1], IntensityData[0], IntensityData[1], res[0], res[1]);
	}
    #endif
}

