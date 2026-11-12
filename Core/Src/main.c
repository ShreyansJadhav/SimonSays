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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "Adafruit_ST7735_Macros.h"
#include "Adafruit_ST77xx_Macros.h"
#include "Adafruit_TFTShield18_Macros.h"
#include "dwt_module.h"
#include "Adafruit_TFTShield18_API.h"
#include "Adafruit_ST7735_API.h"
#include "stm32h5xx_hal.h"

#include "buzzer.h"
#include "stm32h5xx_hal_tim.h"

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

COM_InitTypeDef BspCOMInit;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef handle_GPDMA1_Channel6;
DMA_HandleTypeDef handle_GPDMA1_Channel5;

RNG_HandleTypeDef hrng;

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef handle_GPDMA1_Channel7;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */

Buzzer buzzer1 = {
  &htim1,
  TIM_CHANNEL_1,
  false
};

extern Note const chromaticScale[NUM_NOTES];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_I2C1_Init(void);
static void MX_ICACHE_Init(void);
static void MX_SPI2_Init(void);
static void MX_RNG_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
  #include <stdbool.h>
  // Returns the index of the pressed button (0-3).
  // Returns -1 if no button is pressed.
  int readButton(GPIO_TypeDef *btnPort[], uint16_t btnPin[])
  {
      for (int i = 0; i < 4; i++)
      {
          if (HAL_GPIO_ReadPin(btnPort[i], btnPin[i]) == GPIO_PIN_RESET)
          {
              HAL_Delay(20); // Debounce

              if (HAL_GPIO_ReadPin(btnPort[i], btnPin[i]) == GPIO_PIN_RESET)
              {
                  return i;
              }
          }
      }

      return -1;
  }

    // Returns true if the joystick SELECT button is pressed
  bool selectPressed(void)
  {
      return !(TFTShield18_readButtons() & TFTSHIELD_BUTTON_IN);
  }

  // Delays while checking SELECT every 10 milliseconds
  // Returns true if SELECT was pressed during the delay
  bool delayWithSelect(uint32_t ms)
  {
      uint32_t start = HAL_GetTick();

      while ((uint32_t)(HAL_GetTick() - start) < ms)
      {
          if (selectPressed())
          {
              return true;
          }

          HAL_Delay(10);
      }

      return selectPressed();
  }
    // RGB565 colors for the TFT display
  #define DARK_RED     0x8000
  #define BRIGHT_RED   0xF800

  #define DARK_GREEN   0x0400
  #define BRIGHT_GREEN 0x07E0

  #define DARK_BLUE    0x0010
  #define BRIGHT_BLUE  0x001F

  #define DARK_YELLOW  0x8400
  #define BRIGHT_YELLOW 0xFFE0

  // Draws one LED circle on the TFT
  void drawSimonLED(int ledIndex, bool lit)
  {
      uint16_t darkColors[4] = {
          DARK_RED, DARK_GREEN, DARK_BLUE, DARK_YELLOW
      };

      uint16_t brightColors[4] = {
          BRIGHT_RED, BRIGHT_GREEN, BRIGHT_BLUE, BRIGHT_YELLOW
      };

      if (ledIndex < 0 || ledIndex >= 4)
      {
          return;
      }

      // Four evenly spaced circles across the 160px screen
      int x = 20 + ledIndex * 40;
      int y = 75;
      int radius = 14;

      uint16_t color = lit
          ? brightColors[ledIndex]
          : darkColors[ledIndex];

      Display_fillCircle(x, y, radius, color);
  }

  // Draws the initial Simon Says board
  void drawSimonBoard(void)
  {
      Display_fillScreen(ST77XX_BLACK);

      Display_setTextColor(ST7735_WHITE);
      Display_setTextSize(1, 2);
      Display_setCursor(30, 15);


      // All four circles start unlit
      for (int i = 0; i < 4; i++)
      {
          drawSimonLED(i, false);
      }
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_I2C1_Init();
  MX_ICACHE_Init();
  MX_SPI2_Init();
  MX_RNG_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Enable the DWT delay timer module
  DWT_Init();
  if(!DWT_CheckInitialized()) printf("DWT failed to initialize!\n\r");

  // Initialize the TFT shield object in code and on chip, with failure handling
  TFTShield18_create(&hi2c1);
  if (!TFTShield18_begin())
  {
    printf("Failed to initialize TFT shield!\n\r");
    return 1;
  } else {
    printf("TFT shield initialized successfully!\n\r");
  }

  // Software reset the entire TFT shield to restore it to a known empty startup state
  TFTShield18_tftReset(true);
  DWT_delayMs(100);

  // Initialize the display object and the display itself to support graphics operations. No error handling is
  // provided since there is no way to read the status from the display. Proper operation of the display and response
  // to graphics commands provide enough confidence that the display is working correctly.
  Display_create(&hspi2, DISP_CS_Pin, DISP_CS_GPIO_Port, DISP_DCX_SEL_Pin, DISP_DCX_SEL_GPIO_Port);
  Display_init(INITR_BLACKTAB);
  DWT_delayMs(100);

  // Set the display to be in landscape mode and clear the screen
  Display_setRotation(1);
  Display_fillScreen(ST77XX_MAGENTA);

  // Turn on the TFT shield backlight to its maximum value
  TFTShield18_setBacklight(TFTSHIELD_BACKLIGHT_ON);

  /* Place extra user setup code here */

  Display_setTextSize(1, 2);
  Display_setTextColor(ST7735_BLACK);
  Display_setCursor(Display_width() / 2 - 58, Display_height() / 2 - 5);
  Display_print("Welcome to Simon Says!");


    /* ---- PHASE 1 TEST: pattern building ---- */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_TypeDef *ledPort[4] = { GPIOC,       GPIOC,       GPIOC,       GPIOB };
  uint16_t      ledPin[4]  = { GPIO_PIN_2,  GPIO_PIN_3,  GPIO_PIN_5,  GPIO_PIN_12 };
  GPIO_TypeDef *btnPort[4] = { GPIOB,       GPIOC,       GPIOB,       GPIOC };
  uint16_t      btnPin[4]  = { GPIO_PIN_13, GPIO_PIN_12, GPIO_PIN_8,  GPIO_PIN_11 };

  GPIO_InitTypeDef gpio = {0};
  gpio.Speed = GPIO_SPEED_FREQ_LOW;

  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  for (int i = 0; i < 4; i++)
  {
    gpio.Pin = ledPin[i];
    HAL_GPIO_Init(ledPort[i], &gpio);
    HAL_GPIO_WritePin(ledPort[i], ledPin[i], GPIO_PIN_RESET);
  }

  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  for (int i = 0; i < 4; i++)
  {
    gpio.Pin = btnPin[i];
    HAL_GPIO_Init(btnPort[i], &gpio);
  }


  #define MAX_SEQ 32

  uint8_t sequence[MAX_SEQ];
  int seqLen = 0;

  // Each LED gets a unique note
  int ledNotes[4] = {0, 2, 4, 8};


  buzzerDisable(&buzzer1);

  while (1)
  {
      // WELCOME SCREEN
      seqLen = 0;

      Display_fillScreen(ST77XX_MAGENTA);
      Display_setTextColor(ST7735_BLACK);
      Display_setTextSize(1, 2);
      Display_setCursor(15, 40);
      Display_print("Welcome to Simon Says!");
      Display_setCursor(10, 70);
      Display_print("Press Select");

      // Wait for joystick SELECT press
      while (!selectPressed())
      {
          HAL_Delay(10);
      }

      // Wait for SELECT release before starting
      while (selectPressed())
      {
          HAL_Delay(10);
      }

      bool restartRequested = false;
      bool gameOver = false;

      // COUNTDOWN
      for (int count = 3; count > 0; count--)
      {
          Display_fillScreen(ST77XX_BLACK);
          Display_setTextColor(ST7735_WHITE);
          Display_setCursor(65, 55);
          Display_setTextSize(2, 3);

          char countText[2];
          countText[0] = '0' + count;
          countText[1] = '\0';

          Display_print(countText);

          // SELECT cancels the countdown
          if (delayWithSelect(1000))
          {
              restartRequested = true;
              break;
          }
      }
      drawSimonBoard();

      if (restartRequested)
      {
          while (selectPressed())
          {
              HAL_Delay(10);
          }

          continue; // Return to welcome
      }


      // MAIN GAME LOOP

      while (!gameOver && !restartRequested)
      {
          // 1. Add one random LED to sequence
          if (seqLen < MAX_SEQ)
          {
              uint32_t r;

              if (HAL_RNG_GenerateRandomNumber(&hrng, &r) != HAL_OK)
              {
                  Error_Handler();
              }

              sequence[seqLen++] = r % 4;
          }

          // 2. Show current level
          drawSimonBoard();

          if (delayWithSelect(700))
          {
              restartRequested = true;
              break;
          }

          // PLAY LED SEQUENCE
          for (int i = 0; i < seqLen; i++)
          {
              uint8_t led = sequence[i];

              buzzerSetNote(&buzzer1,
                            chromaticScale[ledNotes[led]], 1);

              HAL_GPIO_WritePin(ledPort[led],
                                ledPin[led], GPIO_PIN_SET);

              drawSimonLED(led, true);
              Display_setTextColor(ST7735_WHITE);
              Display_setTextSize(1, 2);
              Display_setCursor(30, 15);
              Display_print("Watch sequence");

              buzzerEnable(&buzzer1);

              // LED ON for 400 ms
              bool cancelled = delayWithSelect(400);

              // Always turn LED and buzzer off
              HAL_GPIO_WritePin(ledPort[led],
                                ledPin[led], GPIO_PIN_RESET);

              drawSimonLED(led, false);

              buzzerDisable(&buzzer1);

              if (cancelled)
              {
                  restartRequested = true;
                  break;
              }

              // Pause between notes
              if (delayWithSelect(200))
              {
                  restartRequested = true;
                  break;
              }
          }

          if (restartRequested)
          {
              break;
          }

          // PLAYER INPUT
          drawSimonBoard();

          Display_setTextColor(ST7735_WHITE);
          Display_setTextSize(1, 2);
          Display_setCursor(10, 20);
          Display_print("Your turn!");

          int inputIndex = 0;

          while (inputIndex < seqLen && !gameOver)
          {
              // SELECT returns to welcome
              if (selectPressed())
              {
                  restartRequested = true;
                  break;
              }

              int pressed = readButton(btnPort, btnPin);

              // No button pressed yet
              if (pressed == -1)
              {
                  HAL_Delay(10);
                  continue;
              }

              // Light LED and play corresponding note
              HAL_GPIO_WritePin(ledPort[pressed],
                                ledPin[pressed], GPIO_PIN_SET);

              drawSimonLED(pressed, true);

              buzzerSetNote(&buzzer1,
                            chromaticScale[ledNotes[pressed]], 1);

              buzzerEnable(&buzzer1);

              // Wait for physical button release
              while (HAL_GPIO_ReadPin(btnPort[pressed],
                                      btnPin[pressed]) == GPIO_PIN_RESET)
              {
                  if (selectPressed())
                  {
                      restartRequested = true;
                      break;
                  }

                  HAL_Delay(10);
              }

              // Always turn off LED and buzzer
              HAL_GPIO_WritePin(ledPort[pressed],
                                ledPin[pressed], GPIO_PIN_RESET);

              drawSimonLED(pressed, false);

              buzzerDisable(&buzzer1);

              if (restartRequested)
              {
                  break;
              }

              // Debounce release, still checking SELECT
              if (delayWithSelect(20))
              {
                  restartRequested = true;
                  break;
              }

              // Check player's input
              if (pressed == sequence[inputIndex])
              {
                  inputIndex++;
              }
              else
              {
                  gameOver = true;
              }
          }

          if (restartRequested)
          {
              break;
          }

          // CORRECT SEQUENCE
          if (!gameOver)
          {
              Display_fillScreen(ST77XX_GREEN);
              Display_setTextColor(ST7735_BLACK);
              Display_setTextSize(1, 2);
              Display_setCursor(15, 55);
              Display_print("Simon says that's correct!");

              if (delayWithSelect(1000))
              {
                  restartRequested = true;
                  break;
              }

              // Max sequence completed
              if (seqLen == MAX_SEQ)
              {
                  gameOver = true;
              }
          }
      }

      // SELECT RESTART HANDLING
      if (restartRequested)
      {
          buzzerDisable(&buzzer1);

          // Turn off all LEDs
          for (int i = 0; i < 4; i++)
          {
              HAL_GPIO_WritePin(ledPort[i],
                                ledPin[i], GPIO_PIN_RESET);
          }

          // Wait for SELECT release
          while (selectPressed())
          {
              HAL_Delay(10);
          }

          continue; // Return to welcome
      }

      // GAME OVER
      buzzerDisable(&buzzer1);

      drawSimonBoard();
      Display_setTextColor(ST7735_WHITE);
      Display_setTextSize(1, 2);
      Display_setCursor(25, 105);
      Display_print("GAME OVER!");

      // Rapid random LED flashes + notes
      for (int i = 0; i < 12; i++)
      {
          if (selectPressed())
          {
              restartRequested = true;
              break;
          }

          uint32_t r;

          if (HAL_RNG_GenerateRandomNumber(&hrng, &r) != HAL_OK)
          {
              Error_Handler();
          }

          uint8_t led = r % 4;

          buzzerSetNote(&buzzer1,
                        chromaticScale[ledNotes[led]], 1);

          HAL_GPIO_WritePin(ledPort[led],
                            ledPin[led], GPIO_PIN_SET);

          drawSimonLED(led, true);

          buzzerEnable(&buzzer1);

          // Flash for 100 ms, checking SELECT
          bool cancelled = delayWithSelect(100);

          HAL_GPIO_WritePin(ledPort[led],
                            ledPin[led], GPIO_PIN_RESET);

          drawSimonLED(led, false);

          buzzerDisable(&buzzer1);

          if (cancelled)
          {
              restartRequested = true;
              break;
          }

          if (delayWithSelect(50))
          {
              restartRequested = true;
              break;
          }
      }

      // Wait on Game Over screen, unless SELECT was pressed
      if (!restartRequested)
      {
          delayWithSelect(1500);
      }

      // Ensure SELECT is released before returning
      while (selectPressed())
      {
          HAL_Delay(10);
      }

      // Outer loop returns to welcome screen
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_CSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV2;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.CSIState = RCC_CSI_ON;
  RCC_OscInitStruct.CSICalibrationValue = RCC_CSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_CSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 45;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_0);
}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */
  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel5_IRQn);
    HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
    HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

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

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00707CBB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache (default 2-ways set associative cache)
  */
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_1LINE;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x7;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1400;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 600;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DISP_DCX_SEL_GPIO_Port, DISP_DCX_SEL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : DISP_DCX_SEL_Pin */
  GPIO_InitStruct.Pin = DISP_DCX_SEL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(DISP_DCX_SEL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DISP_CS_Pin */
  GPIO_InitStruct.Pin = DISP_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(DISP_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};
  MPU_Attributes_InitTypeDef MPU_AttributesInit = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region 0 and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x08FFF000;
  MPU_InitStruct.LimitAddress = 0x08FFFFFF;
  MPU_InitStruct.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;
  MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Attribute 0 and the memory to be protected
  */
  MPU_AttributesInit.Number = MPU_ATTRIBUTES_NUMBER0;
  MPU_AttributesInit.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);

  HAL_MPU_ConfigMemoryAttributes(&MPU_AttributesInit);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
