#ifndef _UART_H
#define _UART_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdio.h"

#define TXD_PIN         (GPIO_NUM_17)
#define RXD_PIN         (GPIO_NUM_18)

#define UART_NUM        UART_NUM_1

#define SEND_HEADER           (0x89ABCDEF)
#define RECV_HEADER           (0xFEDABC98)

#define NIRS_START_BLOCK    (4000000)
#define NIRS_DATA_LEN       256
#define EMG_DATA_LEN        1024
#define SD_BASE_LEN         1024
#define SD_INT              10
#define SD_SECTOR_NUL       2

typedef struct{
    uint8_t nirs_send_flag;
    uint8_t emg_send_flag;
    uint8_t nirs_data[NIRS_DATA_LEN];
    uint8_t emg_data[EMG_DATA_LEN];
    uint8_t sd_nirs_buffer[SD_BASE_LEN*SD_INT];     __attribute__((aligned(4)));
    uint8_t sd_emg_buffer[SD_BASE_LEN*SD_INT];      __attribute__((aligned(4)));
    uint8_t imu_sd_buffer[512*10];                  __attribute__((aligned(4)));
}G_SENSOR_BUF;

extern G_SENSOR_BUF g_struct_para;

// 帧协议定义
#define UART_RX_BUF_SIZE    2048        // UART接收缓冲区大小
#define UART_TEMP_BUF_SIZE  1024        // UART临时读取缓冲区大小
void uart_init(void);
void uart_rx_task(void *arg);
void uart_tx_task(uint8_t *data, int len);
uint16_t CRC16Calculate(uint8_t* data, uint16_t len);

void send_cmd_to_stm32(uint8_t cmd, uint8_t data);


#ifdef __cplusplus
}
#endif

#endif 
