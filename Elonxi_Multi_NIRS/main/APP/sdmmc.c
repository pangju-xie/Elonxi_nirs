/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sdmmc.h"
#include "uart.h"
#include "i2c_mpu9250.h"

#define EXAMPLE_MAX_CHAR_SIZE    64

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

#define CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
//#define CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
//#define CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

#define CONFIG_EXAMPLE_PIN_CLK GPIO_NUM_9
#define CONFIG_EXAMPLE_PIN_CMD GPIO_NUM_12
#define CONFIG_EXAMPLE_PIN_D0  GPIO_NUM_13
#define CONFIG_EXAMPLE_PIN_D1  GPIO_NUM_14
#define CONFIG_EXAMPLE_PIN_D2  GPIO_NUM_11
#define CONFIG_EXAMPLE_PIN_D3  GPIO_NUM_10


void app_sdmmc_read_sectors_test(void);


sdmmc_card_t *card;

void sd_card_init(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Set bus width to use:
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.width = 4;
#else
    slot_config.width = 1;
#endif

    // On chips where the GPIOs used for SD card can be configured, set them in
    // the slot_config structure:
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
    slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
    slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
#endif  // CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
#endif  // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;


    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
}

int app_sdmmc_read_sectors_safe(uint8_t *buffer, uint32_t pack_num, uint8_t data_type){
    esp_err_t ret;
    uint32_t read_pack_num = 0;
    uint32_t pack_baise = pack_num;
    uint8_t count = 8;
    while(--count){
        ret = app_sdmmc_read_sectors(buffer, pack_baise, data_type);
        read_pack_num = (buffer[14] << 24) | (buffer[15] << 16) | (buffer[16] << 8) | (buffer[17]);
        if(read_pack_num != pack_num){
            ESP_LOGE(TAG, "Failed to read packet:%ld in sector, try to get real packet in %ld, but get packet:%ld,  tried %d times...", pack_num, pack_baise, read_pack_num, 8-count);
            pack_baise = 2 * pack_num - read_pack_num;
        }
        else{
            break;
        }
    }
    if(!count){
        ESP_LOGE(TAG,"Failed to read packet %ld in sector.", pack_num);
        ret = -1;
    }
    return ret;
}


/*****************************************************************************
  * Function:	  
  * 		 app_sdmmc_read_sectors
  * Description: 
  * 		 read sectors
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 int 
*****************************************************************************/
int  app_sdmmc_read_sectors(uint8_t *buffer, uint32_t pack_num, uint8_t data_type)
{
    esp_err_t ret;
    uint32_t data_baise = 0;
    uint8_t data_bytes = 1;
    switch(data_type){
        case 1://emg
            data_baise = 0;
            data_bytes = 2;
            break;
        case 2://nirs
            data_baise = NIRS_START_BLOCK;
            data_bytes = 1;
            break;
        case 3://imu
            data_baise = IMU_START_BLOCK;
            data_bytes = 1;
            break;
    }
    ret = sdmmc_read_sectors(card, buffer, pack_num * data_bytes + data_baise, data_bytes);
    //ret = sdmmc_read_sectors(card, buffer, pack_num*SD_SECTOR_NUL, SD_SECTOR_NUL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sectors: %s, data type:%d(1:EMG, 2:NIRS, 3:IMU).", esp_err_to_name(ret), data_type);
        free(card);
    }

    return ret;
}



/*****************************************************************************
  * Function:	  
  * 		 app_sdmmc_write_sectors
  * Description: 
  * 		 write sectors
  * Parameters:  
  * 		 [void]
  * Return: 	 
  * 		 void
*****************************************************************************/
void app_sdmmc_write_sectors(const void* src, size_t start_block, size_t block_count)
{
    esp_err_t ret;

     // 检查缓冲区是否对齐
     if ((uintptr_t)src % 4 != 0) {
        ESP_LOGE(TAG, "Buffer is not aligned to 4 bytes");
        return;
    }

    ret = sdmmc_write_sectors(card,src, start_block, block_count); // 写入 1 个扇区
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write sectors: %s", esp_err_to_name(ret));
        free(card);
        return;
    }
    else
    {
        //printf("SD written successfully %d\r\n",start_block);
    }
}
