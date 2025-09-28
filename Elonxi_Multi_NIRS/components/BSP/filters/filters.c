/*
 * filters.c
 *
 *  Created on: 2025年1月28日
 *      Author: crk
 */

#include "filters.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


double Bs1_Z[64][3] = {0};
double Bs11_Z[64][3] = {0};
double Bs2_Z[64][3] = {0};
double Bs3_Z[64][3] = {0};
double Bs4_Z[64][3] = {0};
double Bs5_Z[64][3] = {0};
double Bs6_Z[64][3] = {0};
double Bs7_Z[64][3] = {0};
double Bs8_Z[64][3] = {0};
double Bs9_Z[64][3] = {0};
double Bp1_Z[64][3] = {0};
double Bp2_Z[64][3] = {0};
double Bp3_Z[64][3] = {0};
double Bp4_Z[64][3] = {0};
double Bp5_Z[64][3] = {0};
double Lp_Z[64][3] = {0};

void Filter_50Hz_Bs_1(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			-1.902263227297969594431492623698431998491,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			-1.879463762761965917036377504700794816017,
			 0.981907674143790876186699279060121625662
	};

	static const double IIR_PLF_Gain = 0.991153589776121668464270442200358957052 ;
	double finValue = (double)(*inValue);
	Bs1_Z[index][0] = (finValue * IIR_PLF_Gain - Bs1_Z[index][1] * IIR_PLF_A[1] - Bs1_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs1_Z[index][0] * IIR_PLF_B[0] + Bs1_Z[index][1] * IIR_PLF_B[1] + Bs1_Z[index][2] * IIR_PLF_B[2];
	Bs1_Z[index][2] = Bs1_Z[index][1];
	Bs1_Z[index][1] = Bs1_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_50Hz_Bs_2(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			-1.902263227297969594431492623698431998491,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			-1.891260026136428074039486091351136565208,
			 0.982863459887582169294262257608352228999
	};

	static const double IIR_PLF_Gain = 0.991153589776121668464270442200358957052 ;
	double finValue = (double)(*inValue);
	Bs11_Z[index][0] = (finValue * IIR_PLF_Gain - Bs11_Z[index][1] * IIR_PLF_A[1] - Bs11_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs11_Z[index][0] * IIR_PLF_B[0] + Bs11_Z[index][1] * IIR_PLF_B[1] + Bs11_Z[index][2] * IIR_PLF_B[2];
	Bs11_Z[index][2] = Bs11_Z[index][1];
	Bs11_Z[index][1] = Bs11_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_100Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			-1.61816175199937850592846189101692289114 ,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			-1.59807864631544505051863325206795707345 ,
			 0.975177876180648883774892965448088943958
	};

	static const double IIR_PLF_Gain = 0.987588938090324441887446482724044471979 ;
	double finValue = (double)(*inValue);
	Bs2_Z[index][0] = (finValue * IIR_PLF_Gain - Bs2_Z[index][1] * IIR_PLF_A[1] - Bs2_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs2_Z[index][0] * IIR_PLF_B[0] + Bs2_Z[index][1] * IIR_PLF_B[1] + Bs2_Z[index][2] * IIR_PLF_B[2];
	Bs2_Z[index][2] = Bs2_Z[index][1];
	Bs2_Z[index][1] = Bs2_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_150Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			-1.175663330019212304833331472764257341623,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			-1.161072099645408695067771986941806972027,
			 0.975177876180648994797195427963742986321
	};

	static const double IIR_PLF_Gain = 0.987588938090324552909748945239698514342 ;
	double finValue = (double)(*inValue);
	Bs3_Z[index][0] = (finValue * IIR_PLF_Gain - Bs3_Z[index][1] * IIR_PLF_A[1] - Bs3_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs3_Z[index][0] * IIR_PLF_B[0] + Bs3_Z[index][1] * IIR_PLF_B[1] + Bs3_Z[index][2] * IIR_PLF_B[2];
	Bs3_Z[index][2] = Bs3_Z[index][1];
	Bs3_Z[index][1] = Bs3_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_200Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			-0.618082789968684487291739060310646891594,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			-0.610411726197078174926957672141725197434,
			 0.975177876180648883774892965448088943958
	};

	static const double IIR_PLF_Gain = 0.987588938090324441887446482724044471979 ;
	double finValue = (double)(*inValue);
	Bs4_Z[index][0] = (finValue * IIR_PLF_Gain - Bs4_Z[index][1] * IIR_PLF_A[1] - Bs4_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs4_Z[index][0] * IIR_PLF_B[0] + Bs4_Z[index][1] * IIR_PLF_B[1] + Bs4_Z[index][2] * IIR_PLF_B[2];
	Bs4_Z[index][2] = Bs4_Z[index][1];
	Bs4_Z[index][1] = Bs4_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_250Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			-0.000000000000000122474349974549672762046,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			-0.000000000000000120954313234668267014591,
			 0.975177876180648883774892965448088943958
	};

	static const double IIR_PLF_Gain = 0.987588938090324441887446482724044471979 ;
	double finValue = (double)(*inValue);
	Bs5_Z[index][0] = (finValue * IIR_PLF_Gain - Bs5_Z[index][1] * IIR_PLF_A[1] - Bs5_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs5_Z[index][0] * IIR_PLF_B[0] + Bs5_Z[index][1] * IIR_PLF_B[1] + Bs5_Z[index][2] * IIR_PLF_B[2];
	Bs5_Z[index][2] = Bs5_Z[index][1];
	Bs5_Z[index][1] = Bs5_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_300Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			0.618082789968684265247134135279338806868,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			0.610411726197078063904655209626071155071,
			0.975177876180649327864102815510705113411
	};

	static const double IIR_PLF_Gain = 0.987588938090324663932051407755352556705;
	double finValue = (double)(*inValue);
	Bs6_Z[index][0] = (finValue * IIR_PLF_Gain - Bs6_Z[index][1] * IIR_PLF_A[1] - Bs6_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs6_Z[index][0] * IIR_PLF_B[0] + Bs6_Z[index][1] * IIR_PLF_B[1] + Bs6_Z[index][2] * IIR_PLF_B[2];
	Bs6_Z[index][2] = Bs6_Z[index][1];
	Bs6_Z[index][1] = Bs6_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_350Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			1.175663330019211860744121622701641172171,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			1.161072099645408695067771986941806972027,
			0.975177876180649771953312665573321282864
	};

	static const double IIR_PLF_Gain = 0.987588938090324885976656332786660641432;
	double finValue = (double)(*inValue);
	Bs7_Z[index][0] = (finValue * IIR_PLF_Gain - Bs7_Z[index][1] * IIR_PLF_A[1] - Bs7_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs7_Z[index][0] * IIR_PLF_B[0] + Bs7_Z[index][1] * IIR_PLF_B[1] + Bs7_Z[index][2] * IIR_PLF_B[2];
	Bs7_Z[index][2] = Bs7_Z[index][1];
	Bs7_Z[index][1] = Bs7_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_400Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			1.618161751999378727973066816048230975866,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			1.598078646315446160741657877224497497082,
			0.97517787618064999399791759060462936759
	};

	static const double IIR_PLF_Gain = 0.987588938090324996998958795302314683795;
	double finValue = (double)(*inValue);
	Bs8_Z[index][0] = (finValue * IIR_PLF_Gain - Bs8_Z[index][1] * IIR_PLF_A[1] - Bs8_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs8_Z[index][0] * IIR_PLF_B[0] + Bs8_Z[index][1] * IIR_PLF_B[1] + Bs8_Z[index][2] * IIR_PLF_B[2];
	Bs8_Z[index][2] = Bs8_Z[index][1];
	Bs8_Z[index][1] = Bs8_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_450Hz_Bs(u8 index, int* inValue)
{
	static const double IIR_PLF_B[3] =
	{
			1,
			1.902263227297969594431492623698431998491,
			1
	};

	static const double IIR_PLF_A[3] =
	{
			1,
			1.878654120615474765187968841928523033857,
			0.975177876180648550707985577901126816869
	};

	static const double IIR_PLF_Gain = 0.987588938090324330865144020208390429616;
	double finValue = (double)(*inValue);
	Bs9_Z[index][0] = (finValue * IIR_PLF_Gain - Bs9_Z[index][1] * IIR_PLF_A[1] - Bs9_Z[index][2] * IIR_PLF_A[2]) / IIR_PLF_A[0];
	double outValue = Bs9_Z[index][0] * IIR_PLF_B[0] + Bs9_Z[index][1] * IIR_PLF_B[1] + Bs9_Z[index][2] * IIR_PLF_B[2];
	Bs9_Z[index][2] = Bs9_Z[index][1];
	Bs9_Z[index][1] = Bs9_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_PLF(u8 index, int* inValue)
{
	Filter_50Hz_Bs_1(index, inValue);
	Filter_50Hz_Bs_2(index, inValue);
	Filter_100Hz_Bs(index, inValue);
//	Filter_150Hz_Bs(index, inValue);
//	Filter_200Hz_Bs(index, inValue);
//	Filter_250Hz_Bs(index, inValue);
//	Filter_300Hz_Bs(index, inValue);
//	Filter_350Hz_Bs(index, inValue);
//	Filter_400Hz_Bs(index, inValue);
//	Filter_450Hz_Bs(index, inValue);
}

void Filter_BandPass_0d5_50Hz_1(u8 index, int* inValue)
{
	static const double IIR_BP_B[3] =
	{
			1,
			0,
			-1
	};

	static const double IIR_BP_A[3] =
	{
			 1,
			 -1.995558712272719859015523979905992746353,
			 0.995568755195550769698797921591904014349
	};

	static const double IIR_BP_Gain = 0.140431993390145198885576860448054503649 ;
	double finValue = (double)(*inValue);
	Bp2_Z[index][0] = (finValue * IIR_BP_Gain - Bp2_Z[index][1] * IIR_BP_A[1] - Bp2_Z[index][2] * IIR_BP_A[2]) / IIR_BP_A[0];
	double outValue = Bp2_Z[index][0] * IIR_BP_B[0] + Bp2_Z[index][1] * IIR_BP_B[1] + Bp2_Z[index][2] * IIR_BP_B[2];
	Bp2_Z[index][2] = Bp2_Z[index][1];
	Bp2_Z[index][1] = Bp2_Z[index][0];
	*inValue = (int)outValue;
}
//
void Filter_BandPass_0d5_50Hz_2(u8 index, int* inValue)
{
	static const double IIR_BP_B[3] =
	{
			1,
			0,
			-1
	};

	static const double IIR_BP_A[3] =
	{
			 1,
			 -1.567986918658445638641296682180836796761,
			 0.647071130170618458166131858888547867537
	};

	static const double IIR_BP_Gain = 0.140431993390145198885576860448054503649 ;
	double finValue = (double)(*inValue);
	Bp3_Z[index][0] = (finValue * IIR_BP_Gain - Bp3_Z[index][1] * IIR_BP_A[1] - Bp3_Z[index][2] * IIR_BP_A[2]) / IIR_BP_A[0];
	double outValue = Bp3_Z[index][0] * IIR_BP_B[0] + Bp3_Z[index][1] * IIR_BP_B[1] + Bp3_Z[index][2] * IIR_BP_B[2];
	Bp3_Z[index][2] = Bp3_Z[index][1];
	Bp3_Z[index][1] = Bp3_Z[index][0];
	*inValue = (int)outValue;
}

void Filter_BandPass_0d5_50Hz(u8 index, int* inValue)
{
	Filter_BandPass_0d5_50Hz_1(index, inValue);
	Filter_BandPass_0d5_50Hz_2(index, inValue);
}

#define SLIDELEN 500
double SlideWindow[32][SLIDELEN] = {0};
int SlideIndex[32] = {0};
double SlideSum[32] = {0};
double SlideAve[32] = {0};
//
double calcRMS(int index, double inValue)
{
	SlideSum[index] -= SlideWindow[index][SlideIndex[index]];
	SlideWindow[index][SlideIndex[index]] = inValue * inValue;
	SlideSum[index] += inValue * inValue;
	SlideIndex[index] = (SlideIndex[index] + 1) % SLIDELEN;
	SlideAve[index] = sqrt(SlideSum[index] / SLIDELEN);
	return SlideAve[index];
//	*inValue = (int)ave;
}




// 初始化中值滤波器
MedianFilter* medianFilterInit(int window_size) {
    if (window_size % 2 == 0) {
        printf("Error: Window size must be odd.\n");
        return NULL;
    }

    MedianFilter* filter = (MedianFilter*)malloc(sizeof(MedianFilter));
    filter->window_size = window_size;
    filter->buffer_size = window_size;
    filter->head = 0;
    filter->buffer_filled = false;
    
    filter->buffer = (uint16_t*)malloc(window_size * sizeof(uint16_t));
    filter->window = (uint16_t*)malloc(window_size * sizeof(uint16_t));
    
    return filter;
}

// 释放滤波器资源
void medianFilterFree(MedianFilter* filter) {
    free(filter->buffer);
    free(filter->window);
    free(filter);
}

// 插入排序（维护有序窗口）
static void insertSorted(uint16_t* window, int size, uint16_t newValue, uint16_t oldValue) {
    // 移除旧值（如果存在）
    if (oldValue != -1) {
        int i;
        for (i = 0; i < size; i++) {
            if (window[i] == oldValue) {
                break;
            }
        }
        
        // 将后面的元素前移
        for (; i < size - 1; i++) {
            window[i] = window[i + 1];
        }
        
        // 插入新值到合适位置
        for (i = size - 2; i >= 0 && window[i] > newValue; i--) {
            window[i + 1] = window[i];
        }
        window[i + 1] = newValue;
    } else {
        // 首次填充时简单排序
        int i;
        for (i = 0; i < size - 1 && window[i] < newValue; i++);
        for (int j = size - 1; j > i; j--) {
            window[j] = window[j - 1];
        }
        window[i] = newValue;
    }
}

// 实时处理单个样本
uint16_t medianFilterProcess(MedianFilter* filter, uint16_t input) {
    uint16_t oldValue = 0;
    
    // 如果缓冲区已满，记录将被替换的旧值
    if (filter->buffer_filled) {
        oldValue = filter->buffer[filter->head];
    }
    
    // 将新值存入循环缓冲区
    filter->buffer[filter->head] = input;
    filter->head = (filter->head + 1) % filter->buffer_size;
    
    // 更新窗口数据
    if (!filter->buffer_filled) {
        // 首次填充缓冲区
        if (filter->head == 0) {
            filter->buffer_filled = true;
            
            // 复制数据到窗口并排序
            for (int i = 0; i < filter->window_size; i++) {
                filter->window[i] = filter->buffer[i];
            }
            
            // 使用插入排序初始化窗口
            for (int i = 1; i < filter->window_size; i++) {
                float key = filter->window[i];
                int j = i - 1;
                while (j >= 0 && filter->window[j] > key) {
                    filter->window[j + 1] = filter->window[j];
                    j--;
                }
                filter->window[j + 1] = key;
            }
        }
    } else {
        // 维护有序窗口
        insertSorted(filter->window, filter->window_size, input, oldValue);
    }
    
    // 返回中值（只有当缓冲区填满时才有效）
    return filter->buffer_filled ? filter->window[filter->window_size / 2] : input;
}

/**
 * 初始化二阶低通滤波器
 * @param filter 滤波器结构体指针
 * @param sample_rate 采样率(Hz)
 * @param cutoff_freq 截止频率(Hz)
 * @param Q 品质因数(建议0.707为Butterworth响应)
 */
void initLowPassFilter(BiquadFilter* filter, float sample_rate, float cutoff_freq, float Q) {
    float omega = 2.0f * M_PI * cutoff_freq / sample_rate;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * Q);
    
    float b0 = (1.0f - cos_omega) / 2.0f;
    float b1 = 1.0f - cos_omega;
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_omega;
    float a2 = 1.0f - alpha;
    
    // 归一化系数
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
    
    // 清零延迟线
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

/**
 * 执行实时滤波处理
 * @param filter 滤波器结构体指针
 * @param input 输入样本
 * @return 滤波后的输出
 */
float processLowPassFilter(BiquadFilter* filter, float input) {
    // 计算输出
    float output = filter->b0 * input 
                 + filter->b1 * filter->x1 
                 + filter->b2 * filter->x2 
                 - filter->a1 * filter->y1 
                 - filter->a2 * filter->y2;
    
    // 更新延迟线
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    
    return output;
}

void initAntiDriftLPF(DcBlockingBiquad* filter, float sample_rate, float cutoff_freq) {
    float omega = 2 * M_PI * cutoff_freq / sample_rate;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / sqrtf(2); // Butterworth Q
    
    // 分子系数确保直流增益为0
    filter->b0 = (1 - cos_omega)/2;
    filter->b1 = 1 - cos_omega;
    filter->b2 = filter->b0;
    
    // 分母系数
    filter->a1 = -2 * cos_omega;
    filter->a2 = 1 - alpha;
    
    // 调整通带增益
    float omega_c = omega;
    float passband_gain = 1.0f/sqrtf(1 + powf(omega_c/omega, 4));
    float current_gain = (filter->b0 + filter->b1 + filter->b2)/(1 + filter->a1 + filter->a2);
    float scale = passband_gain / fabsf(current_gain);
    
    filter->b0 *= scale;
    filter->b1 *= scale;
    filter->b2 *= scale;
    
    // 初始化状态
    filter->x1 = filter->x2 = 0;
    filter->y1 = filter->y2 = 0;
}

float processAntiDriftLPF(DcBlockingBiquad* filter, float input) {
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                 - filter->a1 * filter->y1 - filter->a2 * filter->y2;
    
    // 更新状态
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    
    return output;
}