/*
 *******************************************************************************
 * Copyright (c) 2019, STMicroelectronics
 * All rights reserved.
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

#include "pins_arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

// Digital PinName array
// This array allows to wrap Arduino pin number(Dx or x)
// to STM32 PinName (PX_n)
const PinName digitalPin[] = {
	//Left side
  //PX_n, //Dx
  PB_14, //D0
  PB_13, //D1
  PB_12, //D2
  PB_11, //D3
  PB_10, //D4
  PB_2, //D5
  PB_1, //D6 //A9
  PB_0, //D7 //A8
  PA_7, //D8 //A7
  PA_6, //D9 //A6
  PA_5, //D10//A5
  PA_3, //D11//A3
  PA_2, //D12//A2
  PA_1, //D13//A1
  PA_0, //D14//A0
  PC_3, //D15//A13
  PC_2, //D16//A12
  PC_1, //D17//A11
  PC_0, //D18//A10
  PC_13,//D19
   
  //Right side
  PB_15, //D20
  PC_6, //D21
  PC_7, //D22
  PC_8,	//D23
  PC_9, //D24
  PA_8, //D25
  PA_9, //D26
  PA_10, //D27
  PA_11, //D28
  PA_12, //D29
  PA_15, //D30
  PC_10, //D31
  PC_11, //D32
  PC_12, //D33
  PD_2, //D34
  PB_3, //D35
  PB_4, //D36
  PB_5, //D37
  PB_6, //D38
  PB_7, //D39
  PB_8, //D40
  PB_9, //D41
  //Other
  PA_13, //SWDIO
  PA_14, //SWCLK
  PA_4,  //NSS LORA //A4
  PC_4,  //RESET LORA //A14
  PC_5,  //DIO0 LORA //A15
  
  // Required only if Ax pins are automaticaly defined using `NUM_ANALOG_FIRST`
  // and have to be contiguous in this array
  // Duplicated pins in order to be aligned with PinMap_ADC
  PA_0, //D47 /A0 =	D14
  PA_1, //D48 /A1 = D13
  PA_2, //D49 /A2 = D12
  PA_3, //D50 /A3 = D11
  PA_4, //D51 /A4 = NSS LORA
  PA_5, //D52 /A5 = D10
  PA_6, //D53 /A6 = D9
  PA_7, //D54 /A7 = D8
  PB_0, //D55 /A8 = D7
  PB_1, //D56 /A9 = D6
  PC_0, //D57 /A10 = D18
  PC_1, //D58 /A11 = D17
  PC_2, //D59 /A12 = D16
  PC_3, //D60 /A13 = D15
  PC_4, //D61 /A14 = RESET LORA
  PC_5, //D62 /A15 = DIO0 LORA
  


};



#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  System Clock Configuration
  * @param  None
  * @retval None
  */
WEAK void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

#ifdef __cplusplus
}
#endif
