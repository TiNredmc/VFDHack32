/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

// Pin connection
// PB15 	MOSI
// PB13 	SCLK
// PE9 		LATCH
// PE8 		BLANK

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint32_t GPbuff[9] ; // 32x9 bit array (288 bits == What display expected).
static uint8_t Sendbuffptr[36];
//Dummy bytes ; Format (MSB)[a,b,c,d,e,f,0,0](LSB)

uint8_t img1[39]={
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54      };

static const uint8_t reOrder0[2][6]={// first 4 bytes (GPbuff[0and3])
{0,2,0,3,0,4}, /* the bit shift to convert def to the afbecd format */
{7,0,6,0,5,0} /* the bit shift to convert abc to the afbecd format */
};

static const uint8_t reOrder1[2][6]={// next 4 bytes (GPbuff[1and4])
{0,3,0,4,0,2}, /* the bit shift to convert efd to the afbecd format */
{6,0,5,0,7,0} /* the bit shift to convert bca to the afbecd format */
};

static const uint8_t reOrder2[2][6]={// last 4 bytes (GPbuff[2and5])
{0,4,0,2,0,3}, /* the bit shift to convert fde to the afbecd format */
{5,0,7,0,6,0} /* the bit shift to convert cab to the afbecd format */
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Delay_us(int usecs){
	int count = (usecs * 48) / 4;
	for (int i = 0; i < count; ++i) {
	    count--;
	}
}

void VFDLoadBMP(uint8_t Grid, uint8_t *sBMP){
	// Logically thinking : Determine the Grid is Event or Odd number (Important, For the simple algorithm to convert abcdef to afbecd format).
		uint8_t EvOd = 0;
		if(Grid%2){
			EvOd = 1;// odd number (odd grid), Only manipulate the a, b, c Dots
		}else{
			EvOd = 0;// event number (event grid), Only manipulate the d, e, f Dots
		}

uint8_t nxtWrd = 0;
uint8_t CMP= 0;
for(uint8_t wdCount=0; wdCount < 2;wdCount++){// First 6 array repeat itself so do this twice.

		// GPbuff[0] and GPbuf[3]
		// 4 bytes on first buffer array
		// 1a 1f 1b 1e 1c 1d to 6a 6f

		for(uint8_t i=0;i < 32;i++){// for 2nd round
			memcpy(&CMP, sBMP,1);
			if(CMP & (1 << reOrder0[EvOd][i%6])){
				GPbuff[0+nxtWrd] |= (1 << i);
			}else{
				GPbuff[0+nxtWrd] &= ~(1 << i);
			}

			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

		// GPbuff[1] and GPbuff[4]
		//
		//4 bytes on second buffer array + 4 bits for 11a, 11f, 11b and 11e
		for(uint8_t i=0;i < 32;i++){
			memcpy(&CMP, sBMP,1);
			if(CMP & (1 << reOrder1[EvOd][(i%6)])){
				GPbuff[1+nxtWrd] |= (1 << i);
			}else{
				GPbuff[1+nxtWrd] &= ~(1 << i);
			}

			if((i%6) == 3){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

		// GPbuff[2] and GPbuff[5]
		//
		//4 bytes on 3rd buffer array
		for(uint8_t i=0;i < 32;i++){
			memcpy(&CMP, sBMP,1);
			if(CMP & (1 << reOrder2[EvOd][i%6])){
				GPbuff[2+nxtWrd] |= (1 << i);
			}else{
				GPbuff[2+nxtWrd] &= ~(1 << i);
			}

			if((i%6) == 1){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

		nxtWrd = 3;// next time we start the array at GPbuff[3]
}//for(uint8_t wdCount=0; wdCount < 2;wdCount++)

		// GPbuff[6]
		// 32a to 38f

		for(uint8_t i=0;i < 32;i++){// for 2nd round
			memcpy(&CMP, sBMP,1);
			if(CMP & (1 << reOrder0[EvOd][i%6])){
				GPbuff[6] |= (1 << i);
			}else{
				GPbuff[6] &= ~(1 << i);
			}

			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

		for(uint8_t i=0;i < 10;i++){
			memcpy(&CMP, sBMP,1);
			if(CMP & (1 << reOrder1[EvOd][i%6])){
				GPbuff[7] |= (1 << i);
			}else{
				GPbuff[7] &= ~(1 << i);
			}

			if((i%6) == 3){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

			if(Grid < 22){// Grid number is between 1 - 21
				GPbuff[7] |= (1 << (Grid+9));// Grid N on
				GPbuff[7] |= (1 << (Grid+10));// Grid  N+1 on
			}else if(Grid == 22){// Grid number is 22
				GPbuff[7] |= (1 << 31);// Grid 22 on
				GPbuff[8] |= (1 << 0);// Grid 23 on
			}else{// Grid number is between 23-54
				GPbuff[8] |= (1 << (Grid-23));
				GPbuff[8] |= (1 << (Grid-22));
			}

			memcpy(Sendbuffptr, GPbuff, 36);
			GPbuff[7] = 0;
			GPbuff[8] = 0;
	}


void VFDUpdate(){
	HAL_GPIO_WritePin(GPIOE, LD4_Pin, GPIO_PIN_SET);// BLK high
	HAL_GPIO_WritePin(GPIOE, LD3_Pin, GPIO_PIN_SET);// LAT high
	Delay_us(50);
	HAL_GPIO_WritePin(GPIOE, LD3_Pin, GPIO_PIN_RESET);// LAT low
	HAL_GPIO_WritePin(GPIOE, LD4_Pin, GPIO_PIN_RESET);// BLK low

	HAL_SPI_Transmit(&hspi2, Sendbuffptr, 36, HAL_MAX_DELAY);// Send data over SPI
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
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  uint8_t GridNum = 1;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	VFDLoadBMP(GridNum++, img1);
	VFDUpdate();
	//HAL_Delay(100);
	if(GridNum > 52)
		GridNum = 1;
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
