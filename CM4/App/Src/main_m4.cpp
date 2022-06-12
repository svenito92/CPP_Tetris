/*
 * main.cpp
 *
 *  Created on: Jun 12, 2022
 *      Author: Sven
 */

#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "lwip.h"

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

void bootSystem(void);

int main(void)
{
  bootSystem();

  /* Initialize all peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_LWIP_Init();

  while (1)
  {
    MX_LWIP_Process();
  }
}

void bootSystem(void)
{
  __HAL_RCC_HSEM_CLK_ENABLE();
  /* Activate HSEM notification for Cortex-M4*/
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

  /* Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for Cortex-M7 to
   perform system initialization (system clock config, external memory configuration.. )*/
  HAL_PWREx_ClearPendingEvent();
  HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);
  /* Clear HSEM flag */
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
}

