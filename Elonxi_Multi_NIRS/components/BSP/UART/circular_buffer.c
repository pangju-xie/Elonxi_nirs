/**
 * @file circular_buffer.c
 * @brief 循环缓冲区实现文件
 * @version 1.0
 * @date 2024
 */

#include "circular_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include "uart.h"

static const char *TAG = "BUFFER";
/**
 * @brief 验证循环缓冲区指针有效性
 * @param cb 循环缓冲区指针
 * @return 操作结果
 */
static inline circ_buf_result_t validate_buffer(circular_buffer_t *cb)
{
    if (cb == NULL) {
        return CIRC_BUF_ERROR_NULL_POINTER;
    }
    if (cb->buffer == NULL || cb->buffer_size == 0) {
        return CIRC_BUF_ERROR_INVALID_PARAM;
    }
    return CIRC_BUF_OK;
}

circ_buf_result_t circular_buffer_init(circular_buffer_t *cb, uint32_t size)
{
    if (cb == NULL) {
        return CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (size == 0) {
        return CIRC_BUF_ERROR_INVALID_SIZE;
    }
    
    // 分配内存
    cb->buffer = (uint8_t*)malloc(size);
    if (cb->buffer == NULL) {
        return CIRC_BUF_ERROR_NO_MEMORY;
    }
    
    // 初始化参数
    cb->buffer_size = size;
    cb->write_pos = 0;
    cb->read_pos = 0;
    cb->data_len = 0;
    cb->allocated = true;
    
    return CIRC_BUF_OK;
}

circ_buf_result_t circular_buffer_init_static(circular_buffer_t *cb, uint8_t *buffer, uint32_t size)
{
    if (cb == NULL || buffer == NULL) {
        return CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (size == 0) {
        return CIRC_BUF_ERROR_INVALID_SIZE;
    }
    
    // 使用静态缓冲区
    cb->buffer = buffer;
    cb->buffer_size = size;
    cb->write_pos = 0;
    cb->read_pos = 0;
    cb->data_len = 0;
    cb->allocated = false;
    
    return CIRC_BUF_OK;
}

circ_buf_result_t circular_buffer_deinit(circular_buffer_t *cb)
{
    if (cb == NULL) {
        return CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    // 如果是动态分配的内存，则释放
    if (cb->allocated && cb->buffer != NULL) {
        free(cb->buffer);
    }
    
    // 重置所有参数
    cb->buffer = NULL;
    cb->buffer_size = 0;
    cb->write_pos = 0;
    cb->read_pos = 0;
    cb->data_len = 0;
    cb->allocated = false;
    
    return CIRC_BUF_OK;
}

circ_buf_result_t circular_buffer_reset(circular_buffer_t *cb)
{
    circ_buf_result_t result = validate_buffer(cb);
    if (result != CIRC_BUF_OK) {
        return result;
    }
    
    cb->write_pos = 0;
    cb->read_pos = 0;
    cb->data_len = 0;
    
    return CIRC_BUF_OK;
}

int32_t circular_buffer_write(circular_buffer_t *cb, const uint8_t *data, uint32_t len)
{
    if (validate_buffer(cb) != CIRC_BUF_OK || data == NULL) {
        return -CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (len == 0) {
        return 0;
    }
    
    // 检查剩余空间
    uint32_t free_space = cb->buffer_size - cb->data_len;
    if (len > free_space) {
        return -CIRC_BUF_ERROR_BUFFER_FULL;
    }
    
    uint32_t written = 0;
    
    for (uint32_t i = 0; i < len; i++) {
        cb->buffer[cb->write_pos] = data[i];
        cb->write_pos = (cb->write_pos + 1) % cb->buffer_size;
        cb->data_len++;
        written++;
    }
    
    return written;
}

int32_t circular_buffer_write_force(circular_buffer_t *cb, const uint8_t *data, uint32_t len)
{
    if (validate_buffer(cb) != CIRC_BUF_OK || data == NULL) {
        return -CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (len == 0) {
        return 0;
    }
    
    uint32_t written = 0;
    
    for (uint32_t i = 0; i < len; i++) {
        // 如果缓冲区满，丢弃最老的数据
        if (cb->data_len >= cb->buffer_size) {
            cb->read_pos = (cb->read_pos + 1) % cb->buffer_size;
            cb->data_len--;
        }
        
        cb->buffer[cb->write_pos] = data[i];
        cb->write_pos = (cb->write_pos + 1) % cb->buffer_size;
        cb->data_len++;
        written++;
    }
    
    return written;
}

int32_t circular_buffer_read(circular_buffer_t *cb, uint8_t *data, uint32_t len)
{
    if (validate_buffer(cb) != CIRC_BUF_OK || data == NULL) {
        return -CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (len == 0) {
        return 0;
    }
    
    // 限制读取长度为实际可用数据长度
    if (len > cb->data_len) {
        len = cb->data_len;
    }
    
    uint32_t read_count = 0;
    
    for (uint32_t i = 0; i < len; i++) {
        data[i] = cb->buffer[cb->read_pos];
        cb->read_pos = (cb->read_pos + 1) % cb->buffer_size;
        cb->data_len--;
        read_count++;
    }
    
    return read_count;
}

circ_buf_result_t circular_buffer_peek(circular_buffer_t *cb, uint32_t offset, uint8_t *data)
{
    circ_buf_result_t result = validate_buffer(cb);
    if (result != CIRC_BUF_OK) {
        return result;
    }
    
    if (data == NULL) {
        return CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (offset >= cb->data_len) {
        return CIRC_BUF_ERROR_INVALID_PARAM;
    }
    
    uint32_t pos = (cb->read_pos + offset) % cb->buffer_size;
    *data = cb->buffer[pos];
    
    return CIRC_BUF_OK;
}

int32_t circular_buffer_discard(circular_buffer_t *cb, uint32_t len)
{
    if (validate_buffer(cb) != CIRC_BUF_OK) {
        return -CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (len == 0) {
        return 0;
    }
    
    // 限制丢弃长度为实际可用数据长度
    if (len > cb->data_len) {
        len = cb->data_len;
    }
    
    cb->read_pos = (cb->read_pos + len) % cb->buffer_size;
    cb->data_len -= len;
    
    return len;
}

int32_t circular_buffer_find(circular_buffer_t *cb, const uint8_t *pattern, uint32_t pattern_len, uint32_t start_offset)
{
    if (validate_buffer(cb) != CIRC_BUF_OK || pattern == NULL) {
        return -CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    if (pattern_len == 0 || start_offset >= cb->data_len) {
        return -CIRC_BUF_ERROR_INVALID_PARAM;
    }
    
    if (cb->data_len < pattern_len + start_offset) {
        return -1;  // 数据不够，无法匹配
    }
    
    // 从start_offset开始搜索
    for (uint32_t i = start_offset; i <= cb->data_len - pattern_len; i++) {
        bool match = true;
        
        // 检查是否匹配
        for (uint32_t j = 0; j < pattern_len; j++) {
            uint32_t pos = (cb->read_pos + i + j) % cb->buffer_size;
            if (cb->buffer[pos] != pattern[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            return i;  // 返回匹配位置
        }
    }
    
    return -1;  // 未找到
}

int32_t circular_buffer_get_data_len(circular_buffer_t *cb)
{
    if (validate_buffer(cb) != CIRC_BUF_OK) {
        return -CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    return cb->data_len;
}

int32_t circular_buffer_get_free_space(circular_buffer_t *cb)
{
    if (validate_buffer(cb) != CIRC_BUF_OK) {
        return -CIRC_BUF_ERROR_NULL_POINTER;
    }
    
    return cb->buffer_size - cb->data_len;
}

bool circular_buffer_is_empty(circular_buffer_t *cb)
{
    if (validate_buffer(cb) != CIRC_BUF_OK) {
        return false;
    }
    
    return cb->data_len == 0;
}

bool circular_buffer_is_full(circular_buffer_t *cb)
{
    if (validate_buffer(cb) != CIRC_BUF_OK) {
        return false;
    }
    
    return cb->data_len >= cb->buffer_size;
}

const char* circular_buffer_get_error_string(circ_buf_result_t result)
{
    switch (result) {
        case CIRC_BUF_OK:
            return "Success";
        case CIRC_BUF_ERROR_NULL_POINTER:
            return "Null pointer error";
        case CIRC_BUF_ERROR_INVALID_SIZE:
            return "Invalid size";
        case CIRC_BUF_ERROR_NO_MEMORY:
            return "Memory allocation failed";
        case CIRC_BUF_ERROR_BUFFER_FULL:
            return "Buffer is full";
        case CIRC_BUF_ERROR_BUFFER_EMPTY:
            return "Buffer is empty";
        case CIRC_BUF_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        default:
            return "Unknown error";
    }
}
/**
 * @brief 查找帧头位置
 * @param cb 循环缓冲区指针
 * @return 帧头位置偏移量，-1表示未找到
 */
static int32_t find_frame_header(circular_buffer_t *cb)
{
    int32_t data_len = circular_buffer_get_data_len(cb);
    if (data_len < 4) {
        return -1;
    }
    
    // 构造帧头模式
    uint8_t frame_header[3] = {0xfe, 0xdc, 0xba};
    
    // 使用循环缓冲区的查找函数
    return circular_buffer_find(cb, frame_header, 3, 0);
}

/**
 * @brief 从循环缓冲区读取指定偏移位置的字节
 * @param cb 循环缓冲区指针
 * @param offset 偏移量
 * @param data 存储结果的指针
 * @return 操作结果
 */
static bool peek_byte_at_offset(circular_buffer_t *cb, uint32_t offset, uint8_t *data)
{
    return circular_buffer_peek(cb, offset, data) == CIRC_BUF_OK;
}

/**
 * @brief 验证并处理完整帧
 * @param cb 循环缓冲区指针
 * @param frame_start 帧头位置
 * @return true表示处理成功，false表示帧不完整或错误
 */
static bool process_frame(circular_buffer_t *cb, uint32_t frame_start)
{
    int32_t available_data = circular_buffer_get_data_len(cb);
    
    // 检查是否有足够的数据来读取长度字段
    if (available_data < (int32_t)(frame_start + 4)) {
        return false;  // 数据不够，等待更多数据
    }
    
    // 读取数据长度字段 (假设在第7、8字节位置，大端序)
    uint8_t len_high, len_low;
    if (!peek_byte_at_offset(cb, frame_start + 4, &len_high) ||
        !peek_byte_at_offset(cb, frame_start + 5, &len_low)) {
        ESP_LOGE(TAG, "Failed to read length field");
        return false;
    }
    
    uint16_t data_len = (len_high << 8) | len_low;
    
    // 计算完整帧长度
    uint32_t total_frame_len = MIN_FRAME_SIZE + data_len;
    
    // 验证帧长度合理性
    if (total_frame_len > MAX_FRAME_SIZE) {
        ESP_LOGE(TAG, "Frame too large: %ld bytes", total_frame_len);
        return false;
    }
    
    // 检查是否接收到完整帧
    if (available_data < (int32_t)(frame_start + total_frame_len)) {
        return false;  // 帧不完整，等待更多数据
    }
    
    // 先丢弃帧头之前的无效数据
    if (frame_start > 0) {
        int32_t discarded = circular_buffer_discard(cb, frame_start);
        if (discarded > 0) {
            ESP_LOGE(TAG, "Discarded %ld invalid bytes", discarded);
        }
    }
    
    // 读取完整帧
    uint8_t frame_buf[MAX_FRAME_SIZE];
    int32_t read_len = circular_buffer_read(cb, frame_buf, total_frame_len);
    
    if (read_len != (int32_t)total_frame_len) {
        ESP_LOGE(TAG, "Frame read error: expected %ld, got %ld", total_frame_len, read_len);
        return false;
    }
    
    // 验证帧头
    if (frame_buf[0] != 0xfe || frame_buf[1] != 0xdc || frame_buf[2] != 0xba) {
        ESP_LOGE(TAG, "Frame header verification failed");
        return false;
    }
    
    // 验证长度字段
    uint16_t expected_data_len = (frame_buf[4] << 8) | frame_buf[5];
    if (expected_data_len != data_len) {
        ESP_LOGE(TAG, "Frame length mismatch: expected %d, got %d", (int)data_len, (int)expected_data_len);
        return false;
    }
    
    // 打印接收到的数据（调试用）
    ESP_LOGI(TAG, "Received valid frame: %ld bytes", total_frame_len);
    
    //表面肌电
    if(frame_buf[3] == 0x01) {
        memcpy(&g_struct_para.emg_data, frame_buf+6, data_len);
        g_struct_para.emg_send_flag = 1;
    }
    //肌氧
    else if(frame_buf[3] == 0x02){
        memcpy(&g_struct_para.nirs_data, frame_buf+6, data_len);
        g_struct_para.nirs_send_flag = 1;
    }
    // 发送UDP数据
    // udp_safe_send(frame_buf, total_frame_len);

    
    return true;
}

/**
 * @brief 处理接收缓冲区中的所有完整帧
 * @param cb 循环缓冲区指针
 */
void process_all_frames(circular_buffer_t *cb)
{
    int32_t max_iterations = 100; // 防止无限循环
    
    while (max_iterations-- > 0) {
        // 查找帧头
        int32_t header_pos = find_frame_header(cb);
        
        if (header_pos == -1) {
            // 没有找到帧头
            int32_t data_len = circular_buffer_get_data_len(cb);
            if (data_len > MAX_FRAME_SIZE) {
                // 如果缓冲区中的数据太多且没有帧头，清空一部分数据
                uint32_t discard_len = data_len - MAX_FRAME_SIZE / 2;
                int32_t discarded = circular_buffer_discard(cb, discard_len);
                if (discarded > 0) {
                    ESP_LOGE(TAG, "Buffer overflow protection: discarded %ld bytes", discarded);
                }
            }
            break;
        }
        
        // 尝试处理从帧头开始的帧
        if (!process_frame(cb, header_pos)) {
            // 如果帧不完整或处理失败
            if (header_pos == 0) {
                int32_t data_len = circular_buffer_get_data_len(cb);
                if (data_len >= MIN_FRAME_SIZE) {
                    // 当前位置就是帧头但处理失败，可能是损坏的帧，丢弃帧头继续查找
                    circular_buffer_discard(cb, 2);
                    ESP_LOGW(TAG, "Discarded corrupted frame header");
                    continue;
                }
            }
            break;
        }
        
        // 成功处理了一帧，继续查找下一帧
    }
    
    if (max_iterations <= 0) {
        ESP_LOGW(TAG, "Frame processing loop limit reached");
    }
}