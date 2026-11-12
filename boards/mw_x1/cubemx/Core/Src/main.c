/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hal_board.h"
#include "hal_gpio.h"
#include "hal_buzzer.h"
#include "hal_time.h"
#include "hal_serial.h"
#include "hal_i2c.h"
#include "hal_spi.h"
#include "hal_adc.h"
#include "core_selftest.h"
#include "core_main.h"
#include "hal_watchdog.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile int mc_boot_safe_ok = 0;  // 1 = حالت امن تأیید شد
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  mc_board_safe_state_c();  // اولین قدم: خروجی‌ها خاموش
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_USB_DEVICE_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM9_Init();
  MX_TIM6_Init();
  MX_SPI3_Init();
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  /* USER CODE BEGIN 2 */
  mc_board_safe_state_c();  // دوباره، چون MX_TIM پین هیتر را AF کرده بود
  mc_boot_safe_ok = mc_board_safe_state_check_c();
  const int mc_selftest_ok = mc_gpio_selftest_c();   // نتیجه در mc_gpio_selftest_ok هم هست
  mc_boot_safe_ok = mc_board_safe_state_check_c();   // بعد از تست دوباره حالت امن را چک می‌کنیم
  mc_time_init_c();                                  // راه‌اندازی زمان‌پایه (TIM6)
  const int mc_time_ok = mc_time_selftest_c();       // نتیجه در mc_time_selftest_ok هم هست
  mc_i2c_selftest_c();   // نتیجه در mc_i2c_selftest_ok
  mc_spi_selftest_c();   // نتیجه در mc_spi_selftest_ok
  mc_adc_selftest_c();   // نتیجه در mc_adc_selftest_ok
  mc_ring_selftest_c();  // نتیجه در mc_ring_selftest_ok
  mc_sched_selftest_c();  // نتیجه در mc_sched_selftest_ok
  mc_core_init_c();       // ثبت وظیفه‌های هسته
  // کد صوتی راه‌اندازی: همه سالم = بوق Boot ، مشکل در هر کدام = بوق Error
  if ((mc_boot_safe_ok == 1) && (mc_selftest_ok == 1) && (mc_time_ok == 1)) {
    mc_buzzer_play_c(MC_SOUND_BOOT);
  } else {
    mc_buzzer_play_c(MC_SOUND_ERROR);
  }
  mc_watchdog_start_c(3000U);  // Watchdog با تایم‌اوت ۳ ثانیه؛ تغذیه‌اش در زمان‌بند هسته انجام می‌شود
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
    mc_gpio_debug_poll_c();
    mc_buzzer_test_poll_c();   // تست موقت بیزر با کلیدهای روی برد
    mc_time_debug_poll_c();    // به‌روزرسانی متغیرهای دیباگ زمان
    mc_serial_poll_c();        // پیام شروع و echo ارتباط USB
    mc_adc_debug_poll_c();     // به‌روزرسانی متغیرهای دیباگ ADC
    mc_core_loop_c();          // اجرای زمان‌بند هسته
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// وقفه تایمرها: زمان‌پایه (TIM6) هر ۱۰۰ میکروثانیه اینجا می‌آید.
// این تابع باید خیلی کوتاه باشد؛ هیچ‌وقت HAL_Delay یا کار سنگین اینجا نگذار.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    mc_time_tick_c();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
