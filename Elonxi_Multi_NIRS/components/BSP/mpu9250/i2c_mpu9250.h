/**
 ****************************************************************************************************
* @file        i2c_mpu9250.h
* @author      zz
* @date        2025-03-05
* @brief       led
* @license     ELONXI
****************************************************************************************************
*/
#ifndef __I2C_MPU9250_H
#define __I2C_MPU9250_H

#define  IMU_START_BLOCK       (6000000)
#define  IMU_SD_BASE_LEN       (512) 
#define  IMU_SD_INT	           (10)
#define  IMU_BLOCK_LEN         (5)

#define IMU_PACK_LEN  200

extern uint32_t g_imu_packcnt;
extern uint8_t  g_imu_send_flag;

/*struct*/
typedef struct
{
    //uint8_t inm_read_data[200];
    uint8_t imu_data[IMU_PACK_LEN];
}IMU_PARA;


extern  IMU_PARA  g_struct_imu_para;

void app_mpu9250_task(void);
void SetIMU(uint8_t flag);


#endif