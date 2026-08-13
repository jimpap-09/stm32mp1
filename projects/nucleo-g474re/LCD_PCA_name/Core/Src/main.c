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
#include "main.h"
#include "stm32g4xx_ll_i2c.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_cortex.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define I2C_INSTANCE I2C1
#define PCA9555_ADDR 0x40
#define PCA_REG_OUTPUT_0  0x02
#define PCA_REG_CONFIG_0  0x06

#define LCD_RS_BIT  0x04 // (1 << 2)
#define LCD_E_BIT   0x08 // (1 << 3)
#define LCD_D4_BIT  0x10
#define LCD_D5_BIT  0x20
#define LCD_D6_BIT  0x40
#define LCD_D7_BIT  0x80
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t pex_data_reg = 0x00;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

void PCA9555_WriteRegister(uint8_t reg, uint8_t value);
void PCA9555_Init(void);

void lcd_write_4bits(uint8_t value);
void lcd_pulse_enable(void);
void lcd_send(uint8_t value, uint8_t mode);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_clear(void);
void lcd_init(void);

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
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

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
  LL_mDelay(100);
  lcd_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  lcd_data('O');
	  lcd_data('K');

	  lcd_command(0xC0); // change line

	  lcd_data('D');
	  lcd_data('I');
	  lcd_data('M');
	  lcd_data('I');
	  lcd_data('T');
	  lcd_data('R');
	  lcd_data('I');
	  lcd_data('S');

	  LL_mDelay(2000);
	  lcd_clear();

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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSI_Enable();
   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  LL_RCC_HSI_SetCalibTrimming(64);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  }

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  LL_Init1msTick(16000000);

  LL_SetSystemCoreClock(16000000);
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
  I2C_InitStruct.Timing = 0x00503D58;
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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void PCA9555_WriteRegister(uint8_t reg, uint8_t value) {
	while(LL_I2C_IsActiveFlag_BUSY(I2C_INSTANCE)); // wait bus to be clear
	// start transfer
	LL_I2C_HandleTransfer(I2C_INSTANCE, PCA9555_ADDR, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

	while(!LL_I2C_IsActiveFlag_TXIS(I2C_INSTANCE));

    LL_I2C_TransmitData8(I2C_INSTANCE, reg);    //send register address

    while(!LL_I2C_IsActiveFlag_TXIS(I2C_INSTANCE));

    LL_I2C_TransmitData8(I2C_INSTANCE, value);  // send data

	while(!LL_I2C_IsActiveFlag_STOP(I2C_INSTANCE));

	LL_I2C_ClearFlag_STOP(I2C_INSTANCE);         // wait for stop condition
}

void PCA9555_Init(void) {
	PCA9555_WriteRegister(PCA_REG_CONFIG_0, 0x00); // pex0 as output

	pex_data_reg = 0x00;
	PCA9555_WriteRegister(PCA_REG_OUTPUT_0, pex_data_reg);
	// initialize output to 0
}

void lcd_write_4bits(uint8_t value) {
	pex_data_reg &= 0x0F; // keep first 4 bits

	if (value & 0x01) pex_data_reg |= LCD_D4_BIT;
	if (value & 0x02) pex_data_reg |= LCD_D5_BIT;
	if (value & 0x04) pex_data_reg |= LCD_D6_BIT;
	if (value & 0x08) pex_data_reg |= LCD_D7_BIT;
	// send 4 bits to I2C
	PCA9555_WriteRegister(PCA_REG_OUTPUT_0, pex_data_reg);

}

void lcd_pulse_enable(void) {
	pex_data_reg |= LCD_E_BIT;
	PCA9555_WriteRegister(PCA_REG_OUTPUT_0, pex_data_reg);
	LL_mDelay(1);

	pex_data_reg &= ~LCD_E_BIT;
	PCA9555_WriteRegister(PCA_REG_OUTPUT_0, pex_data_reg);
	LL_mDelay(1);

}

void lcd_send(uint8_t value, uint8_t mode) {

	if (mode == 1) {
	  pex_data_reg |= LCD_RS_BIT; // for data mode
	}
	else {
		pex_data_reg &= ~LCD_RS_BIT; // for command mode
	}
	 // send RS before data
	PCA9555_WriteRegister(PCA_REG_OUTPUT_0, pex_data_reg);

	lcd_write_4bits((value >> 4) & 0x0F); // send 4 high bits
	lcd_pulse_enable();

	lcd_write_4bits(value & 0x0F); // send 4 low bits
	lcd_pulse_enable();
}

void lcd_command(uint8_t cmd) {
    lcd_send(cmd, 0);
}

void lcd_data(uint8_t data) {
    lcd_send(data, 1);
}

void lcd_clear(void) {
    lcd_command(0x01);
    LL_mDelay(5);
}

void lcd_init(void) {
	PCA9555_Init(); // initialize PCA first
	LL_mDelay(50);

	// RS=0,E=0
	pex_data_reg &= ~(LCD_RS_BIT | LCD_E_BIT);
	PCA9555_WriteRegister(PCA_REG_OUTPUT_0, pex_data_reg);

	lcd_write_4bits(0x03);
	    lcd_pulse_enable();
	    LL_mDelay(5);

	    lcd_write_4bits(0x03);
	    lcd_pulse_enable();
	    LL_mDelay(1);

	    lcd_write_4bits(0x03);
	    lcd_pulse_enable();
	    LL_mDelay(1);

	    lcd_write_4bits(0x02); // 4-bit mode
	    lcd_pulse_enable();
	    LL_mDelay(1);


	    lcd_command(0x28); // 4-bit, 2 lines, 5x8
	    lcd_command(0x0C); // Display ON, Cursor OFF
	    lcd_command(0x06); // Increment cursor
	    lcd_clear();
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
