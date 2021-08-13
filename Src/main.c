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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
static uint32_t GPbuff[36] ; // define the 32-bit unsigned integer for holding 288 it data
uint8_t * Sendbuffptr = (uint8_t *)GPbuff;
//Dummy bytes ; Format (MSB)[a,b,c,d,e,f,0,0](LSB)

uint8_t img1[39]={
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78, 0xD8,
0x78, 0xD8, 0x78      };

static const uint8_t reOrder[2][6]={
{0,2,0,3,0,4}, /* the bit shift to convert def to the afbecd format */
{7,0,6,0,5,0} /* the bit shift to convert abc to the afbecd format */
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void VFDLoadBMP(uint8_t Grid, uint8_t *sBMP){
	// Logically thinking : Determine the Grid is Event or Odd number (Important, For the simple algorithm to convert abcdef to afbecd format).
		uint8_t EvOd = 0;
		if(Grid%2){
			EvOd = 1;// odd number (odd grid), Only manipulate the a, b, c Dots
		}else{
			EvOd = 0;// event number (event grid), Only manipulate the d, e, f Dots
		}
uint8_t nxtWrd = 0;
for(uint8_t wdCount=0; wdCount < 2;wdCount++){// Do twice
		//4 bytes on first buffer array
	if(wdCount){
		for(uint8_t i=0;i < 32;i++){// for 2nd round
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[0+nxtWrd] = (1 << i);
			}else{
				GPbuff[0+nxtWrd] = (0 << i);
			}
			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}
	}else{
		for(uint8_t i=0;i < 30;i++){// for 1st round
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[0+nxtWrd] = (1 << (i+2));
			}else{
				GPbuff[0+nxtWrd] = (0 << (i+2));
			}
			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
	}
	}
		//4 bytes on second buffer array + 2 bit for 11a,11f
		for(uint8_t i=0;i < 30;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[1+nxtWrd] = (1 << i);
			}else{
				GPbuff[1+nxtWrd] = (0 << i);
			}
			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}
		//2 bit for 11a,11f
		for(uint8_t i=30;i < 32;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[1+nxtWrd] = (1 << i);
				}else{
				GPbuff[1+nxtWrd] = (0 << i);
			}
		}

		//4byte on third buffer array
		for(uint8_t i=0;i < 4;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[2+nxtWrd] = (1 << i);
			}else{
				GPbuff[2+nxtWrd] = (0 << i);
			}
		}
		sBMP++;

		for(uint8_t i=0;i < 23;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[2+nxtWrd] = (1 << (i+4));
			}else{
				GPbuff[2+nxtWrd] = (0 << (i+4));
			}
			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

		//for 16a to 16e
		for(uint8_t i=27;i < 31;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][(i-3)%6])){
				GPbuff[2+nxtWrd] = (1 << i);
			}else{
				GPbuff[2+nxtWrd] = (0 << i);
			}
		}

		//16d and 16c in the 4th array(word)
		for(uint8_t i=0;i < 2;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][(i+4)%6])){
				GPbuff[2+nxtWrd] = (1 << i);
			}else{
				GPbuff[2+nxtWrd] = (0 << i);
			}
		}
		sBMP++;
		nxtWrd = 3;// next time we start the array at GPbuff[3]
}//for(uint8_t wdCount=0; wdCount < 2;wdCount++)

		//32d and 32c in the 7th buffer array(word)
		for(uint8_t i=0;i < 2;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][(i+4)%6])){
				GPbuff[6] = (1 << i);
			}else{
				GPbuff[6] = (0 << i);
			}
		}
		sBMP++;

		//4 bytes on 7th buffer array
		for(uint8_t i=0;i < 30;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[6] = (1 << (i+2));
			}else{
				GPbuff[6] = (0 << (i+2));
			}
			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

		// 1 byte on 8th buffer array
		for(uint8_t i=0;i < 8;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[7] = (1 << i);
			}else{
				GPbuff[7] = (0 << i);
			}
			if((i%6) == 5){// move to next bitmap when we complete previous 6bit BMP
			sBMP++;
			}
		}

		for(uint8_t i=0; i < 4;i++){
			if((uintptr_t)sBMP & (1 << reOrder[EvOd][i%6])){
				GPbuff[7] = (1 << (i+8));
			}else{
				GPbuff[7] = (0 << (i+8));
			}
		}

	if(Grid == 52){
			GPbuff[7] = (1 << 12);
			GPbuff[8] = (1 << 31);
	}else{
			if(Grid < 20){
				GPbuff[7] = (1 << (Grid+11));
				GPbuff[7] = (1 << (Grid+12));
			}else if(Grid == 20){
				GPbuff[7] = (1 << 31);
				GPbuff[8] = (1 << 0);
			}else{
				GPbuff[8] = (1 << (Grid-21));
				GPbuff[8] = (1 << (Grid-20));
			}
	}

}

void VFDUpdate(){
	HAL_GPIO_WritePin(GPIOE, LD4_Pin, GPIO_PIN_SET);// BLK high
	HAL_GPIO_WritePin(GPIOE, LD3_Pin, GPIO_PIN_SET);// LAT high
	for(uint8_t count;count < 256;count++){}
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	VFDLoadBMP(1, img1);
	VFDUpdate();
	HAL_Delay(100);
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
