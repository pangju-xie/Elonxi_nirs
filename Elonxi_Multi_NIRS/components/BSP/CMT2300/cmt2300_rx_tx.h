#ifndef _CMT2300_RX_TX_H_
#define _CMT2300_RX_TX_H_

//#include "sys.h"
#include <stdint.h>

extern uint8_t cmt2300_recev_buff[255];

int cmt2300_go_receive( void );
void Cmt2300_Init( void );
int CMT2300_Rece_buff( uint8_t* len );
uint8_t CMT2300_Send_Buff(uint8_t* tx_buffer,uint16_t tx_cnt);


#endif
