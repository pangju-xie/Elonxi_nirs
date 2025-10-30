/**
 ****************************************************************************************************
* @file        wifi_data.c
* @author      zz
* @version     V1.0
* @date        2024-09-12
* @brief       WIFI UDP
* @license     ELONXI
****************************************************************************************************
*/


#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "lwip/err.h"   
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "app.h"
#include "wifi_data.h"
#include "uart.h"
#include "adc1.h"
#include "sdmmc.h"





/********************************************************************************************/
/*数据CRC16校验
**参数:
pDataIn	:数据
iLenIn	:数据长度
**返回值:	校验结果
*/
/********************************************************************************************/

static unsigned short const crc16Table[256] = { //crc16 校验码列表
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 };

uint16_t CRC16(const uint8_t* pData, uint32_t uLen)
{
	uint16_t result = 0;
	uint16_t tableNo = 0;
	int i;

	if (!pData || !uLen) {
		return 0;
	}
	for (i = 0; i < uLen; i++) {
		tableNo = ((result & 0xff) ^ (pData[i] & 0xff));
		result = ((result >> 8) & 0xff) ^ crc16Table[tableNo];
	}
	return result;
}


/*****************************************************************************
  * Function:	  
  * 		 packetSendMessage
  * Description: 
  * 		 send message packet
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
int packetSendMessage(uint8_t *data,uint8_t *sn, uint8_t systype, uint8_t synFlag)
{
    uint16_t crc=0;
    uint8_t i=0;
    //head
	data[i++] = 0x5A;
    //length
	data[i++] = 0x00;
    data[i++] = 0x13;
    //cmd
    data[i++] = systype;

     //flag
    if(synFlag == 1){
        data[i++] = 0;
        data[i++] = 0;
    }else{
        data[i++] = 0;
        data[i++] = 1;
    }   

    //sn
    memcpy(data+i, sn, 8);
    i += 8;
    
    //channel Type 有多少种类型的数据
	data[i++] = 0x01;

    if(systype == DEVICE_TYPE_NIRS_ID){
        //sample channel
        data[i++] = NIRS_CHANNEL;
        //sample byte
        data[i++] = NIRS_BYTES;
        //sample rate
        data[i++] = (g_app_var.nirs_dr_pack>>8)&0xFF;
        data[i++] = g_app_var.nirs_dr_pack&0xFF;
    }  
    else if(systype == DEVICE_TYPE_EMG_ID){
        //sample channel
        data[i++] = SAMPLE_CHANNEL;
        //sample byte
        data[i++] = SAMPLE_BYTES;
        //sample rate
        data[i++] = (g_app_var.emg_dr_pack>>8)&0xFF;
        data[i++] = g_app_var.emg_dr_pack&0xFF;
    }

    crc = CRC16(data, 0x13);
    data[i++] = (crc >> 8)&0xFF;
	data[i++] = crc&0xFF;
  
    //udp_send_data(data, 21);
    return i;	
}


/*****************************************************************************
  * Function:	  
  * 		 packetSendData
  * Description: 
  * 		 send data packet
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
int packetSendData(uint8_t *data, uint8_t *src,uint8_t sentype, uint16_t length, uint32_t stamp)
{
    uint16_t i=0;
    uint16_t crc=0;

#if 0

#else
    //head
    data[i++] = 0x5A; 

    //length
    data[i++] = 0x0;
    data[i++] = 0x0;

    //cmd
    data[i++] = sentype +1; 

    //mark
    data[i++] = 0;//(sDb.seq>>8)&0xFF;
    data[i++] = 0x01;//sDb.seq&0xFF;

    //sn
    memcpy(data+i, g_app_var.serialNumber, 8);
    i += 8;

    data[i++] = (stamp >> 24)&0xFF;
	data[i++] = (stamp >> 16)&0xFF;
	data[i++] = (stamp >> 8)&0xFF;
	data[i++] = stamp&0xFF;

    //interval
    data[i++] = 0x01; //ms <= 200ms

    //data
    memcpy(data+i, src, length);
    i += length;

    //printf("packet data= %d %d %d\r\n",data[2418],data[2419], data[2420]);

    //modify length
    data[1] = (i>> 8)&0xFF;
    data[2] = i&0xFF;

    //crc
    crc = CRC16(data, i);
    data[i++] = (crc >> 8)&0xFF;
	data[i++] = crc&0xFF;

    //printf("packet ii= %d\r\n",i);
    //send data
    return i;	
#endif
}



/*****************************************************************************
  * Function:	  
  * 		 udpSendSensorData
  * Description: 
  * 		 send sensor data for udp
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
int udpSendSensorData(uint8_t type, uint8_t sentype)
{
    //uint8_t data[1024*25]={0};
	int len=0;
    uint16_t len_data = 0;
    //memset(sDb.payload, 0, 1024*25);
    if(type == 0){
        len = packetSendMessage(g_app_var.payload,g_app_var.serialNumber, sentype, g_app_var.synFlag);
        // printf("print syn data: ");
        // for(int i = 0;i<len;i++){
        //     printf("%02x ",g_app_var.payload[i]);
        // }
        // printf(".\r\n");
        g_app_var.synFlag = 0;     
    }
    else
    {
        if(sentype == DEVICE_TYPE_NIRS_ID){
            len_data= NIRS_BYTES*NIRS_CHANNEL*g_app_var.nirs_dr_pack;
            //for sensor data 
            len = packetSendData(g_app_var.payload, g_struct_para.nirs_data, sentype, len_data, g_app_var.nirs_packet_counter);

            //memcpy(&g_struct_para.sd_buffer[SD_BASE_LEN*g_app_var.sd_count],g_struct_para.send_data,SD_BASE_LEN);
            memcpy(&g_struct_para.sd_nirs_buffer[NIRS_DATA_LEN*g_app_var.nirs_sd_count],g_app_var.payload,NIRS_DATA_LEN);
            g_app_var.nirs_sd_count++;   

            if(g_app_var.nirs_sd_count >= SD_INT)
            {
                g_app_var.nirs_sd_count = 0;
                g_app_var.nirs_sd_ready_packcnt = g_app_var.nirs_packet_counter - SD_INT + 1;
                g_app_var.nirs_sd_write_flag = 1;
            }
            if(g_app_var.rf_syn_packcnt > 0)
            {
                // if((g_app_var.nirs_packet_counter > g_app_var.rf_syn_packcnt) || (g_app_var.nirs_packet_counter < g_app_var.rf_syn_packcnt))
                // {
                //     printf("[%ld syn = %ld]\r\n",g_app_var.nirs_packet_counter,g_app_var.rf_syn_packcnt);
                //     g_app_var.nirs_packet_counter = g_app_var.rf_syn_packcnt - 1;
                //     g_app_var.rf_syn_packcnt = 0;
                // }
            }
            g_app_var.nirs_packet_counter++;    
        }
        else{
            len_data = SAMPLE_BYTES*SAMPLE_CHANNEL*g_app_var.emg_dr_pack;
            //for sensor data 
            len = packetSendData(g_app_var.payload, g_struct_para.nirs_data, sentype, len_data, g_app_var.emg_packet_counter);

            //memcpy(&g_struct_para.sd_buffer[SD_BASE_LEN*g_app_var.sd_count],g_struct_para.send_data,SD_BASE_LEN);
            memcpy(&g_struct_para.sd_emg_buffer[SD_BASE_LEN*g_app_var.emg_sd_count],g_app_var.payload,SD_BASE_LEN);
            g_app_var.emg_sd_count++;   

            if(g_app_var.emg_sd_count >= SD_INT)
            {
                g_app_var.emg_sd_count = 0;
                g_app_var.emg_sd_ready_packcnt = g_app_var.emg_packet_counter - SD_INT + 1;
                g_app_var.emg_sd_write_flag = 1;
            }
            if(g_app_var.rf_syn_packcnt > 0)
            {
                // if((g_app_var.emg_packet_counter > g_app_var.rf_syn_packcnt) || (g_app_var.emg_packet_counter < g_app_var.rf_syn_packcnt))
                // {
                //     printf("[%ld syn = %ld]\r\n",g_app_var.emg_packet_counter,g_app_var.rf_syn_packcnt);
                //     g_app_var.emg_packet_counter = g_app_var.rf_syn_packcnt - 1;
                //     g_app_var.rf_syn_packcnt = 0;
                // }
            }
            g_app_var.emg_packet_counter++;
        }
        
    }

    //printf("packet len= %d\r\n",len);
    udp_send_data(g_app_var.payload, len);

	return len;	
}

