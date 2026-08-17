/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Keypad 4x4 & LCD 2x16 via PCA9555 (Exercise 6.2)
  ******************************************************************************
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
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_cortex.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    REG_INPUT_0         = 0,
    REG_INPUT_1         = 1,
    REG_OUTPUT_0        = 2,
    REG_OUTPUT_1        = 3,
    REG_POLARITY_INV_0  = 4,
    REG_POLARITY_INV_1  = 5,
    REG_CONFIGURATION_0 = 6,
    REG_CONFIGURATION_1 = 7
} PCA9555_REGISTERS;

/* LCD bit definitions connected to PCA9555 Port 0 (pex0) */
#define LCD_RS_BIT  0x04 // IO0_2
#define LCD_E_BIT   0x08 // IO0_3
#define LCD_D4_BIT  0x10 // IO0_4
#define LCD_D5_BIT  0x20 // IO0_5
#define LCD_D6_BIT  0x40 // IO0_6
#define LCD_D7_BIT  0x80 // IO0_7
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2C_INSTANCE      I2C1
#define PCA9555_ADDRESS   0x40
#define NO_KEY_PRESSED    0xFFFF
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t IO0 = 0x00;
uint16_t pressed_keys = NO_KEY_PRESSED;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
/* PCA9555 Hardware Layer */
void PCA9555_Write(uint8_t reg, uint8_t value);
uint8_t PCA9555_Read(uint8_t reg);
void PCA9555_Init(void);

/* LCD Driver Functions (4-bit mode on Port 0) */
void lcd_write_4bits(uint8_t value);
void lcd_pulse_enable(void);
void lcd_send(uint8_t value, uint8_t mode);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_clear(void);
void lcd_init(void);

/* Keypad 4x4 Functions (Port 1) */
int scan_row(int row);
uint16_t scan_keypad(void);
void scan_keypad_rising_edge(void);
char keypad_to_ascii(uint16_t key_code);
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
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

  /** Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
  LL_PWR_DisableUCPDDeadBattery();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  LL_mDelay(100);
  lcd_init();

  char prev_key = '\0';
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 1. Σάρωση πληκτρολογίου με debouncing και rising-edge detection
      scan_keypad_rising_edge();
      char key = keypad_to_ascii(pressed_keys);

      // 2. Εμφάνιση του νέου πλήκτρου στην LCD
      if (key != '\0' && key != prev_key)
      {
          lcd_clear();        // Καθαρίζει την οθόνη
          lcd_data(key);      // Εμφανίζει τον χαρακτήρα στην 1η θέση
          prev_key = key;     // Αποθηκεύει το τελευταίο πατημένο πλήκτρο
      }
      else if (key == '\0')
      {
          prev_key = '\0';    // Επαναφορά όταν ελευθερωθεί ο διακόπτης
      }
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4) {}

  LL_PWR_EnableRange1BoostMode();
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1) {}

  LL_RCC_HSI_SetCalibTrimming(64);
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_4, 85, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_EnableDomain_SYS();
  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1) {}

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

  for (__IO uint32_t i = (170 >> 1); i != 0; i--);

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
  LL_I2C_InitTypeDef I2C_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

  /**I2C1 GPIO Configuration
  PA15   ------> I2C1_SCL
  PB9    ------> I2C1_SDA
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

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

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
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
}

/* USER CODE BEGIN 4 */

/* ========================================================================== */
/*                      PCA9555 I2C Low-Level Drivers                         */
/* ========================================================================== */

void PCA9555_Write(uint8_t reg, uint8_t value)
{
    while (LL_I2C_IsActiveFlag_BUSY(I2C_INSTANCE));

    LL_I2C_HandleTransfer(I2C_INSTANCE, PCA9555_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

    while (!LL_I2C_IsActiveFlag_TXIS(I2C_INSTANCE));
    LL_I2C_TransmitData8(I2C_INSTANCE, reg);

    while (!LL_I2C_IsActiveFlag_TXIS(I2C_INSTANCE));
    LL_I2C_TransmitData8(I2C_INSTANCE, value);

    while (!LL_I2C_IsActiveFlag_STOP(I2C_INSTANCE));
    LL_I2C_ClearFlag_STOP(I2C_INSTANCE);
}

uint8_t PCA9555_Read(uint8_t reg)
{
    uint8_t received_data = 0;

    LL_I2C_HandleTransfer(I2C_INSTANCE, PCA9555_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXIS(I2C_INSTANCE));
    LL_I2C_TransmitData8(I2C_INSTANCE, reg);
    while (!LL_I2C_IsActiveFlag_TC(I2C_INSTANCE));

    LL_I2C_HandleTransfer(I2C_INSTANCE, PCA9555_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    while (!LL_I2C_IsActiveFlag_RXNE(I2C_INSTANCE));
    received_data = LL_I2C_ReceiveData8(I2C_INSTANCE);

    while (!LL_I2C_IsActiveFlag_STOP(I2C_INSTANCE));
    LL_I2C_ClearFlag_STOP(I2C_INSTANCE);

    return received_data;
}

void PCA9555_Init(void) {
	// pex0 as output
	PCA9555_Write(REG_CONFIGURATION_0, 0x00);

	IO0 = 0x00;
	// initialize output to 0
	PCA9555_Write(REG_OUTPUT_0, IO0);
}

/* ========================================================================== */
/*                         LCD Driver Functions                               */
/* ========================================================================== */

void lcd_write_4bits(uint8_t value)
{
    IO0 &= 0x0F; // Preserve control signals on lower nibble
    if (value & 0x01) IO0 |= LCD_D4_BIT;
    if (value & 0x02) IO0 |= LCD_D5_BIT;
    if (value & 0x04) IO0 |= LCD_D6_BIT;
    if (value & 0x08) IO0 |= LCD_D7_BIT;
    PCA9555_Write(REG_OUTPUT_0, IO0);
}

void lcd_pulse_enable(void)
{
    IO0 |= LCD_E_BIT;
    PCA9555_Write(REG_OUTPUT_0, IO0);
    LL_mDelay(1);

    IO0 &= ~LCD_E_BIT;
    PCA9555_Write(REG_OUTPUT_0, IO0);
    LL_mDelay(1);
}

void lcd_send(uint8_t value, uint8_t mode)
{
    if (mode == 1) IO0 |= LCD_RS_BIT;  // Data mode
    else           IO0 &= ~LCD_RS_BIT; // Command mode

    PCA9555_Write(REG_OUTPUT_0, IO0);

    lcd_write_4bits((value >> 4) & 0x0F); // High nibble
    lcd_pulse_enable();

    lcd_write_4bits(value & 0x0F);        // Low nibble
    lcd_pulse_enable();
}

void lcd_command(uint8_t cmd)
{
    lcd_send(cmd, 0);
}

void lcd_data(uint8_t data)
{
    lcd_send(data, 1);
}

void lcd_clear(void)
{
    lcd_command(0x01);
    LL_mDelay(5);
}

void lcd_init(void)
{
    PCA9555_Init();
    LL_mDelay(50);

    IO0 &= ~(LCD_RS_BIT | LCD_E_BIT);
    PCA9555_Write(REG_OUTPUT_0, IO0);

    lcd_write_4bits(0x03);
    lcd_pulse_enable();
    LL_mDelay(5);

    lcd_write_4bits(0x03);
    lcd_pulse_enable();
    LL_mDelay(1);

    lcd_write_4bits(0x03);
    lcd_pulse_enable();
    LL_mDelay(1);

    lcd_write_4bits(0x02); // Set 4-bit mode
    lcd_pulse_enable();
    LL_mDelay(1);

    lcd_command(0x28); // 4-bit, 2 lines, 5x8 font
    lcd_command(0x0C); // Display ON, Cursor OFF
    lcd_command(0x06); // Increment cursor position
    lcd_clear();
}

/* ========================================================================== */
/*                         Keypad 4x4 Driver Functions                        */
/* ========================================================================== */

int scan_row(int row)
{
    uint8_t btn_pressed;

    // Only the selected row becomes output (active low)
    PCA9555_Write(REG_CONFIGURATION_1, (uint8_t)~(1U << row));
    PCA9555_Write(REG_OUTPUT_1, 0x00);

    // Read columns IO1[7:4]
    btn_pressed = (uint8_t)~(PCA9555_Read(REG_INPUT_1) >> 4);
    btn_pressed &= 0x0F;

    for (int col = 0; col < 4; col++)
    {
        if (btn_pressed & (1U << col))
        {
            return col;
        }
    }
    return -1;
}

uint16_t scan_keypad(void)
{
    int col;
    for (int row = 0; row < 4; row++)
    {
        col = scan_row(row);
        if (col != -1)
        {
            return ((uint16_t)col << 8) | (uint16_t)row;
        }
    }
    return NO_KEY_PRESSED;
}

void scan_keypad_rising_edge(void)
{
    uint16_t pressed_keys_tempo = scan_keypad();
    LL_mDelay(20);
    uint16_t current_keys = scan_keypad();

    if (current_keys == pressed_keys_tempo && pressed_keys_tempo != pressed_keys)
    {
        pressed_keys = pressed_keys_tempo;
    }
}

char keypad_to_ascii(uint16_t key_code)
{
    static const char key_map[4][4] = {
        {'*', '0', '#', 'D'},
        {'7', '8', '9', 'C'},
        {'4', '5', '6', 'B'},
        {'1', '2', '3', 'A'}
    };

    if (key_code == NO_KEY_PRESSED)
    {
        return '\0';
    }

    int col = (key_code >> 8) & 0xFF;
    int row = key_code & 0xFF;

    if ((row >= 0) && (row < 4) && (col >= 0) && (col < 4))
    {
        return key_map[row][col];
    }
    return '\0';
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
