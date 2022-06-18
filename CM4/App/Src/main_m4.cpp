/*
 * main.cpp
 *
 *  Created on: Jun 12, 2022
 *      Author: Sven
 */

#include <mqtt_intercom.h>
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "lwip.h"
#include "mqtt_m4.h"
#include "mqtt_intercom.h"

#define DHCP_STATE_BOUND  10

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

static void bootSystem(void);
static void setupExternalInterrupts(void);

volatile uint32_t msTime;

int main(void)
{
  bootSystem();

  /* Initialize all peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  printf("Hello from M4!\n");

  setupExternalInterrupts();

  MX_LWIP_Init();
  printf("LWIP initialized\n");

  // Wait sometime for dhcp bind
  while (ethernetif_dhcp_state != DHCP_STATE_BOUND)
  {
    MX_LWIP_Process();
  }

  ip_addr_t host;
  IP4_ADDR(&host, 10, 20, 30, 1);

  mqtt_m4__init(host, 1883, "tetris_test");
  mqtt_m4__connect();
  mqtt_m4__subscribe("test_subscribe", 0);
  mqtt_m4__publish("tetris_publish", (uint8_t*) &host, sizeof(ip_addr_t), MQTT_M4__QOS_MAX_ONCE, MQTT_M4__NO_RETAIN);

  mqtt_intercom__init();

  while (1)
  {
    msTime = HAL_GetTick();
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
  MPU_Config_ext();
}

void setupExternalInterrupts(void)
{
//  // Setup incoming interrupt EXTI1
//  HAL_EXTI_EdgeConfig( EXTI_LINE1, EXTI_RISING_EDGE);
//  HAL_NVIC_SetPriority(EXTI1_IRQn, 0xFU, 0U);
//  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
//
//  // Setup outgoing Interrupt EXTI0
//  HAL_EXTI_D1_EventInputConfig(EXTI_LINE0, EXTI_MODE_IT, DISABLE);
//  HAL_EXTI_D2_EventInputConfig(EXTI_LINE0, EXTI_MODE_IT, ENABLE);
//
//  __enable_irq();
}

//void EXTI1_IRQHandler(uint16_t GPIO_Pin)
//{
//  printf("Main M4: HAL_GPIO_EXTI_Callback()\n");
//  UNUSED(GPIO_Pin);
//  HAL_EXTI_D2_ClearFlag( EXTI_LINE1);
//}

void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
  if(SemMask & __HAL_HSEM_SEMID_TO_MASK(HSEM_INTERCOM))
  {
    if(mqtt_intercom__state == INTERCOM_IDLE)
    {
      mqtt_intercom__receive();
    }
    else
    {
      printf("Intercom M4: Ack\n");
    }
  }
}


