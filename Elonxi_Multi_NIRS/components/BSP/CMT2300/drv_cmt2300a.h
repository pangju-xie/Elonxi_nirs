#ifndef __DRV_CMT2300A_H
#define __DRV_CMT2300A_H

#include "typedefs.h"
#include "driver/gpio.h"
#include <stdint.h>


#define CMT2300A_GPIO1_PIN						GPIO_NUM_26

#define CMT2300A_GPIO2_PIN						GPIO_NUM_21

//#define CMT2300A_GPIO3_PIN						GPIO_NUM_16

#define CMT2300A_SPI_CSB_PIN					GPIO_NUM_33

#define CMT2300A_SPI_SCLK_PIN					GPIO_NUM_48

#define CMT2300A_SPI_MOSI_PIN					GPIO_NUM_34

#define CMT2300A_SPI_MISO_PIN					GPIO_NUM_46

#define CMT2300A_SPI_FCSB_PIN					GPIO_NUM_47

uint8_t CMT2300A_ReadGpio1(void);
uint8_t CMT2300A_ReadGpio2(void);
uint8_t CMT2300A_ReadGpio3(void);

void CMT2300A_InitGpio(void);
u8 CMT2300A_ReadReg(u8 addr);
void CMT2300A_WriteReg(u8 addr, u8 dat);

void CMT2300A_ReadFifo(u8 buf[], u16 len);
void CMT2300A_WriteFifo(const u8 buf[], u16 len);

#endif


