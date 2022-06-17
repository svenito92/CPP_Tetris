/*
 * main.cpp
 *
 *  Created on: Jun 12, 2022
 *      Author: Sven
 */

// Core Includes
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

// App Includes
#include "Looper.h"

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

void bootSystem(void);
void setupExternalInterrupts(void);

int main(void)
{
  bootSystem();
  setupExternalInterrupts();

  /* Initialize all peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  MX_SPI5_Init();

  printf("Hello from M7!\n");

#ifndef DEBUG_M4_ONLY // Used to exclude M7 from running game to help debug M4 Core
  Looper looper = Looper();
  looper.run();
#endif
  while (1)
  {
    /* This is where the interrupt would be generated. */
    HAL_EXTI_GenerateSWInterrupt(EXTI_LINE1);
    HAL_Delay(1000);
  }
}

void bootSystem(void)
{
  int32_t timeout;

  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if (timeout < 0)
  {
    Error_Handler();
  }

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();
  /* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of HSEM notification */
  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  /*Take HSEM */
  HAL_HSEM_FastTake(HSEM_ID_0);
  /*Release HSEM in order to notify the CPU2(CM4)*/
  HAL_HSEM_Release(HSEM_ID_0, 0);
  /* wait until CPU2 wakes up from stop mode */
  timeout = 0xFFFF;
  while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
  if (timeout < 0)
  {
    Error_Handler();
  }
}

void setupExternalInterrupts(void)
{
  // Setup incoming interrupt EXTI0
  HAL_EXTI_EdgeConfig( EXTI_LINE0, EXTI_RISING_EDGE);
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0xFU, 0U);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  // Setup outgoing Interrupt EXTI1
  HAL_EXTI_D1_EventInputConfig(EXTI_LINE1, EXTI_MODE_IT, DISABLE);
  HAL_EXTI_D2_EventInputConfig(EXTI_LINE1, EXTI_MODE_IT, ENABLE);
}

void EXTI0_IRQHandler(uint16_t GPIO_Pin)
{
  printf("Main M7: HAL_GPIO_EXTI_Callback()\n");
  UNUSED(GPIO_Pin);
  HAL_EXTI_D1_ClearFlag( EXTI_LINE0);
}
