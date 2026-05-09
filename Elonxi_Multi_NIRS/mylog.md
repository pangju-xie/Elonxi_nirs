稳定出现panic:
    Guru Meditation Error: Core  0 panic'ed (StoreProhibited). Exception was unhandled.

    Core  0 register dump:
    PC      : 0x4200d3e8  PS      : 0x00060b30  A0      : 0x8200d2cb  A1      : 0x3fcc8570  
    A2      : 0x000000fe  A3      : 0x3fcc85b0  A4      : 0x00000030  A5      : 0x3fca33dc  
    A6      : 0x3fcac664  A7      : 0x00000030  A8      : 0xc5b60fb6  A9      : 0xbddb07db  
    A10     : 0x00000030  A11     : 0x3fcc8b6c  A12     : 0x07db07db  A13     : 0x07db07db  
    A14     : 0xbdf13684  A15     : 0xbddb07db  SAR     : 0x00000004  EXCCAUSE: 0x0000001d  
    EXCVADDR: 0xc5b60fb6  LBEG    : 0x4200d3b9  LEND    : 0x4200d40a  LCOUNT  : 0x0000002f  


    Backtrace: 0x4200d3e5:0x3fcc8570 0x4200d2c8:0x3fcc85a0 0x4037e0dd:0x3fcc89a0




    ELF file SHA256: 80c9b200

    Rebooting...


对应报错函数：circular_buffer.c  circular_buffer_find

        for (uint32_t j = 0; j < pattern_len; j++) {

            uint32_t pos = (cb->read_pos + i + j) % cb->buffer_size;    此行附近报错
            // 确保位置在有效范围内
            if (pos >= cb->buffer_size) {
                // 这不应该发生，但如果发生则处理

报错时均有j=2

circular_buffer_write_force接收数据后写内存时, 打印printf("write_len: %d, %p, %d, %p, %p, %d\n", len, cb->buffer, cb->buffer_size, cb->read_pos, cb->write_pos, cb->data_len);
发现cb值被修改：write_len: 48, 0x7db07db, 131794907, 0xbdf13684, 0xbddb07db, -1120119671
正常为：write_len: 48, 0x3fca2c5c, 1920, 0x480, 0x480, 0

且报错时均为第12次访问0x480

4.25：
尝试开始采样一段时间后停止，再次开启采样，仍然在第12次访问0x480出现bug
考虑在所有可能的函数执行前后打印被监视变量的值，最终定位异常修改函数:
process_all_frames
-> process_frame
-> memcpy(&g_struct_para.nirs_data[count * data_len], frame_buf+6, data_len);
data_len = 40
由于count始终自增, 因此count足够大时可能会导致后续变量被覆盖

其中一次内存布局:
g_struct_para size=16643, 其中.nirs_data后16640bytes
uart_rx_buf_memory size=1920
uart_rx_buffer
因此count=404时,g_struct_para.nirs_data[count * data_len]指向uart_rx_buffer(cb)，并覆盖对应内容

4.27:
删除isRF相关代码后，
if(count == g_app_var.nirs_dr_pack){
            count = 0;   <---此处未执行
            // udpSendSensorData(TYPE_DATA, DEVICE_TYPE_NIRS_ID);
            sendToUpAppSensor(DEVICE_TYPE_NIRS_ID);
            LED_G_TOGGLE();
            if(g_app_var.nirs_sd_write_flag){
                g_app_var.nirs_sd_write_flag = 0;
                app_sdmmc_write_sectors(g_struct_para.sd_nirs_buffer,NIRS_START_BLOCK+g_app_var.nirs_sd_ready_packcnt, SD_INT); 
            }
        }
导致count不断自增，引起后续数据覆盖bug


5.5:
wifi_udp, wifi_udp_app调用sendto时socket不同

5.8
传感器接串口时可正常连接wifi，不接时无法连接wifi
且启动时使用万用表电压档测量3v3/gnd（正接）也可连接wifi，反接不能

传感器电池接口接触、esp32热点设备同时存在问题