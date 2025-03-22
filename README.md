# Control WS2812B RGB LED using Nuvoton M2003FC

## M2003FC Nuvoton Cortex-M23 MCU with 32KB Flash and 4KB SRAM

### 簡介

這個專案展示了如何使用 Nuvoton M2003FC 微控制器來控制 WS2812B RGB LED。WS2812B 是一種常見的可編程 RGB LED，具有內建控制器，可以通過單線協議進行控制。

### 硬體需求

- Nuvoton M2003FC 開發板
- WS2812B RGB LED
- 連接線

### 軟體需求

- ARM GCC Compiler
- VSCODE IDE
- NuMicro ICP Programming Tool

### 快速開始

1. 將 WS2812B RGB LED 連接到 M2003FC 開發板的 GPB11 引腳。
2. 使用 Keil 或 IAR 開啟專案並編譯。
3. 使用 NuMicro ISP Programming Tool 將編譯好的韌體燒錄到 M2003FC 開發板。
4. 上電並運行，觀察 WS2812B RGB LED 顯示彩虹效果。

### 目錄結構

- `M2003FC/WS2812B/WS2812B.c` - WS2812B 控制函數實現
- `M2003FC/WS2812B/WS2812B.h` - WS2812B 控制函數庫頭文件
- `M2003FC/WS2812B/WS2812B_Demo.c` - 範例程式碼，展示如何使用 WS2812B 控制函數

### 功能介紹

- 初始化 WS2812B 控制引腳
- 設置 WS2812B LED 數量
- 發送 RGB 顏色數據到 WS2812B
- 顯示彩虹效果
- 顯示單一顏色
- 旋轉彩虹顏色

## 部分細節說明

### 初始化 WS2812B 控制引腳

指定  PIN 腳位為 WS2812B 控制引腳

```c
// WS2812B LED控制相關定義
#define WS2812B_PIN      BIT11
#define WS2812B_PORT     PB
#define WS2812B_DO      PB11
```

### 控制 WS2812B LED 數量

```c
// 預設LED數量
#define DEFAULT_WS2812B_COUNT 5
#define MAX_WS2812B_COUNT 30
```

### 顯示彩虹效果

```c
    /* 顯示彩虹效果 */
    WS2812B_ShowRainbow();
```

### 顯示單一顏色

```c
    /* 顯示全綠色 */
    RGB_t greenColor = {0, 255, 0}; // 綠色 (R=0, G=255, B=0)
    WS2812B_ShowSolidColor(greenColor);
```


## 底層控制

### WS2812B Data Format

WS2812B LED 使用 24 位元 RGB 數據格式，每個 LED 需要 3 個顏色通道的數據。數據格式如下：
- 每個顏色通道的數據範圍為 0 到 255
- 數據順序為 G (綠色), R (紅色), B (藍色)
- 數據傳輸順序為 MSB (最高有效位元) 到 LSB (最低有效位元)
- 數據傳輸順序為 G7, G6, ..., G0, R7, R6, ..., R0, B7, B6, ..., B0
- 數據傳輸順序為 0bGGGGGGGGRRRRRRRRBBBBBBBB
- 數據傳輸順序為 0xGGRRBB

### WS2812B Timing

WS2812B LED 使用單線協議進行控制，數據傳輸速率為 800 Kbps。數據傳輸時序如下：

- 0 bit: T0H = 0.4us, T0L = 0.85us
- 1 bit: T1H = 0.8us, T1L = 0.45us
- Reset: Treset > 50us
 
