/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Keypad 4x4 through PCA9555 - Exercise 6.1
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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
    REG_INPUT_0 = 0,
    REG_INPUT_1 = 1,
    REG_OUTPUT_0 = 2,
    REG_OUTPUT_1 = 3,
    REG_POLARITY_INV_0 = 4,
    REG_POLARITY_INV_1 = 5,
    REG_CONFIGURATION_0 = 6,
    REG_CONFIGURATION_1 = 7

} PCA9555_REGISTERS;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PCA9555_ADDRESS   0x40
#define NO_KEY_PRESSED    0xFFFF

#define LED_PINS (LL_GPIO_PIN_1 | \
                  LL_GPIO_PIN_2 | \
                  LL_GPIO_PIN_3 | \
                  LL_GPIO_PIN_4)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint16_t pressed_keys = NO_KEY_PRESSED;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

void PCA9555_Write(uint8_t reg, uint8_t value);
uint8_t PCA9555_Read(uint8_t reg);

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

    /*
     * Αρχικά όλα τα LEDs OFF.
     */
    LL_GPIO_ResetOutputPin(GPIOB, LED_PINS);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /*
         * 1. Διαβάζουμε απευθείας το πληκτρολόγιο για να ξέρουμε
         *    ποιο πλήκτρο είναι πατημένο ΑΥΤΗ ΤΗ ΣΤΙΓΜΗ.
         */

    	scan_keypad_rising_edge();
    	char key = keypad_to_ascii(pressed_keys);

        /*
         * 2. Σβήνουμε πρώτα όλα τα LEDs (PB1 - PB4)
         */
        LL_GPIO_ResetOutputPin(GPIOB, LED_PINS);

        /*
         * 3. Ανάβουμε το αντίστοιχο LED ανάλογα με το πλήκτρο
         */
        if (key == '4')
        {
            LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1); // PB1 LED ON[cite: 5]
        }
        else if (key == '2')
        {
            LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_2); // PB2 LED ON[cite: 5]
        }
        else if (key == '3')
        {
            LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_3); // PB3 LED ON[cite: 5]
        }
        else if (key == 'B')
        {
            LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_4); // PB4 LED ON[cite: 5]
        }

        /* Μικρή καθυστέρηση για αποφυγή υπερβολικού φόρτου στο I2C */
        LL_mDelay(10);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */

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
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

  /**/
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);

  /**/
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_2);

  /**/
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_3);

  /**/
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_4);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/*
 * Write one byte to PCA9555 register
 */
void PCA9555_Write(uint8_t reg, uint8_t value)
{
    LL_I2C_HandleTransfer(
        I2C1,
        PCA9555_ADDRESS,
        LL_I2C_ADDRSLAVE_7BIT,
        2,
        LL_I2C_MODE_AUTOEND,
        LL_I2C_GENERATE_START_WRITE
    );

    while (!LL_I2C_IsActiveFlag_TXIS(I2C1));

    LL_I2C_TransmitData8(I2C1, reg);

    while (!LL_I2C_IsActiveFlag_TXIS(I2C1));

    LL_I2C_TransmitData8(I2C1, value);

    while (!LL_I2C_IsActiveFlag_STOP(I2C1));

    LL_I2C_ClearFlag_STOP(I2C1);
}


/*
 * Read one byte from PCA9555 register
 */
uint8_t PCA9555_Read(uint8_t reg)
{
    uint8_t received_data = 0;

    /*
     * First phase:
     * Select register
     */
    LL_I2C_HandleTransfer(
        I2C1,
        PCA9555_ADDRESS,
        LL_I2C_ADDRSLAVE_7BIT,
        1,
        LL_I2C_MODE_SOFTEND,
        LL_I2C_GENERATE_START_WRITE
    );

    while (!LL_I2C_IsActiveFlag_TXIS(I2C1));

    LL_I2C_TransmitData8(I2C1, reg);

    while (!LL_I2C_IsActiveFlag_TC(I2C1));


    /*
     * Second phase:
     * Repeated START and read one byte
     */
    LL_I2C_HandleTransfer(
        I2C1,
        PCA9555_ADDRESS,
        LL_I2C_ADDRSLAVE_7BIT,
        1,
        LL_I2C_MODE_AUTOEND,
        LL_I2C_GENERATE_START_READ
    );

    while (!LL_I2C_IsActiveFlag_RXNE(I2C1));

    received_data = LL_I2C_ReceiveData8(I2C1);

    while (!LL_I2C_IsActiveFlag_STOP(I2C1));

    LL_I2C_ClearFlag_STOP(I2C1);

    return received_data;
}


/*
 * Scan one keypad row.
 *
 * PCA9555 IO1:
 *
 * IO1[3:0] -> rows
 * IO1[7:4] -> columns
 */
int scan_row(int row)
{
    uint8_t btn_pressed;

    /*
     * Configuration register:
     *
     * 1 = input
     * 0 = output
     *
     * Only the selected row becomes output.
     */
    PCA9555_Write(
        REG_CONFIGURATION_1,
        (uint8_t)~(1U << row)
    );

    /*
     * Selected row -> LOW
     */
    PCA9555_Write(
        REG_OUTPUT_1,
        0x00
    );

    /*
     * Read IO1[7:4].
     *
     * Columns are active-low.
     */
    btn_pressed =
        (uint8_t)~(PCA9555_Read(REG_INPUT_1) >> 4);

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


/*
 * Scan all four keypad rows.
 */
uint16_t scan_keypad(void)
{
    int row;
    int col;

    for (row = 0; row < 4; row++)
    {
        col = scan_row(row);

        if (col != -1)
        {
            /*
             * High byte -> column
             * Low byte  -> row
             */
            return ((uint16_t)col << 8) |
                   (uint16_t)row;
        }
    }

    return NO_KEY_PRESSED;
}


/*
 * Keypad debounce.
 */
void scan_keypad_rising_edge(void)
{
    uint16_t pressed_keys_tempo;
    uint16_t current_keys;

    /*
     * First measurement.
     */
    pressed_keys_tempo = scan_keypad();

    /*
     * Debounce delay.
     */
    LL_mDelay(20);

    /*
     * Second measurement.
     */
    current_keys = scan_keypad();

    /*
     * Accept the new state only if
     * both measurements are equal.
     */
    if (current_keys == pressed_keys_tempo
    	&& pressed_keys_tempo != pressed_keys)
    {
        pressed_keys = pressed_keys_tempo;
    }
}


/*
 * Convert row/column to ASCII.
 */
char keypad_to_ascii(uint16_t key_code)
{
    static const char key_map[4][4] =
    {
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

    if ((row >= 0) && (row < 4) &&
        (col >= 0) && (col < 4))
    {
        return key_map[row][col];
    }

    return '\0';
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
