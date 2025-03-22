/**************************************************************************//**
 * @file     WS2812B.c
 * @version  V1.0
 * @brief    WS2812B LED控制函數實現
 *
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include "WS2812B.h"

// WS2812B控制變數
volatile uint8_t g_ws2812bCount = DEFAULT_WS2812B_COUNT;
volatile RGB_t g_ws2812bColors[MAX_WS2812B_COUNT];

// 預定義彩虹色
const RGB_t RAINBOW_COLORS[] = {
    {255, 0, 0},     // 紅
    {255, 127, 0},   // 橙
    {255, 255, 0},   // 黃
    {0, 255, 0},     // 綠
    {0, 0, 255},     // 藍
    {75, 0, 130},    // 靛
    {143, 0, 255}    // 紫
};
const uint8_t RAINBOW_COLOR_COUNT = sizeof(RAINBOW_COLORS) / sizeof(RGB_t);

/**
 * @brief    初始化 SysTick 定時器，提供納秒級延遲
 * @param    無
 * @return   無
 */
void SysTick_Init(void)
{
    // 確保SysTick定時器關閉
    SysTick->CTRL = 0;
    
    // 設置SysTick重載值 (系統時鐘頻率對應每個tick的時間)
    // 由於納秒級精度需要更高分辨率，這裡設置為最大值
    SysTick->LOAD = 0xFFFFFF;
    
    // 清除當前值
    SysTick->VAL = 0;
    
    // 使用處理器時鐘作為時鐘源，啟用定時器但不使能中斷
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    
    printf("SysTick initialization complete, frequency: %d MHz\n", SystemCoreClock / 1000000);
}

/**
 * @brief    納秒級延遲函數 - 優化版
 * @param    ns 要延遲的納秒數
 * @return   無
 */
void DelayNs(uint32_t ns)
{
    // 按照24MHz CPU時鐘，每個NOP約執行41.67ns
    // 簡化計算: 24個NOP約等於1微秒
    uint32_t nops = ns / 42;
    
    // 避免零延遲
    if (nops == 0) nops = 1;
    
    // 使用NOP指令進行精確延遲
    for (uint32_t i = 0; i < nops; i++) {
        __NOP();
    }
}

/**
 * @brief    初始化WS2812B控制引腳
 * @param    無
 * @return   無
 */
void WS2812B_Init(void)
{
    // 確保GPB時鐘已啟用
    CLK_EnableModuleClock(GPB_MODULE);
    
    // 設置GPB11為輸出模式
    GPIO_SetMode(WS2812B_PORT, WS2812B_PIN, GPIO_MODE_OUTPUT);
    
    // 初始設置為低電平
    WS2812B_PORT->DOUT &= ~WS2812B_PIN;
    
    // 初始化SysTick定時器
    SysTick_Init();
    
    printf("WS2812B initialization complete (GPB11)\n");
}

/**
 * @brief    設置WS2812B LED數量
 * @param    count LED數量
 * @return   無
 */
void WS2812B_SetLEDCount(uint8_t count)
{
    if (count > MAX_WS2812B_COUNT) {
        count = MAX_WS2812B_COUNT;
        printf("Warning: LED count exceeds limit, set to maximum: %d\n", MAX_WS2812B_COUNT);
    }
    
    g_ws2812bCount = count;
    printf("WS2812B LED count set to: %d\n", g_ws2812bCount);
}

/**
 * @brief    執行指定數量的NOP指令以實現延遲
 * @param    nops 要執行的NOP指令數量
 * @return   無
 */
void DelayNop(uint32_t nops)
{
    // 避免零延遲
    if (nops == 0) nops = 1;
    
    // 使用NOP指令進行精確延遲
    for (uint32_t i = 0; i < nops; i++) {
        __NOP();
    }
}

/**
 * @brief    發送一個位元到WS2812B
 * @param    bit 要發送的位元 (0或1)
 * @return   無
 */
void WS2812B_SendBit(uint8_t bit)
{
    // 4 , 400ns , 20,800ns
    if (bit) {
        // 發送 "1" - T0H: 700ns, T0L: 600ns
        WS2812B_DO = 1;  // 高電平
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();                        // 約400ns (24MHz時鐘下)
        WS2812B_DO = 0; // 低電平
        __NOP();
        __NOP();
        __NOP();
        __NOP();
                               // 約600ns (24MHz時鐘下)
    } else {
        // 發送 "0" - T1H: 350ns, T1L: 800ns
        WS2812B_DO = 1;  // 高電平
        __NOP();
        __NOP();
        __NOP();
        __NOP();                        // 約350ns (24MHz時鐘下)
        WS2812B_DO = 0; // 低電平
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();                      // 約800ns (24MHz時鐘下)
    }
}

/**
 * @brief    發送一個字節到WS2812B
 * @param    byte 要發送的字節
 * @return   無
 */
void WS2812B_SendByte(uint8_t byte)
{
    for (int8_t i = 7; i >= 0; i--) {
        WS2812B_SendBit((byte >> i) & 0x01);
    }
}

/**
 * @brief    發送一個RGB顏色到WS2812B
 * @param    rgb RGB顏色結構體
 * @return   無
 */
void WS2812B_SendColor(RGB_t rgb)
{
    // WS2812B顏色順序為GRB
    WS2812B_SendByte(rgb.g);
    WS2812B_SendByte(rgb.r);
    WS2812B_SendByte(rgb.b);
}

/**
 * @brief    重置WS2812B，準備新的數據傳輸
 * @param    無
 * @return   無
 */
void WS2812B_Reset(void)
{
    WS2812B_PORT->DOUT &= ~WS2812B_PIN; // 低電平
    // >50us的低電平作為重置信號
    DelayNop(1200);                     // 約50us (24MHz時鐘下)
}

/**
 * @brief    更新所有WS2812B LED的顏色
 * @param    無
 * @return   無
 */
void WS2812B_Update(void)
{
    uint8_t i;
    
    // 禁用中斷以確保時序準確
    __disable_irq();
    
    // 發送各個LED的顏色數據
    for (i = 0; i < g_ws2812bCount; i++) {
        WS2812B_SendColor(g_ws2812bColors[i]);
    }
    
    // 重置信號，表示傳輸結束
    WS2812B_Reset();
    
    // 重新啟用中斷
    __enable_irq();
}

/**
 * @brief    設置指定LED的顏色
 * @param    index LED索引
 * @param    rgb   RGB顏色
 * @return   無
 */
void WS2812B_SetLEDColor(uint8_t index, RGB_t rgb)
{
    if (index < g_ws2812bCount) {
        g_ws2812bColors[index] = rgb;
    }
}

/**
 * @brief    顯示彩虹色彩
 * @param    無
 * @return   無
 */
void WS2812B_ShowRainbow(void)
{
    for (uint8_t i = 0; i < g_ws2812bCount; i++) {
        // 計算彩虹色索引，確保在彩虹色陣列範圍內循環
        uint8_t colorIndex = i % RAINBOW_COLOR_COUNT;
        WS2812B_SetLEDColor(i, RAINBOW_COLORS[colorIndex]);
    }
    
    // 更新所有LED
    WS2812B_Update();
    printf("Rainbow colors displayed\n");
}

/**
 * @brief    顯示單一顏色
 * @param    rgb 要顯示的顏色
 * @return   無
 */
void WS2812B_ShowSolidColor(RGB_t rgb)
{
    for (uint8_t i = 0; i < g_ws2812bCount; i++) {
        WS2812B_SetLEDColor(i, rgb);
    }
    
    // 更新所有LED
    WS2812B_Update();
    printf("Solid color displayed (R:%d G:%d B:%d)\n", rgb.r, rgb.g, rgb.b);
}

/**
 * @brief    旋轉彩虹顏色，使其向前移動一個位置
 * @param    無
 * @return   無
 */
void WS2812B_ShiftRainbow(void)
{
    if (g_ws2812bCount <= 1) return;  // 如果只有一個LED或沒有LED，不做任何操作
    
    // 保存第一個LED的顏色
    RGB_t firstColor = g_ws2812bColors[0];
    
    // 將所有LED的顏色向前移動一個位置
    for (uint8_t i = 0; i < g_ws2812bCount - 1; i++) {
        g_ws2812bColors[i] = g_ws2812bColors[i + 1];
    }
    
    // 將第一個顏色放到最後
    g_ws2812bColors[g_ws2812bCount - 1] = firstColor;
    
    // 更新所有LED
    WS2812B_Update();
    printf("Rainbow colors shifted one position\n");
}
