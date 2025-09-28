#include <string.h>
#include "cmt2300_rx_tx.h"
#include "radio.h"
#include "esp_log.h"

/*-----------------------------------------------------------------------------
功能：CMT2300进入接收状态,并清空FIFO
返回:
	TRUE:成功
	FALSE:失败
------------------------------------------------------------------------------*/
int cmt2300_go_receive( void )
{
	CMT2300A_GoStby();
	CMT2300A_ClearInterruptFlags();

	/* Must clear FIFO after enable SPI to read or write the FIFO */
	CMT2300A_EnableReadFifo();
	CMT2300A_ClearRxFifo();

	if( FALSE==CMT2300A_GoRx() )
		return FALSE;	
	
	return TRUE;
}

/*----Cmt2300 应用初始化----*/
void Cmt2300_Init( void )
{
	RF_Init();
	if( FALSE == CMT2300A_IsExist() ) 
	{
        //USART_SendString( USART1 , "CMT2300A not found!\r\n");		//CMT2300故障闪烁，一直
		ESP_LOGI("CMT2300A", "CMT2300A not found!" );
    }
    else 
	{
        //USART_SendString( USART1 ,"CMT2300A ready\r\n");
		ESP_LOGI("CMT2300A", "CMT2300A ready" );
		CMT2300A_GoStby();
		//cmt2300_go_receive();			                        //进入接收模式
		ESP_LOGI("CMT2300A", "CMT2300A ready=%d" ,cmt2300_go_receive());
    }
}

 /*------------------------------------------------------------------------------
功能：CMT2300 发送缓冲中的一串数据
参数:
(in)* tx_buffer:	要发送的数据缓存
(in)tx_cnt:			缓存中数据的个数
返回:	FALSE	；	发送失败
		TRUE	：	发送成功
-------------------------------------------------------------------------------*/
uint8_t CMT2300_Send_Buff(uint8_t* tx_buffer,uint16_t tx_cnt)
{		
	if(tx_cnt == 0)
		return FALSE;

	CMT2300A_GoStby();
	CMT2300A_ClearInterruptFlags();
	
	/* Must clear FIFO after enable SPI to read or write the FIFO */
	CMT2300A_EnableWriteFifo();
	CMT2300A_ClearTxFifo();
	
	CMT2300A_WriteReg(0x46,tx_cnt);
	/* The length need be smaller than 32 */
	CMT2300A_WriteFifo(tx_buffer, tx_cnt);
	
	if( 0==(CMT2300A_MASK_TX_FIFO_NMTY_FLG & CMT2300A_ReadReg(CMT2300A_CUS_FIFO_FLAG)) )
	{
		return FALSE;
	}
	if(FALSE==CMT2300A_GoTx())
	{
		return FALSE;
	}

	while(1)
	{
		if(CMT2300A_MASK_TX_DONE_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_CLR1))
//		if(CMT2300A_ReadGpio1())  /* Read INT1, TX_DONE */
		{
			CMT2300A_ClearInterruptFlags();
			CMT2300A_GoSleep();
			return TRUE;
		}
	}
}

uint8_t cmt2300_recev_buff[255]={0};
/*------------------------------------------------------------------------------
功能：CMT2300 接收一串数据,将数据存在CMT2300的接收缓存中
参数:
(out):CMT2300接收到的数据个数
返回:	FALSE	；	接收失败
		TRUE	：	接收成功
-------------------------------------------------------------------------------*/
int CMT2300_Rece_buff( uint8_t* len )
{	
	uint8_t len_data = 0;
	//if(CMT2300A_ReadGpio2())  /* Read INT2, PKT_OK */
	if( CMT2300A_MASK_PKT_OK_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG) )
	{
		CMT2300A_GoStby();
		memset( cmt2300_recev_buff , 0  , sizeof(cmt2300_recev_buff));
		
		CMT2300A_ReadFifo( len, 1);							/* The length need be smaller than 32 */
		CMT2300A_ReadFifo(cmt2300_recev_buff, *len);
		
		CMT2300A_ClearInterruptFlags();
		CMT2300A_GoSleep();
		
		cmt2300_go_receive();					//重新进入发送模式
		
		//printf("%s" , cmt2300_recev_buff);
		//ESP_LOGI("CMT2300A", "revice=%02x %02x %02x %02x %02x",cmt2300_recev_buff[0],cmt2300_recev_buff[1],cmt2300_recev_buff[2],cmt2300_recev_buff[3],cmt2300_recev_buff[4]);
		len_data = cmt2300_recev_buff[1] + 2;

		printf("CMT2300A , revice=");
		for(uint8_t i = 0; i < len_data; i++)
		{
			printf("%02X ",cmt2300_recev_buff[i]);
		}
		printf("\r\n");
		return TRUE;
	}
	else
		return FALSE;							//没有接收到数据
}








