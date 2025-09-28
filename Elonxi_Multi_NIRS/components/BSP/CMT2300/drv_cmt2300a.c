#include "drv_cmt2300a.h"
#include "led.h"
#include "driver/gpio.h"

void cmt_spi3_delay(void)
{
//    u32 n = 7;
//    while(n--);
}

void cmt_spi3_delay_us(void)
{
//    u16 n = 8;
//    while(n--);
}

void cmt_spi3_csb_out(void)
{
    gpio_config_t gpio_csb_conf = {0};

    gpio_csb_conf.intr_type = GPIO_INTR_DISABLE;          /* 失能引脚中断 */
    gpio_csb_conf.mode = GPIO_MODE_OUTPUT;          /* 输入输出模式 */
    gpio_csb_conf.pull_up_en = GPIO_PULLUP_DISABLE;       /* 失能上拉 */
    gpio_csb_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   /* 失能下拉 */
    gpio_csb_conf.pin_bit_mask = 1ull << CMT2300A_SPI_CSB_PIN;       /* 设置的引脚的位掩码 */
    gpio_config(&gpio_csb_conf);                          /* 配置GPIO */
  
}

void cmt_spi3_fcsb_out(void)
{
    gpio_config_t gpio_fcsb_conf = {0};

    gpio_fcsb_conf.intr_type = GPIO_INTR_DISABLE;          
    gpio_fcsb_conf.mode = GPIO_MODE_OUTPUT;          
    gpio_fcsb_conf.pull_up_en = GPIO_PULLUP_DISABLE;       
    gpio_fcsb_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   
    gpio_fcsb_conf.pin_bit_mask = 1ull << CMT2300A_SPI_FCSB_PIN;      
    gpio_config(&gpio_fcsb_conf);
}

void cmt_spi3_sclk_out(void)
{
    gpio_config_t gpio_sclk_conf = {0};

    gpio_sclk_conf.intr_type = GPIO_INTR_DISABLE;          
    gpio_sclk_conf.mode = GPIO_MODE_OUTPUT;          
    gpio_sclk_conf.pull_up_en = GPIO_PULLUP_DISABLE;       
    gpio_sclk_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   
    gpio_sclk_conf.pin_bit_mask = 1ull << CMT2300A_SPI_SCLK_PIN;      
    gpio_config(&gpio_sclk_conf);
}

void cmt_spi3_sdio_out(void)
{
    gpio_config_t gpio_sdio_o_conf = {0};

    gpio_sdio_o_conf.intr_type = GPIO_INTR_DISABLE;          
    gpio_sdio_o_conf.mode = GPIO_MODE_OUTPUT;          
    gpio_sdio_o_conf.pull_up_en = GPIO_PULLUP_DISABLE;       
    gpio_sdio_o_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   
    gpio_sdio_o_conf.pin_bit_mask = 1ull << CMT2300A_SPI_MOSI_PIN;      
    gpio_config(&gpio_sdio_o_conf);
}

void cmt_spi3_sdio_in(void)
{
    gpio_config_t gpio_sdio_i_conf = {0};

    gpio_sdio_i_conf.intr_type = GPIO_INTR_DISABLE;          
    gpio_sdio_i_conf.mode = GPIO_MODE_INPUT;          
    gpio_sdio_i_conf.pull_up_en = GPIO_PULLUP_DISABLE;       
    gpio_sdio_i_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   
    gpio_sdio_i_conf.pin_bit_mask = 1ull << CMT2300A_SPI_MOSI_PIN;      
    gpio_config(&gpio_sdio_i_conf);
}

void cmt_spi3_csb_1(void)
{
    gpio_set_level(CMT2300A_SPI_CSB_PIN, PIN_SET);
}

void cmt_spi3_csb_0(void)
{
    gpio_set_level(CMT2300A_SPI_CSB_PIN, PIN_RESET);
}

void cmt_spi3_fcsb_1(void)       
{
    gpio_set_level(CMT2300A_SPI_FCSB_PIN, PIN_SET);
}

void cmt_spi3_fcsb_0(void)
{
    gpio_set_level(CMT2300A_SPI_FCSB_PIN, PIN_RESET);
}

void cmt_spi3_sclk_1(void)
{
    gpio_set_level(CMT2300A_SPI_SCLK_PIN, PIN_SET);
}

void cmt_spi3_sclk_0(void)
{
    gpio_set_level(CMT2300A_SPI_SCLK_PIN, PIN_RESET);
}

void cmt_spi3_sdio_1(void)
{
	gpio_set_level(CMT2300A_SPI_MOSI_PIN, PIN_SET);
}

void cmt_spi3_sdio_0(void)
{
	gpio_set_level(CMT2300A_SPI_MOSI_PIN, PIN_RESET);
}

uint8_t cmt_spi3_sdio_read(void)
{
    return gpio_get_level(CMT2300A_SPI_MOSI_PIN);
}

uint8_t CMT2300A_ReadGpio1(void)
{
    return gpio_get_level(CMT2300A_GPIO1_PIN);
}

uint8_t CMT2300A_ReadGpio2(void)
{
    return gpio_get_level(CMT2300A_GPIO2_PIN);
}


void cmt_spi3_init(void)
{
    cmt_spi3_csb_1();
    cmt_spi3_csb_out();
    cmt_spi3_csb_1();   /* CSB has an internal pull-up resistor */
    
    cmt_spi3_sclk_0();
    cmt_spi3_sclk_out();
    cmt_spi3_sclk_0();   /* SCLK has an internal pull-down resistor */
    
    cmt_spi3_sdio_1();
    cmt_spi3_sdio_out();
    cmt_spi3_sdio_1();
    
    cmt_spi3_fcsb_1();
    cmt_spi3_fcsb_out();
    cmt_spi3_fcsb_1();  /* FCSB has an internal pull-up resistor */

    cmt_spi3_delay();
}

void cmt_spi3_send(u8 data8)
{
    u8 i;

    for(i=0; i<8; i++)
    {
        cmt_spi3_sclk_0();

        /* Send byte on the rising edge of SCLK */
        if(data8 & 0x80)
            cmt_spi3_sdio_1();
        else            
            cmt_spi3_sdio_0();

        cmt_spi3_delay();

        data8 <<= 1;
        cmt_spi3_sclk_1();
        cmt_spi3_delay();
    }
}

u8 cmt_spi3_recv(void)
{
    u8 i;
    u8 data8 = 0xFF;

    for(i=0; i<8; i++)
    {
        cmt_spi3_sclk_0();
        cmt_spi3_delay();
        data8 <<= 1;

        cmt_spi3_sclk_1();

        /* Read byte on the rising edge of SCLK */
        if(cmt_spi3_sdio_read())
            data8 |= 0x01;
        else
            data8 &= ~0x01;

        cmt_spi3_delay();
    }

    return data8;
}

void cmt_spi3_write(u8 addr, u8 dat)
{
    cmt_spi3_sdio_1();
    cmt_spi3_sdio_out();

    cmt_spi3_sclk_0();
    cmt_spi3_sclk_out();
    cmt_spi3_sclk_0(); 

    cmt_spi3_fcsb_1();
    cmt_spi3_fcsb_out();
    cmt_spi3_fcsb_1();

    cmt_spi3_csb_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    /* r/w = 0 */
    cmt_spi3_send(addr&0x7F);

    cmt_spi3_send(dat);

    cmt_spi3_sclk_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    cmt_spi3_csb_1();
    
    cmt_spi3_sdio_1();
    cmt_spi3_sdio_in();
    
    cmt_spi3_fcsb_1();    
}

void cmt_spi3_read(u8 addr, u8* p_dat)
{
    cmt_spi3_sdio_1();
    cmt_spi3_sdio_out();

    cmt_spi3_sclk_0();
    cmt_spi3_sclk_out();
    cmt_spi3_sclk_0(); 

    cmt_spi3_fcsb_1();
    cmt_spi3_fcsb_out();
    cmt_spi3_fcsb_1();

    cmt_spi3_csb_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    /* r/w = 1 */
    cmt_spi3_send(addr|0x80);

    /* Must set SDIO to input before the falling edge of SCLK */
    cmt_spi3_sdio_in();
    
    *p_dat = cmt_spi3_recv();

    cmt_spi3_sclk_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    cmt_spi3_csb_1();
    
    cmt_spi3_sdio_1();
    cmt_spi3_sdio_in();
    
    cmt_spi3_fcsb_1();
}

void cmt_spi3_write_fifo(const u8* p_buf, u16 len)
{
    u16 i;

    cmt_spi3_fcsb_1();
    cmt_spi3_fcsb_out();
    cmt_spi3_fcsb_1();

    cmt_spi3_csb_1();
    cmt_spi3_csb_out();
    cmt_spi3_csb_1();

    cmt_spi3_sclk_0();
    cmt_spi3_sclk_out();
    cmt_spi3_sclk_0();

    cmt_spi3_sdio_out();

    for(i=0; i<len; i++)
    {
        cmt_spi3_fcsb_0();

        /* > 1 SCLK cycle */
        cmt_spi3_delay();
        cmt_spi3_delay();

        cmt_spi3_send(p_buf[i]);

        cmt_spi3_sclk_0();

        /* > 2 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();

        cmt_spi3_fcsb_1();

        /* > 4 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
    }

    cmt_spi3_sdio_in();
    
    cmt_spi3_fcsb_1();
}

void cmt_spi3_read_fifo(u8* p_buf, u16 len)
{
    u16 i;

    cmt_spi3_fcsb_1();
    cmt_spi3_fcsb_out();
    cmt_spi3_fcsb_1();

    cmt_spi3_csb_1();
    cmt_spi3_csb_out();
    cmt_spi3_csb_1();

    cmt_spi3_sclk_0();
    cmt_spi3_sclk_out();
    cmt_spi3_sclk_0();

    cmt_spi3_sdio_in();

    for(i=0; i<len; i++)
    {
        cmt_spi3_fcsb_0();

        /* > 1 SCLK cycle */
        cmt_spi3_delay();
        cmt_spi3_delay();

        p_buf[i] = cmt_spi3_recv();

        cmt_spi3_sclk_0();

        /* > 2 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();

        cmt_spi3_fcsb_1();

        /* > 4 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
    }

    cmt_spi3_sdio_in();
    
    cmt_spi3_fcsb_1();
}

void CMT2300A_InitGpio(void)
{
    gpio_config_t gpio_conf = {0};

    gpio_conf.intr_type = GPIO_INTR_DISABLE;          
    gpio_conf.mode = GPIO_MODE_INPUT;          
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;       
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   
    gpio_conf.pin_bit_mask = (1ull << CMT2300A_SPI_MISO_PIN)|(1ull << CMT2300A_GPIO2_PIN);      
    gpio_config(&gpio_conf);
	
	cmt_spi3_init();
}

/*! ********************************************************
* @name    CMT2300A_ReadReg
* @desc    Read the CMT2300A register at the specified address.
* @param   addr: register address
* @return  Register value
* *********************************************************/
u8 CMT2300A_ReadReg(u8 addr)
{
    u8 dat = 0xFF;
    cmt_spi3_read(addr, &dat);
	
    return dat;
}

/*! ********************************************************
* @name    CMT2300A_WriteReg
* @desc    Write the CMT2300A register at the specified address.
* @param   addr: register address
*          dat: register value
* *********************************************************/
void CMT2300A_WriteReg(u8 addr, u8 dat)
{
    cmt_spi3_write(addr, dat);
}

/*! ********************************************************
* @name    CMT2300A_ReadFifo
* @desc    Reads the contents of the CMT2300A FIFO.
* @param   buf: buffer where to copy the FIFO read data
*          len: number of bytes to be read from the FIFO
* *********************************************************/
void CMT2300A_ReadFifo(u8 buf[], u16 len)
{
    cmt_spi3_read_fifo(buf, len);
}

/*! ********************************************************
* @name    CMT2300A_WriteFifo
* @desc    Writes the buffer contents to the CMT2300A FIFO.
* @param   buf: buffer containing data to be put on the FIFO
*          len: number of bytes to be written to the FIFO
* *********************************************************/
void CMT2300A_WriteFifo(const u8 buf[], u16 len)
{
    cmt_spi3_write_fifo(buf, len);
}
