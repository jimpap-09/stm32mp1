/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Solution of Exercise 5.1 using STM32G474 and PCA9555
  ******************************************************************************
 *
 * σε αυτό το παράδειγμα,
 * πραγματοποιούνται 2 λογικές συναρτήσεις
 * F0 = 
**/
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

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN 0 */

/*
 * Η διεύθυνση 7-bit του PCA9555 είναι 0x20.
 * Στις συναρτήσεις LL δίνεται μετατοπισμένη κατά μία θέση:
 * 0x20 << 1 = 0x40.
 */
#define PCA9555_ADDRESS    0x40

/* Μάσκες για τις εξόδους IO0_0 και IO0_1 */
#define PCA_F0_PIN         (1U << 0)
#define PCA_F1_PIN         (1U << 1)

/* Καταχωρητές του PCA9555 */
typedef enum
{
    REG_INPUT_0           = 0,
    REG_INPUT_1           = 1,
    REG_OUTPUT_0          = 2,
    REG_OUTPUT_1          = 3,
    REG_POLARITY_INV_0    = 4,
    REG_POLARITY_INV_1    = 5,
    REG_CONFIGURATION_0   = 6,
    REG_CONFIGURATION_1   = 7

} PCA9555_REGISTERS;


/**
  * @brief  Εγγραφή μίας τιμής σε καταχωρητή του PCA9555.
  * @param  reg: Καταχωρητής που θα προσπελαστεί.
  * @param  value: Τιμή που θα γραφτεί.
  */
static void PCA9555_Write(uint8_t reg, uint8_t value)
{
    /*
     * Έναρξη μετάδοσης:
     * - περιφερειακό I2C1
     * - διεύθυνση PCA9555
     * - διευθυνσιοδότηση 7-bit
     * - αποστολή δύο bytes
     * - αυτόματο STOP
     * - λειτουργία εγγραφής
     */
    LL_I2C_HandleTransfer(
        I2C1,
        PCA9555_ADDRESS,
        LL_I2C_ADDRSLAVE_7BIT,
        2,
        LL_I2C_MODE_AUTOEND,
        LL_I2C_GENERATE_START_WRITE
    );

    /* Αναμονή μέχρι να μπορεί να σταλεί το Command Byte */
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
    {
    }

    /* Αποστολή της διεύθυνσης του καταχωρητή */
    LL_I2C_TransmitData8(I2C1, reg);

    /* Αναμονή μέχρι να μπορεί να σταλεί η τιμή */
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
    {
    }

    /* Αποστολή της τιμής προς τον επιλεγμένο καταχωρητή */
    LL_I2C_TransmitData8(I2C1, value);

    /* Αναμονή για την αυτόματη δημιουργία της συνθήκης STOP */
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
    {
    }

    /* Καθαρισμός της σημαίας STOP */
    LL_I2C_ClearFlag_STOP(I2C1);
}


/**
  * @brief  Διαβάζει τις εισόδους PB0-PB3 και δημιουργεί το byte εξόδου.
  * @retval Byte με F0 στο bit 0 και F1 στο bit 1.
  */
static uint8_t Calculate_Logical_Functions(void)
{
    uint8_t A;
    uint8_t B;
    uint8_t C;
    uint8_t D;

    uint8_t F0;
    uint8_t F1;

    uint8_t pca_output;

    /* Ανάγνωση των τεσσάρων εισόδων του STM32 */
    A = LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_0) ? 1U : 0U;
    B = LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_1) ? 1U : 0U;
    C = LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_2) ? 1U : 0U;
    D = LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_3) ? 1U : 0U;

    /*
     * F0 = (A·B' + C·B·D)'
     *
     * Το λογικό NOT εφαρμόζεται μόνο στο ένα bit
     * με XOR ως προς 1, ώστε το αποτέλεσμα να είναι 0 ή 1.
     */
    F0 = ((A & (B ^ 1U)) | (C & B & D)) ^ 1U;

    /*
     * F1 = (A + C)·(B·D)
     */
    F1 = (A | C) & (B & D);

    /*
     * IO0_0 <- F0
     * IO0_1 <- F1
     */
    pca_output = 0;

    if (F0 != 0U)
    {
        pca_output |= PCA_F0_PIN;
    }

    if (F1 != 0U)
    {
        pca_output |= PCA_F1_PIN;
    }

    return pca_output;
}

/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    uint8_t output_value;

    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();

    /*
     * Configuration Port 0:
     *
     * bit = 0 -> έξοδος
     * bit = 1 -> είσοδος
     *
     * 0xFC = 11111100
     *
     * IO0_0 και IO0_1: έξοδοι
     * IO0_2 έως IO0_7: είσοδοι
     */
    PCA9555_Write(REG_CONFIGURATION_0, 0xFC);

    /* Αρχικά μηδενίζονται οι δύο έξοδοι */
    PCA9555_Write(REG_OUTPUT_0, 0x00);

    while (1)
    {
        /*
         * Ανάγνωση PB0-PB3 και υπολογισμός
         * των λογικών συναρτήσεων F0 και F1.
         */
        output_value = Calculate_Logical_Functions();

        /*
         * Μετάδοση των αποτελεσμάτων στον PCA9555:
         *
         * bit 0 -> IO0_0 -> F0
         * bit 1 -> IO0_1 -> F1
         */
        PCA9555_Write(REG_OUTPUT_0, output_value);

        /*
         * Μικρή καθυστέρηση για περιορισμό των συνεχόμενων
         * μεταδόσεων και πιθανών σπινθηρισμών των εισόδων.
         */
        LL_mDelay(10);
    }
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);

    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4)
    {
    }

    LL_PWR_EnableRange1BoostMode();

    LL_RCC_HSI_Enable();

    while (LL_RCC_HSI_IsReady() != 1)
    {
    }

    LL_RCC_HSI_SetCalibTrimming(64);

    LL_RCC_PLL_ConfigDomain_SYS(
        LL_RCC_PLLSOURCE_HSI,
        LL_RCC_PLLM_DIV_4,
        85,
        LL_RCC_PLLR_DIV_2
    );

    LL_RCC_PLL_EnableDomain_SYS();
    LL_RCC_PLL_Enable();

    while (LL_RCC_PLL_IsReady() != 1)
    {
    }

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);

    while (LL_RCC_GetSysClkSource() !=
           LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    {
    }

    for (__IO uint32_t i = (170 >> 1); i != 0; i--)
    {
    }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    LL_SetSystemCoreClock(170000000);

    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief I2C1 Initialization Function
  * @retval None
  */
static void MX_I2C1_Init(void)
{
    LL_I2C_InitTypeDef I2C_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Πηγή ρολογιού του I2C1 */
    LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);

    /* Ενεργοποίηση ρολογιών GPIO */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    /*
     * PA15 -> I2C1_SCL
     */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;

    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*
     * PB9 -> I2C1_SDA
     */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;

    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Ενεργοποίηση ρολογιού I2C1 */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    /* Αρχικοποίηση του περιφερειακού I2C1 */
    I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
    I2C_InitStruct.Timing = 0x40B285C2;
    I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
    I2C_InitStruct.DigitalFilter = 0;
    I2C_InitStruct.OwnAddress1 = 0;
    I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
    I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;

    LL_I2C_Init(I2C1, &I2C_InitStruct);

    LL_I2C_EnableAutoEndMode(I2C1);

    LL_I2C_SetOwnAddress2(
        I2C1,
        0,
        LL_I2C_OWNADDRESS2_NOMASK
    );

    LL_I2C_DisableOwnAddress2(I2C1);
    LL_I2C_DisableGeneralCall(I2C1);
    LL_I2C_EnableClockStretching(I2C1);
}


/**
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Ενεργοποίηση του ρολογιού της GPIOB */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    /*
     * PB0 -> A
     * PB1 -> B
     * PB2 -> C
     * PB3 -> D
     */
    GPIO_InitStruct.Pin =
        LL_GPIO_PIN_0 |
        LL_GPIO_PIN_1 |
        LL_GPIO_PIN_2 |
        LL_GPIO_PIN_3;

    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;

    /*
     * Χρησιμοποιείται pull-up, θεωρώντας ότι οι είσοδοι
     * συνδέονται μέσω διακοπτών προς τη γείωση.
     */
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;

    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
