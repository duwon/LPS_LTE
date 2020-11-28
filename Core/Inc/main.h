/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USER_BTN_Pin GPIO_PIN_0
#define USER_BTN_GPIO_Port GPIOA
#define ADC_BAT_Pin GPIO_PIN_1
#define ADC_BAT_GPIO_Port GPIOA
#define DEBUG_TX_Pin GPIO_PIN_2
#define DEBUG_TX_GPIO_Port GPIOA
#define ADC1_Pin GPIO_PIN_3
#define ADC1_GPIO_Port GPIOA
#define PWR_BATCHECK_Pin GPIO_PIN_4
#define PWR_BATCHECK_GPIO_Port GPIOA
#define DIN0_Pin GPIO_PIN_5
#define DIN0_GPIO_Port GPIOA
#define DIN1_Pin GPIO_PIN_6
#define DIN1_GPIO_Port GPIOA
#define DIN2_Pin GPIO_PIN_7
#define DIN2_GPIO_Port GPIOA
#define DIN3_Pin GPIO_PIN_0
#define DIN3_GPIO_Port GPIOB
#define PWR_12V_Pin GPIO_PIN_1
#define PWR_12V_GPIO_Port GPIOB
#define LTE_WAKEUP_Pin GPIO_PIN_8
#define LTE_WAKEUP_GPIO_Port GPIOA
#define LTE_TX_Pin GPIO_PIN_9
#define LTE_TX_GPIO_Port GPIOA
#define LTE_RX_Pin GPIO_PIN_10
#define LTE_RX_GPIO_Port GPIOA
#define PWR_RS232_Pin GPIO_PIN_11
#define PWR_RS232_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define DEBUG_RX_Pin GPIO_PIN_15
#define DEBUG_RX_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
