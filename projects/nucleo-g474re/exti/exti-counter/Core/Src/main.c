/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : External interrupt counter
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
  Σε αυτό το παράδειγμα εκτελούνται δύο ρουτίνες εξυπηρέτησης διακοπών που προκαλούνται
  από τις γραμμές 1(PB1) και 13(PC13) από τα κουμπιά B1 και User Button αντίστοιχα.
  Αν πατήσω το κουμμπί Β1 δημιουργείται αίτημα exti1 διακοπής και εκτελείται η blink() 3 φορές.
  Αν πατήσω το κουμπί User Button δημιουργείται αίτημα exti διακοπής και εκτελείται η UserButton_Callback()
  που στην ουσία αυξάνει έναν μετρητή στην PCA_9555_OUTPUT_0 extended port.
  Αν όμως έχω πατημένο το κουμπί PB0, όσο και να πατήσω το User Button δεν αυξάνει ο μετρητής.
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32g4xx_ll_i2c.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_utils.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * Ο μετρητής χρησιμοποιεί 5 bits:
 *
 * 0x1F = 0001 1111
 *
 * Επομένως οι επιτρεπόμενες τιμές είναι 0 έως 31.
 */
#define COUNTER_MASK  0x1FU

/* Address of PCA9555 (A0-A2 grounded -> 0x40) */
#define PCA9555_ADDRESS 0x40

/* Registers of PCA9555 */
typedef enum {
    REG_INPUT_0 = 0,
	REG_INPUT_1 = 1,
    REG_OUTPUT_0 = 2,
	REG_OUTPUT_1 = 3,
    REG_POLARITY_INV_0 = 4,
	REG_POLARITY_INV_1 = 5,
    REG_CONFIGURATION_0 = 6,
	REG_CONFIGURATION_1 = 7
} PCA9555_REGISTERS;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/*
 * Μετρητής των εξωτερικών διακοπών.
 *
 * Το volatile είναι απαραίτητο επειδή η μεταβλητή
 * τροποποιείται μέσα από ρουτίνα διακοπής.
 */
volatile uint8_t counter = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

void UserButton_Callback(void);
void PCA9555_Write(uint8_t reg, uint8_t value);
uint8_t PCA9555_Read(uint8_t reg);
void blink(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Εμφανίζει την τιμή του μετρητή στα EXT_PORT0[4:0] του PCA9555.
  * @param  value: Τιμή από 0 έως 31.
  * @retval None
  */
static void DisplayCounter(uint8_t value)
{
    uint8_t output_value;

    output_value = value & COUNTER_MASK;

    PCA9555_Write(REG_OUTPUT_0, output_value);
}

// Write "value" to "reg" of PCA9555
void PCA9555_Write(uint8_t reg, uint8_t value)
{
	// Start the transfer. Use I2C1, transmit 2 bytes in total, send stop automatically when the transmission is over and we want to write
    LL_I2C_HandleTransfer(I2C1, PCA9555_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

    // Send the PCA register address (reg)
    while(!LL_I2C_IsActiveFlag_TXIS(I2C1)); // Wait until the buffer is empty
    LL_I2C_TransmitData8(I2C1, reg);

    // Send the data (value)
    while(!LL_I2C_IsActiveFlag_TXIS(I2C1)); // Wait until the buffer is empty
    LL_I2C_TransmitData8(I2C1, value);

    // Wait for the Stop flag
    while(!LL_I2C_IsActiveFlag_STOP(I2C1));
    LL_I2C_ClearFlag_STOP(I2C1); // Clear the Stop flag
}

// Read a value from "reg" of PCA9555
uint8_t PCA9555_Read(uint8_t reg)
{
    uint8_t received_data = 0;

    // 1st phase
    // Start the transfer. Use I2C1, transmit 1 byte in total (the register), don't stop yet and we want to write
    LL_I2C_HandleTransfer(I2C1, PCA9555_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

    while(!LL_I2C_IsActiveFlag_TXIS(I2C1)); // Wait until the buffer is empty
    LL_I2C_TransmitData8(I2C1, reg);        // Send the PCA register address (reg)

    while(!LL_I2C_IsActiveFlag_TC(I2C1));   // Wait until the transfer is complete

    // 2nd phase
    // Restart the transfer. Use I2C1, receive 1 byte in total, send stop automatically when the transmission is over and we want to read
    LL_I2C_HandleTransfer(I2C1, PCA9555_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

    while(!LL_I2C_IsActiveFlag_RXNE(I2C1)); // Wait until the data arrives
    received_data = LL_I2C_ReceiveData8(I2C1); // Read the data

    // Wait for the Stop flag
    while(!LL_I2C_IsActiveFlag_STOP(I2C1));
    LL_I2C_ClearFlag_STOP(I2C1); // Clear the Stop flag

    return received_data; // Return the data
}

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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  // Set EXT_PORT0 as output
  PCA9555_Write(REG_CONFIGURATION_0, 0x00);

  /*
   * Αρχική εμφάνιση:
   *
   * counter = 0
   * EXT_PORT0[4:0] = 00000
   */
  DisplayCounter(counter);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /*
     * Η κύρια ρουτίνα δεν χρειάζεται να εκτελεί κάτι.
     *
     * Το πρόγραμμα περιμένει την εξωτερική διακοπή
     * που προκαλείται από το πλήκτρο PC13.
     */

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

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  /**I2C1 GPIO Configuration
  PA15   ------> I2C1_SCL
  PB9   ------> I2C1_SDA
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */

  /** I2C Initialization
  */
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x40B285C2;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_EnableAutoEndMode(I2C1);
  LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
  LL_I2C_DisableOwnAddress2(I2C1);
  LL_I2C_DisableGeneralCall(I2C1);
  LL_I2C_EnableClockStretching(I2C1);
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE1);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_13;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_1;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_NO);

  /**/
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_1, LL_GPIO_PULL_NO);

  /**/
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);

  /**/
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_1, LL_GPIO_MODE_INPUT);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  NVIC_SetPriority(EXTI1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(EXTI1_IRQn);
  NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief  Εκτελείται μετά από διακοπή του PC13.
  * @retval None
  */
void UserButton_Callback(void)
{
  /*
   * Το PB1 είναι active-high,
   * διακόπτη του ntuAg2board
   * ωστε να έχω pull-up στο PB1.
   *
   * Αν είναι πατημένο, έχει λογική τιμή 1.
   * Σε αυτή την περίπτωση εγκαταλείπουμε τη
   * συνάρτηση χωρίς να αυξήσουμε τον μετρητή.
   */
  if (LL_GPIO_IsInputPinSet(
          GPIOB,
          LL_GPIO_PIN_0
      ) != 0U)
  {
    return;
  }

  /*
   * Το PB0 δεν είναι πατημένο, επομένως
   * αυξάνουμε τον μετρητή.
   */
  counter++;

  /*
   * Όταν ο μετρητής ξεπεράσει το 31,
   * επιστρέφει στο 0.
   */
  if (counter > 31U)
  {
    counter = 0U;
  }

  /*
   * Εμφάνιση της νέας τιμής στα EXT_PORT0[4:0] του PCA9555.
   */
  DisplayCounter(counter);
}

void blink(void) {
    PCA9555_Write(REG_OUTPUT_0, 0xFF);
    LL_mDelay(500);
    PCA9555_Write(REG_OUTPUT_0, 0x00);
    LL_mDelay(500);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

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

  /*
   * User can add implementation to report the
   * file name and line number.
   */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
