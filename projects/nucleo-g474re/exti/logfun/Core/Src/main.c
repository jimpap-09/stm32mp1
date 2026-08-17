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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define F0_PORT GPIOB
#define F1_PORT GPIOC

#define F0_LED_MASK (LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | \
                     LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | \
                     LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | \
                     LL_GPIO_PIN_6 | LL_GPIO_PIN_7)

#define F1_LED_MASK (LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | \
                     LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | \
                     LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | \
                     LL_GPIO_PIN_6 | LL_GPIO_PIN_7)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t A_values[6];
volatile uint8_t B_values[6];
volatile uint8_t C_values[6];
volatile uint8_t D_values[6];
volatile uint8_t F0_values[6];
volatile uint8_t F1_values[6];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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
	uint8_t A = 0x52;
	uint8_t B = 0x42;
	uint8_t C = 0x22;
	uint8_t D = 0x02;

	uint8_t F0;
	uint8_t F1;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /** Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
  */
  LL_PWR_DisableUCPDDeadBattery();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  for (uint8_t i = 0; i < 6; i++)
  {
      /*
       * Αποθήκευση των τιμών της συγκεκριμένης επανάληψης
       * ώστε να μπορούν να παρακολουθηθούν στο debugger.
       */
      A_values[i] = A;
      B_values[i] = B;
      C_values[i] = C;
      D_values[i] = D;

      /*
       * F0 = (A' · B + B' · D)'
       *
       * ~  : bitwise NOT
       * &  : bitwise AND
       * |  : bitwise OR
       */
      F0 = (uint8_t)~(
              ((uint8_t)(~A) & B) |
              ((uint8_t)(~B) & D)
           );

      /*
       * F1 = (A + C) · (B + D)
       *
       * Στη λογική άλγεβρα:
       * + σημαίνει OR
       * · σημαίνει AND
       */
      F1 = (uint8_t)((A | C) & (B | D));

      F0_values[i] = F0;
      F1_values[i] = F1;

      /*
       * Σβήσιμο των LEDs PB0-PB7 και PC0-PC7.
       */
      LL_GPIO_ResetOutputPin(F0_PORT, F0_LED_MASK);
      LL_GPIO_ResetOutputPin(F1_PORT, F1_LED_MASK);

      /*
       * Εμφάνιση του F0 στα PB0-PB7
       * και του F1 στα PC0-PC7.
       *
       * Επειδή οι τιμές είναι 8 bit και χρησιμοποιούνται
       * τα pins 0 έως 7, η τιμή μπορεί να χρησιμοποιηθεί
       * απευθείας ως pin mask.
       */
      LL_GPIO_SetOutputPin(F0_PORT, (uint32_t)F0);
      LL_GPIO_SetOutputPin(F1_PORT, (uint32_t)F1);

      /*
       * Παραμονή του αποτελέσματος στα LEDs για 2 sec.
       */
      LL_mDelay(2000);

      /*
       * Αύξηση των μεταβλητών για την επόμενη επανάληψη.
       */
      A = (uint8_t)(A + 0x01);
      B = (uint8_t)(B + 0x02);
      C = (uint8_t)(C + 0x03);
      D = (uint8_t)(D + 0x04);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4)
  {
  }
  LL_PWR_EnableRange1BoostMode();
  LL_RCC_HSI_Enable();
   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  LL_RCC_HSI_SetCalibTrimming(64);
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_4, 85, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_EnableDomain_SYS();
  LL_RCC_PLL_Enable();
   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {
  }

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);
   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }

  /* Insure 1us transition state at intermediate medium speed clock*/
  for (__IO uint32_t i = (170 >> 1); i !=0; i--);

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  LL_Init1msTick(170000000);

  LL_SetSystemCoreClock(170000000);
}

/* USER CODE BEGIN 4 */

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
