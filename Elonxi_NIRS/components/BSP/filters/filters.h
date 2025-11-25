/*
 * filters.h
 *
 *  Created on: 2025年1月28日
 *      Author: crk
 */

#ifndef INC_FILTERS_H_
#define INC_FILTERS_H_

#include "esp_log.h"
#include "stdbool.h"

typedef uint16_t u16;
typedef uint8_t u8;

typedef struct {
    uint16_t* buffer;      // 数据缓冲区
    uint16_t* window;      // 当前窗口数据（排序后）
    int window_size;    // 窗口大小（必须为奇数）
    int buffer_size;    // 缓冲区大小（通常等于window_size）
    int head;           // 缓冲区头部指针
    bool buffer_filled; // 缓冲区是否已填满
} MedianFilter;

typedef struct {
    float a0, a1, a2;  // 分母系数
    float b0, b1, b2;  // 分子系数
    float x1, x2;      // 输入延迟线
    float y1, y2;      // 输出延迟线
} BiquadFilter;

typedef struct {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} DcBlockingBiquad;

void Filter_PLF(u8 index, int* inValue);
void Filter_BandPass_0d5_50Hz(u8 index, int* inValue);

uint16_t medianFilterProcess(MedianFilter* filter, uint16_t input);
MedianFilter* medianFilterInit(int window_size);
void medianFilterFree(MedianFilter* filter);

float processLowPassFilter(BiquadFilter* filter, float input);
void initLowPassFilter(BiquadFilter* filter, float sample_rate, float cutoff_freq, float Q);

void initAntiDriftLPF(DcBlockingBiquad* filter, float sample_rate, float cutoff_freq);
float processAntiDriftLPF(DcBlockingBiquad* filter, float input);
#endif /* INC_FILTERS_H_ */
