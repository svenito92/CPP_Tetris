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


#include "mqtt_m4.h"

#define DHCP_STATE_BOUND  10

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

void bootSystem(void);

volatile uint32_t msTime;

int main(void)
{
  bootSystem();

  /* Initialize all peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_LWIP_Init();
  MX_LWIP_Process();

  // Wait sometime for dhcp bind
  while (ethernetif_dhcp_state != DHCP_STATE_BOUND)
  {
    MX_LWIP_Process();
  }
//  uint32_t timestamp = HAL_GetTick();
//  while (HAL_GetTick() < timestamp + 10000)
//  {
//    MX_LWIP_Process();
//  }

  ip_addr_t host;
  IP4_ADDR(&host, 10, 20, 30, 1);

  mqtt_m4__init(host, 1883, "tetris_test");
  mqtt_m4__connect();
  mqtt_m4__subscribe("test_subscribe", 0);
  mqtt_m4__publish("tetris_publish", (uint8_t*)&host , sizeof(ip_addr_t), MQTT_M4__QOS_MAX_ONCE, MQTT_M4__NO_RETAIN);

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
}

