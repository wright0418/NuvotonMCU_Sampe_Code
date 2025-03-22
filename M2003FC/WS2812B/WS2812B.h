/**************************************************************************//**
 * @file     WS2812B.h
 * @version  V1.0
 * @brief    WS2812B LED控制函數庫
 *
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __WS2812B_H__
#define __WS2812B_H__

#include "NuMicro.h"

// WS2812B LED控制相關定義
#define WS2812B_PIN      BIT11
#define WS2812B_PORT     PB
#define WS2812B_DO      PB11

// 預設LED數量
#define DEFAULT_WS2812B_COUNT 5
#define MAX_WS2812B_COUNT 30

// RGB顏色結構體
typedef struct {
    uint8_t r;  // 紅色分量 (0-255)
    uint8_t g;  // 綠色分量 (0-255)
    uint8_t b;  // 藍色分量 (0-255)
} RGB_t;

// 預定義彩虹色
extern const RGB_t RAINBOW_COLORS[];
extern const uint8_t RAINBOW_COLOR_COUNT;

// 函數聲明
void DelayNop(uint32_t nops);  // 修改為DelayNop
void SysTick_Init(void);
void WS2812B_Init(void);
void WS2812B_SetLEDCount(uint8_t count);
void WS2812B_SendBit(uint8_t bit);
void WS2812B_SendByte(uint8_t byte);
void WS2812B_SendColor(RGB_t rgb);
void WS2812B_Reset(void);
void WS2812B_Update(void);
void WS2812B_SetLEDColor(uint8_t index, RGB_t rgb);
void WS2812B_ShowRainbow(void);
void WS2812B_ShowSolidColor(RGB_t rgb);
void WS2812B_ShiftRainbow(void);  // 新增：旋轉彩虹顏色

#endif /* __WS2812B_H__ */
