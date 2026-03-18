/**
 ****************************************************************************************************
* @file        NIRS.h
* @author      xfw
* @version     V1.0
* @date        2025-5-13
* @brief       nirs config
* @license     ELONXI
****************************************************************************************************
*/
#ifndef __NIRS__H
#define __NIRS__H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdio.h>

#define RED_PWM_PIN     (GPIO_NUM_17)
#define IR_PWM_PIN      (GPIO_NUM_18)
#define RED_CTRL_PIN    (GPIO_NUM_19)
#define IR_CTRL_PIN     (GPIO_NUM_20)
#define PD_PIN          (GPIO_NUM_2)

#define NIRS_OPEN_TIME      4
#define NIRS_PWM_TIME       5

//频率100：50：25：12.5Hz，//10,20,40,80ms
#define RED_ON              0 
#define RED_OFF             (NIRS_OPEN_TIME)
#define IR_ON               (NIRS_PWM_TIME)
#define IR_OFF              (NIRS_OPEN_TIME+NIRS_PWM_TIME)
#define NIRS_OFF            (NIRS_PWM_TIME*2)

#define NIRS_TIME_PERIOD    (2*NIRS_PWM_TIME)

//PWM配置
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (8192) // Set duty to 100%. (2 ** 13) * 100% = 8192
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

#define NIRS_SOURCE_NUM     	2
#define NIRS_DETECTOR_NUM   	1
#define NIRS_SEND_BUF_NUM   	2
#define NIRS_DEFAULT_BASE_NUM 	10
#define NIRS_DATA_LEN			512
#define NIRS_SD_INT				10
#define SD_SECTOR_MUL			1
#define TH_LIGHT_LEAKOFF		150		//环境光污染阈值

typedef enum {
	NIRS_STATE_STOP = 0,
	NIRS_STATE_START = 1,
} NIRS_STATE;

typedef enum {
	COLLECT_PHASE_PAUSE = 0,
	COLLECT_PHASE_RED = 1,
	COLLECT_PHASE_IR = 2,
	COLLECT_PHASE_SEND_DATA = 3,
}COLLECT_PHASE;

typedef struct _NIRS_SOURCE{
    uint16_t wl;
	float Hb;
	float HbO2;
	float DPF;
	float rate;
}NIRS_SOURCE;

typedef struct _NIRS_STRUCT
{
	NIRS_SOURCE led[NIRS_SOURCE_NUM];
	float pd_len[NIRS_DETECTOR_NUM];
	float paraA[2*NIRS_SOURCE_NUM];
	float paraB[5*NIRS_SOURCE_NUM];
}NIRS_STRUCT;

typedef enum{
	SR_100 = 1,
	SR_50  = 2,
	SR_20  = 3,
	SR_10  = 4,
}NIRS_SR;

typedef struct{
	int count;
	uint32_t value;
}G_MEAN_CAL;

typedef struct _NIRS_CONTEXT
{
	volatile uint8_t state;
	volatile uint8_t collect_state;
	volatile uint32_t collect_step;
	int basecount;
	NIRS_STRUCT nirs_struct;
	NIRS_SR semple_rate;
}NIRS_CONTEXT;

typedef struct{
	float mean;
	float std;
	float buf[10];
}OUTPOINT;


typedef struct{
	uint8_t nirs_light_leakoff_flag;
	uint8_t nirs_ready_flag;
	G_MEAN_CAL pwm_cap;
	uint16_t RawData[NIRS_SOURCE_NUM*NIRS_DETECTOR_NUM];
	OUTPOINT base[2];
	float BaseConcData[4];
	float ConcData[4];
	uint8_t SendData[NIRS_DATA_LEN];
	uint8_t sd_buffer[NIRS_DATA_LEN*NIRS_SD_INT]	__attribute__((aligned(4)));
    uint8_t imu_sd_buffer[512*10]         			__attribute__((aligned(4)));
}NIRS_DATA;

typedef struct{
	uint8_t intedata[NIRS_SOURCE_NUM*NIRS_DETECTOR_NUM];
	float ConcData[4];
}NIRS_DATA_CACHE;


extern NIRS_DATA G_nirs_data;
extern uint8_t nirs_flag;
extern uint8_t bat_flag;
extern NIRS_CONTEXT g_nirs_ctx;

extern QueueHandle_t xTimerQueue;
extern volatile uint8_t nirs_timer_flag;

void NIRS_Init(void);
void NIRS_Start(void);
void NIRS_Stop(void);
void NIRS_Handler(void);
uint64_t GetTimerCount(void);
COLLECT_PHASE get_nirs_collect_flag(void);

void nirs_set_sprate(uint8_t spr);
uint8_t nirs_get_sprate(void);

uint8_t nirs_get_ready_flag(void);
void nirs_set_ready_flag(uint8_t flag);

void HBCalculateInit(float* paraA, float* paraB);
void HBCalculate(uint16_t* Data, float* res, float* paraA, float* paraB);

#ifdef __cplusplus
}
#endif
#endif

