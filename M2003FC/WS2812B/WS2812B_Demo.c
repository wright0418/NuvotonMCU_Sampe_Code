/**************************************************************************//**
 * @file     main.c
 * @version  V0.10
 * @brief    A project template for M2003C MCU.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 ****************************************************************************/

#include <stdio.h>
#include "NuMicro.h"
#include "WS2812B.h" // 引用新的 WS2812B 標頭檔

// LED控制結構體定義
typedef struct {
    GPIO_T* port;          // LED所在的GPIO port (PA, PB等)
    uint32_t pin;          // LED的pin號碼 (BIT0, BIT1等)
    uint32_t onTime;       // LED亮的時間 (ms)
    uint32_t offTime;      // LED暗的時間 (ms)
    uint32_t delayCount;   // 目前延遲計數
    uint8_t state;         // LED目前狀態 (0:關閉, 1:亮起)
    uint8_t delayComplete; // 延遲完成標誌
    char name[16];         // LED名稱，用於調試
} LED_t;

// LED模式參數結構體
typedef struct {
    uint32_t defaultOnTime;   // 預設模式亮時間
    uint32_t defaultOffTime;  // 預設模式暗時間
    uint32_t altOnTime;       // 替代模式亮時間
    uint32_t altOffTime;      // 替代模式暗時間
} LEDTiming_t;

// 全局變數
#define MAX_LED_COUNT 10
volatile uint32_t g_u32Timer0Ticks = 0;
volatile LED_t g_LEDs[MAX_LED_COUNT];
volatile uint8_t g_LEDCount = 0;
volatile LEDTiming_t g_LEDTimings[MAX_LED_COUNT];
volatile uint32_t g_lastModeChangeTime = 0;
volatile uint8_t g_currentMode = 0; // 0: 預設模式, 1: 替代模式

// 按鈕相關變數
volatile uint8_t g_buttonPrevState = 1; // 初始狀態為高電平 (未按下)
volatile uint32_t g_lastButtonPressTime = 0; // 上次按鈕按下的時間
#define DEBOUNCE_TIME 20 // 按鈕防抖時間 (20 × 10ms = 200ms)

void SetVTOR(void)
{
    extern uint32_t Image$$RO$$Base;
    SCB->VTOR = (uint32_t)&Image$$RO$$Base;
}

/**
 * @brief    註冊一個LED到控制系統
 * @param    port     LED所在的GPIO port
 * @param    pin      LED的pin號碼
 * @param    onTime   LED亮的時間 (ms)
 * @param    offTime  LED暗的時間 (ms)
 * @param    name     LED名稱
 * @return   LED的索引，如果失敗則返回-1
 */
int LED_Register(GPIO_T* port, uint32_t pin, uint32_t onTime, uint32_t offTime, const char* name)
{
    if (g_LEDCount >= MAX_LED_COUNT) {
        printf("Error: LED count exceeds limit\n");
        return -1;
    }
    
    // 將LED資訊儲存到陣列中
    g_LEDs[g_LEDCount].port = port;
    g_LEDs[g_LEDCount].pin = pin;
    g_LEDs[g_LEDCount].onTime = onTime;
    g_LEDs[g_LEDCount].offTime = offTime;
    g_LEDs[g_LEDCount].delayCount = offTime / 10; // 初始為關閉狀態，等待offTime
    g_LEDs[g_LEDCount].state = 0;                 // 初始為關閉狀態
    g_LEDs[g_LEDCount].delayComplete = 0;         // 初始為未完成狀態
    
    // 複製名稱，確保不超出緩衝區
    strncpy((char*)g_LEDs[g_LEDCount].name, name, 15);
    g_LEDs[g_LEDCount].name[15] = '\0'; // 確保字符串以NULL結尾
    
    // 設置GPIO為輸出模式
    GPIO_SetMode(port, pin, GPIO_MODE_OUTPUT);
    
    // 初始設置為關閉狀態 (高電平)
    port->DOUT |= pin;
    
    return g_LEDCount++;
}

/**
 * @brief    更新指定LED的時間設定
 * @param    index    LED的索引
 * @param    onTime   新的亮時間 (ms)
 * @param    offTime  新的暗時間 (ms)
 * @return   成功返回0，失敗返回-1
 */
int LED_UpdateTiming(uint8_t index, uint32_t onTime, uint32_t offTime)
{
    if (index >= g_LEDCount) {
        return -1;
    }
    
    g_LEDs[index].onTime = onTime;
    g_LEDs[index].offTime = offTime;
    
    return 0;
}

/**
 * @brief    設置LED的替代閃爍模式時間
 * @param    index    LED的索引
 * @param    onTime   替代模式亮時間 (ms)
 * @param    offTime  替代模式暗時間 (ms)
 * @return   無
 */
void LED_SetAlternateTiming(uint8_t index, uint32_t onTime, uint32_t offTime)
{
    if (index >= g_LEDCount) {
        return;
    }
    
    // 儲存原始時間設定
    g_LEDTimings[index].defaultOnTime = g_LEDs[index].onTime;
    g_LEDTimings[index].defaultOffTime = g_LEDs[index].offTime;
    
    // 設置替代模式時間
    g_LEDTimings[index].altOnTime = onTime;
    g_LEDTimings[index].altOffTime = offTime;
}

/**
 * @brief    切換所有LED的閃爍模式
 */
void LED_SwitchMode(void)
{
    // 切換模式
    g_currentMode = !g_currentMode;
    
    // 根據當前模式更新所有LED的時間
    for (uint8_t i = 0; i < g_LEDCount; i++) {
        if (g_currentMode == 0) {
            // 切換到預設模式
            LED_UpdateTiming(i, g_LEDTimings[i].defaultOnTime, g_LEDTimings[i].defaultOffTime);
            printf("Switch %s to default mode (%u ms, %u ms)\n", 
                  g_LEDs[i].name, g_LEDTimings[i].defaultOnTime, g_LEDTimings[i].defaultOffTime);
        } else {
            // 切換到替代模式
            LED_UpdateTiming(i, g_LEDTimings[i].altOnTime, g_LEDTimings[i].altOffTime);
            printf("Switch %s to alternate mode (%u ms, %u ms)\n", 
                  g_LEDs[i].name, g_LEDTimings[i].altOnTime, g_LEDTimings[i].altOffTime);
        }
    }
    
    // 更新模式切換時間戳
    g_lastModeChangeTime = g_u32Timer0Ticks;
}

/**
 * @brief    Timer0 中斷處理函數
 */
void TMR0_IRQHandler(void)
{
    uint8_t i;
    
    // 清除中斷標誌
    TIMER_ClearIntFlag(TIMER0);
    
    // 增加計數器
    g_u32Timer0Ticks++;
    
    // 處理所有註冊的LED
    for (i = 0; i < g_LEDCount; i++) {
        // 如果LED有等待中的延遲
        if (g_LEDs[i].delayCount > 0) {
            g_LEDs[i].delayCount--;
            if (g_LEDs[i].delayCount == 0) {
                g_LEDs[i].delayComplete = 1;
            }
        }
    }
}

/**
 * @brief    Timer0 初始化函數
 * @param    無
 * @return   無
 */
void Timer0_Init(void)
{
    /* 啟用 Timer0 時鐘 */
    CLK_EnableModuleClock(TMR0_MODULE);
    
    /* 選擇 Timer0 時鐘源為 HIRC */
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);
    
    /* 初始化 Timer0 為週期模式，頻率 100Hz (10ms) */
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 100); 
    
    /* 啟用 Timer0 中斷 */
    TIMER_EnableInt(TIMER0);
    
    /* 啟用 Timer0 的 NVIC 中斷 */
    NVIC_EnableIRQ(TMR0_IRQn);
    
    /* 啟動 Timer0 */
    TIMER_Start(TIMER0);
}

/**
 * @brief    更新所有LED狀態
 * @param    無
 * @return   無
 */
void LED_UpdateAll(void)
{
    uint8_t i;
    
    for (i = 0; i < g_LEDCount; i++) {
        // 檢查是否需要更新LED狀態
        if (g_LEDs[i].delayComplete) {
            // 切換LED狀態
            g_LEDs[i].state = !g_LEDs[i].state;
            
            // 設置LED的電平
            if (g_LEDs[i].state) {
                // LED亮起 (低電平)
                g_LEDs[i].port->DOUT &= ~g_LEDs[i].pin;
                g_LEDs[i].delayCount = g_LEDs[i].onTime / 10;
                printf("%s ON, delay %ums, Ticks: %u\n", g_LEDs[i].name, g_LEDs[i].onTime, g_u32Timer0Ticks);
            } else {
                // LED關閉 (高電平)
                g_LEDs[i].port->DOUT |= g_LEDs[i].pin;
                g_LEDs[i].delayCount = g_LEDs[i].offTime / 10;
                printf("%s OFF, delay %ums, Ticks: %u\n", g_LEDs[i].name, g_LEDs[i].offTime, g_u32Timer0Ticks);
            }
            
            // 重置延遲完成標誌
            g_LEDs[i].delayComplete = 0;
        }
    }
}

/**
 * @brief    初始化按鈕GPB0為輸入模式
 * @param    無
 * @return   無
 */
void Button_Init(void)
{
    // 啟用 GPB 時鐘
    CLK_EnableModuleClock(GPB_MODULE);
    
    // 設置 GPB0 為輸入模式
    GPIO_SetMode(PB, BIT0, GPIO_MODE_QUASI);
    
    printf("Button initialization complete (GPB0)\n");
}

/**
 * @brief    檢測按鈕是否被按下 (帶防抖功能)
 * @param    無
 * @return   如果按鈕被按下並通過防抖檢測，則返回1，否則返回0
 */
uint8_t Button_IsPressed(void)
{
    uint8_t currentState = (PB->PIN & BIT0) ? 1 : 0; // 讀取按鈕狀態
    
    // 如果按鈕狀態從高變低 (按下) 且經過足夠的防抖時間
    if (g_buttonPrevState == 1 && currentState == 0 && 
        (g_u32Timer0Ticks - g_lastButtonPressTime) > DEBOUNCE_TIME) {
        
        g_lastButtonPressTime = g_u32Timer0Ticks; // 更新按下時間
        g_buttonPrevState = currentState; // 更新按鈕狀態
        return 1; // 按鈕被按下
    }
    
    // 更新按鈕狀態
    g_buttonPrevState = currentState;
    return 0; // 按鈕未被按下或防抖期間
}

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable Internal RC 24 MHz clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC and HXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL2_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set GPB multi-function pins for UART0 RXD and TXD */
    Uart0DefaultMPF();

    /* Lock protected registers */
    SYS_LockReg();
}


/*
 * This is a template project for M2003 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */
int main()
{
    SYS_Init();
    
    /* 初始化 Timer0 並啟用中斷 */
    Timer0_Init();

    /* Init UART to 115200-8n1 for print message */
    UART_Open(UART0, 115200);

    /* Enable GPB clock */
    CLK_EnableModuleClock(GPB_MODULE);
    
    /* Connect UART to PC, and open a terminal tool to receive following message */
    printf("123 Hello World 567\n");
    printf("LED control system initialized\n");
    
    /* 初始化按鈕 */
    Button_Init();
    
    /* 初始化WS2812B */
    WS2812B_Init();
    
    /* 設置WS2812B LED數量 (預設為5) */
    WS2812B_SetLEDCount(5);
    
    /* 顯示全紅色 */
    // RGB_t redColor = {0, 0, 255}; // 紅色 (R=255, G=0, B=0)
    // WS2812B_ShowSolidColor(redColor);

    WS2812B_ShowRainbow();
    
    /* 註冊LED */
    LED_Register(PB, BIT1, 200, 1000, "Red LED");     // 紅色LED：亮200ms，暗1000ms
    LED_Register(PB, BIT7, 100, 3000, "Green LED");   // 綠色LED：亮100ms，暗3000ms
    
    /* 設置LED的替代閃爍模式 */
    LED_SetAlternateTiming(0, 200, 200);     // PB1替代模式：亮200ms，暗200ms
    LED_SetAlternateTiming(1, 1000, 1000);   // PB7替代模式：亮1000ms，暗1000ms
    
    /* 用於追踪上次旋轉彩虹的時間 */
    uint32_t lastRainbowShiftTime = g_u32Timer0Ticks;

    /* 使用無限迴圈控制所有LED */
    while (1) {
        /* 更新所有LED狀態 */
        LED_UpdateAll();
        
        /* 檢查按鈕是否被按下，如果是則切換模式 */
        if (Button_IsPressed()) {
            printf("Button pressed, switching LED mode\n");
            LED_SwitchMode();
        }
        
        /* 每間隔1秒旋轉彩虹顏色一次 (100個ticks = 1秒，因為Timer0是100Hz) */
        if ((g_u32Timer0Ticks - lastRainbowShiftTime) >= 50) {
            WS2812B_ShiftRainbow();  // 旋轉彩虹色
            lastRainbowShiftTime = g_u32Timer0Ticks;  // 更新時間
        }
        
        /* 這裡可以做其他事情，CPU 不會被阻塞 */
        // 例如處理通信等
    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
