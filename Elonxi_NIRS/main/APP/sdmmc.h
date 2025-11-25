/**
 ****************************************************************************************************
* @file        sd_card.h
* @author      zz
* @version     V1.0
* @date        2025-03-06
* @brief       sd card sdmmc
* @license     ELONXI
****************************************************************************************************
*/

#ifndef __SDMMC_H
#define __SDMMC_H

void sd_card_init(void);
int  app_sdmmc_read_sectors(uint8_t *buffer, uint32_t pack_num);
int  app_sdmmc_read_sectors_imu(uint8_t *buffer, uint32_t pack_num);
int app_sdmmc_read_sectors_safe(uint8_t *buffer, uint32_t pack_num);

void app_sdmmc_write_sectors(const void* src, size_t start_block, size_t block_count);
#endif
