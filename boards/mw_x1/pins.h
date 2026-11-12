#pragma once
// فقط پوشه boards مجاز به include کردن هدرهای ST است
#include "stm32f4xx_hal.h"

/* =====================================================================
                 Pin Map Mks Robin Nano V3.1 Board
                      Code Version : V1.0.0
                        MCU : STM32F407VGT6
                      Developer : Amir Gharzi
                      Last Update : 1405/08/10
===================================================================== */

/*----- Axis Motors ---------------------------------------------------*/

//--------- Axis_X ---------
// Driver IC       :A4988

#define MC_X_STEP_PORT                 GPIOE
#define MC_X_STEP_PIN                  GPIO_PIN_3
#define MC_X_DIR_PORT                  GPIOE
#define MC_X_DIR_PIN                   GPIO_PIN_2
#define MC_X_EN_PORT                   GPIOE
#define MC_X_EN_PIN                    GPIO_PIN_4

//--------- Axis_Y ---------
// Driver IC       :A4988

#define MC_Y_STEP_PORT                 GPIOE
#define MC_Y_STEP_PIN                  GPIO_PIN_0
#define MC_Y_DIR_PORT                  GPIOB
#define MC_Y_DIR_PIN                   GPIO_PIN_9
#define MC_Y_EN_PORT                   GPIOE
#define MC_Y_EN_PIN                    GPIO_PIN_1

//--------- Axis_Z ---------
// Driver IC       :A4988

#define MC_Z_STEP_PORT                 GPIOB
#define MC_Z_STEP_PIN                  GPIO_PIN_5
#define MC_Z_DIR_PORT                  GPIOB
#define MC_Z_DIR_PIN                   GPIO_PIN_4
#define MC_Z_EN_PORT                   GPIOB
#define MC_Z_EN_PIN                    GPIO_PIN_8

/*----- Axis Limits ---------------------------------------------------*/

//--------- Axis_X ---------
// Limit Type      : NC
// Active Level    : HIGH (NC: عادی LOW، فعال یا قطع‌سیم HIGH؛ تست‌شده روی برد)

#define MC_X_LIMIT_MIN_PORT            GPIOA
#define MC_X_LIMIT_MIN_PIN             GPIO_PIN_15

//--------- Axis_Y ---------
// Limit Type      : NC
// Active Level    : HIGH (NC: عادی LOW، فعال یا قطع‌سیم HIGH؛ تست‌شده روی برد)

#define MC_Y_LIMIT_MIN_PORT            GPIOD
#define MC_Y_LIMIT_MIN_PIN             GPIO_PIN_2

//--------- Axis_Z ---------
// Limit Type      : NC
// Active Level    : HIGH (NC: عادی LOW، فعال یا قطع‌سیم HIGH؛ تست‌شده روی برد)

#define MC_Z_LIMIT_MIN_PORT            GPIOC
#define MC_Z_LIMIT_MIN_PIN             GPIO_PIN_8

/*----- LCD -----------------------------------------------------------*/

// Interface : SPI1
// Touch IC  : ADS7843E
// LCD IC    : ST7796S
// Resolution: 480x320 (landscape, MADCTL 0x28, inversion ON)
// LCD MKS TS35 V2

//--------- LCD ---------
#define MC_LCD_SCK_PORT                GPIOA         // SPI1_SCK
#define MC_LCD_SCK_PIN                 GPIO_PIN_5
#define MC_LCD_MISO_PORT               GPIOA         // SPI1_MISO
#define MC_LCD_MISO_PIN                GPIO_PIN_6
#define MC_LCD_MOSI_PORT               GPIOA         // SPI1_MOSI
#define MC_LCD_MOSI_PIN                GPIO_PIN_7
#define MC_LCD_CS_PORT                 GPIOD         // LCD chip select
#define MC_LCD_CS_PIN                  GPIO_PIN_11
#define MC_LCD_RS_PORT                 GPIOD         // Data/Command (RS)
#define MC_LCD_RS_PIN                  GPIO_PIN_10
#define MC_LCD_RES_PORT                GPIOC         // Reset
#define MC_LCD_RES_PIN                 GPIO_PIN_6
#define MC_LCD_BLK_PORT                GPIOD         // Backlight (HIGH = on)
#define MC_LCD_BLK_PIN                 GPIO_PIN_13

//--------- Touch ---------
// Touch IC        : ADS7843E
// Calibration     : TODO
#define MC_TOUCH_CS_PORT               GPIOE         // Touch chip select
#define MC_TOUCH_CS_PIN                GPIO_PIN_14
#define MC_TOUCH_IRQ_PORT              GPIOE         // PEN, active-LOW, pull-up
#define MC_TOUCH_IRQ_PIN               GPIO_PIN_15

//--------- Encoder ---------
// Steps/Click     : TODO
// Direction Invert: TODO
#define MC_ENC_A_PORT                  GPIOE
#define MC_ENC_A_PIN                   GPIO_PIN_8
#define MC_ENC_B_PORT                  GPIOE
#define MC_ENC_B_PIN                   GPIO_PIN_11
#define MC_ENC_BTN_PORT                GPIOE         // Push button, active-LOW
#define MC_ENC_BTN_PIN                 GPIO_PIN_13

//--------- Buzzer ---------
// Active Level    : HIGH
// Type            : Active
#define MC_BUZZER_PORT                 GPIOC
#define MC_BUZZER_PIN                  GPIO_PIN_5

/*----- Extruder ------------------------------------------------------*/

//--------- Motor ---------
// Driver IC       :A4988
// Step Active     : لبه بالارونده (A4988)
// Dir Active      : TODO
// Enable Active   : LOW (فعال با LOW، پیش‌فرض غیرفعال توسط پول‌آپ روی برد)
// نام‌های EX_ قبلاً تغییر کرده‌اند
#define MC_EX_STEP_PORT                 GPIOD
#define MC_EX_STEP_PIN                  GPIO_PIN_6
#define MC_EX_DIR_PORT                  GPIOD
#define MC_EX_DIR_PIN                   GPIO_PIN_3
#define MC_EX_EN_PORT                   GPIOB
#define MC_EX_EN_PIN                    GPIO_PIN_3

//--------- Fan_Filament ---------
// Active Level    : HIGH (ماسفت N، پایین‌کش روی گیت)
// PWM Timer       : ندارد (PC14 تایمر سخت‌افزاری ندارد؛ PWM نرم‌افزاری)
#define MC_FAN_FILAMENT_PORT           GPIOC
#define MC_FAN_FILAMENT_PIN            GPIO_PIN_14

//--------- Bltouch Sensor ---------
// Servo Timer/Channel: TODO
// Probe Active Level: TODO
#define MC_BLTOUCH_SERVO_PORT          GPIOA         // Servo control (PWM)
#define MC_BLTOUCH_SERVO_PIN           GPIO_PIN_8
#define MC_BLTOUCH_KEY_PORT            GPIOC         // Probe signal
#define MC_BLTOUCH_KEY_PIN             GPIO_PIN_4

//--------- Filament Sensor ---------
// Active Level    : HIGH
#define MC_FILAMENT_SENSOR_PORT        GPIOA
#define MC_FILAMENT_SENSOR_PIN         GPIO_PIN_4

//--------- Heater ---------
// Active Level    : HIGH (ماسفت N، پایین‌کش روی گیت، هنگام ریست خاموش)
// Max Temp        : TODO
// PWM / Bang-Bang : TODO
#define MC_HEATER_EX_PORT              GPIOE
#define MC_HEATER_EX_PIN               GPIO_PIN_5

//--------- Sensor_Temp ---------
// Sensor Type     : Thermistor : TODO
// ADC Channel     : ADC1_IN11 (PC1) - در CubeMX تأیید شود
// Pull-up Value   : TODO
#define MC_SENSOR_TEMP_EX_PORT         GPIOC
#define MC_SENSOR_TEMP_EX_PIN          GPIO_PIN_1

/*----- Bed -----------------------------------------------------------*/

//--------- Heater ---------
// Active Level    : HIGH (ماسفت N، پایین‌کش روی گیت، هنگام ریست خاموش)
// Max Temp        : TODO
// PWM / Bang-Bang : TODO
#define MC_HEATER_BED_PORT             GPIOA
#define MC_HEATER_BED_PIN              GPIO_PIN_0

//--------- Sensor_Temp ---------
// Sensor Type     : Thermistor : TODO
// ADC Channel     : ADC1_IN10 (PC0) - در CubeMX تأیید شود
// Pull-up Value   : TODO
#define MC_SENSOR_TEMP_BED_PORT        GPIOC
#define MC_SENSOR_TEMP_BED_PIN         GPIO_PIN_0

/*----- Wifi ----------------------------------------------------------*/

// Module IC       : ESP8266 / ESP32 : TODO
// Baud Rate       : TODO

//--------- USART1 ---------
#define MC_RX_WIFI_PORT                GPIOA
#define MC_RX_WIFI_PIN                 GPIO_PIN_10
#define MC_TX_WIFI_PORT                GPIOA
#define MC_TX_WIFI_PIN                 GPIO_PIN_9
#define MC_IO0_WIFI_PORT               GPIOC
#define MC_IO0_WIFI_PIN                GPIO_PIN_13
#define MC_IO1_WIFI_PORT               GPIOC
#define MC_IO1_WIFI_PIN                GPIO_PIN_7
#define MC_CTRL_WIFI_PORT              GPIOE         // Wifi RST
#define MC_CTRL_WIFI_PIN               GPIO_PIN_9    // Wifi RST

//--------- SPI2 ---------
// NOTE: SCK / MISO / MOSI are shared with the Flash IC (SPI2)
#define MC_CS_WIFI_PORT                GPIOC
#define MC_CS_WIFI_PIN                 GPIO_PIN_15
#define MC_SCK_WIFI_PORT               GPIOB
#define MC_SCK_WIFI_PIN                GPIO_PIN_13
#define MC_MISO_WIFI_PORT              GPIOC
#define MC_MISO_WIFI_PIN               GPIO_PIN_2
#define MC_MOSI_WIFI_PORT              GPIOC
#define MC_MOSI_WIFI_PIN               GPIO_PIN_3

/*----- USB_Usart -----------------------------------------------------*/

// Mode            : Device / CDC : Device (CDC)
// Speed           : TODO
#define MC_USB_USART_DN_PORT           GPIOA         // USB D-
#define MC_USB_USART_DN_PIN            GPIO_PIN_11
#define MC_USB_USART_DP_PORT           GPIOA         // USB D+
#define MC_USB_USART_DP_PIN            GPIO_PIN_12
// غیرفعال: PA14 همان SWCLK (ST-Link) است. تا تأیید کاربرد از روی نقشه برد استفاده نشه.
// #define MC_USB_USART_CTRL_PORT         GPIOA
// #define MC_USB_USART_CTRL_PIN          GPIO_PIN_14

/*----- USB_OTG -------------------------------------------------------*/

// Mode            : Host / Device : Host (فلش USB)
// Speed           : TODO
#define MC_USB_OTG_DM_PORT             GPIOB         // USB D-
#define MC_USB_OTG_DM_PIN              GPIO_PIN_14
#define MC_USB_OTG_DP_PORT             GPIOB         // USB D+
#define MC_USB_OTG_DP_PIN              GPIO_PIN_15

/*----- SD Card -------------------------------------------------------*/

// Interface       : SPI / SDIO : SPI (SPI3)
// Filesystem      : FatFs : TODO
// Detect Active Level: TODO (روی برد پول‌آپ ۱۰K؛ احتمالاً با کارت LOW می‌شود، باید تست شود)
#define MC_SD_CS_PORT                  GPIOC
#define MC_SD_CS_PIN                   GPIO_PIN_9
#define MC_SD_MOSI_PORT                GPIOC
#define MC_SD_MOSI_PIN                 GPIO_PIN_12
#define MC_SD_SCK_PORT                 GPIOC
#define MC_SD_SCK_PIN                  GPIO_PIN_10
#define MC_SD_MISO_PORT                GPIOC
#define MC_SD_MISO_PIN                 GPIO_PIN_11
#define MC_SD_DET_PORT                 GPIOD         // Card detect
#define MC_SD_DET_PIN                  GPIO_PIN_12

/*----- IC Memory -----------------------------------------------------*/

//--------- Flash (W25Q64JVSSIQ) ---------
// Interface       : SPI2
// Capacity        : 64 Mbit
// Usage           : TODO
#define MC_FL_CS_PORT                  GPIOB
#define MC_FL_CS_PIN                   GPIO_PIN_12
#define MC_FL_MOSI_PORT                GPIOC
#define MC_FL_MOSI_PIN                 GPIO_PIN_3
#define MC_FL_SCK_PORT                 GPIOB
#define MC_FL_SCK_PIN                  GPIO_PIN_13
#define MC_FL_MISO_PORT                GPIOC
#define MC_FL_MISO_PIN                 GPIO_PIN_2

//--------- EEPROM (AT24C32D) ---------
// Interface       : I2C
// I2C Address     : 0x50 (7 بیتی؛ A0 تا A2 به GND)
// Capacity        : 32 Kbit
#define MC_EEPROM_SCL_PORT             GPIOB
#define MC_EEPROM_SCL_PIN              GPIO_PIN_6
#define MC_EEPROM_SDA_PORT             GPIOB
#define MC_EEPROM_SDA_PIN              GPIO_PIN_7

/*----- Keys ----------------------------------------------------------*/

// Active Level    : LOW (فشرده = LOW؛ تست‌شده روی برد)
// Pull Config     : START: پول‌آپ بیرونی روی برد. STOP: پایین‌کش بیرونی 100K (R5)، پس pull-up داخلی MCU باید فعال شود.
#define MC_KEY_START_PORT              GPIOE         // Start key
#define MC_KEY_START_PIN               GPIO_PIN_6
#define MC_KEY_STOP_PORT               GPIOB         // Stop key
#define MC_KEY_STOP_PIN                GPIO_PIN_2

/*----- STLINK v2 (SWD) -----------------------------------------------*/

#define MC_SWDIO_PORT                  GPIOA
#define MC_SWDIO_PIN                   GPIO_PIN_13
#define MC_SWCLK_PORT                  GPIOA
#define MC_SWCLK_PIN                   GPIO_PIN_14
