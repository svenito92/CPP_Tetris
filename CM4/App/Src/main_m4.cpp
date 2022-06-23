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

int main(void)
{
  ip_addr_t host;
  uint16_t port;

  bootSystem();

  /* Initialize all peripherals */
  MX_GPIO_Init();
  //MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  printf("\n\n\nHello from M4! (%s)\n", __TIME__);

#ifdef REMOTE_CONNECTION
  IP4_ADDR(&host, 144, 2, 69, 24);
  port = 54321;

#else
  IP4_ADDR(&host, 10, 20, 30, 1);
  port = 1883;
#endif

  MX_LWIP_Init();
  printf("LWIP initialized\n");


  // Wait for dhcp bind
  while (ethernetif_dhcp_state != DHCP_STATE_BOUND)
  {
    MX_LWIP_Process();
  }
  const ip4_addr_t *ip_addr = netif_ip4_addr(&gnetif);
  uint32_t ip = ip_addr->addr;
  char ipStr[16];
  sprintf(ipStr, "%d.%d.%d.%d",
      (uint8_t)(ip >> 0) & 0xFF,
      (uint8_t)(ip >> 8) & 0xFF,
      (uint8_t)(ip >>16) & 0xFF,
      (uint8_t)(ip >>24) & 0xFF);
  printf("Got IP address: %s\n", (const char *)&ipStr);

  mqtt_intercom__init(FALSE); // Init intercom without erasing

  MX_LWIP_Process();

  char clientName[30];
  sprintf(clientName, "tetris_%s", (const char *)ipStr);
  mqtt_m4__init(host, port, (const char *)clientName);
  MX_LWIP_Process();

  mqtt_m4__connect();
  MX_LWIP_Process();

  mqtt_intercom__set_m4_ready(); // Will be executed only when mqtt_intercom__handle() is called at least twice!

  while (1)
  {
    MX_LWIP_Process();
    mqtt_m4__handler();
    mqtt_intercom__handler();
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
  //HAL_NVIC_SetPriority(HSEM1_IRQn, 0xFU, 0U);
  //HAL_NVIC_EnableIRQ(HSEM1_IRQn);

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
  if (SemMask & __HAL_HSEM_SEMID_TO_MASK(INTERCOM_HSEM))
  {
    printf("it\n");
    mqtt_intercom__hsem_it();
  }
  // Re-enable the HSEM interrupt because HAL handler is disabling it...
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(INTERCOM_HSEM));
}

void mqtt_intercom__receive_cb(intercom_data_t *data)
{
  switch (data->cmd)
  {
  case MQTT_PUBLISH:
    mqtt_m4__publish(data->topic, data->data, data->data_length, MQTT_M4__QOS_MIN_ONCE, MQTT_M4__NO_RETAIN);
    break;
  case MQTT_SUBSCRIBE:
    mqtt_m4__subscribe(data->topic, 1);
    break;
  default:
    printf("Intercom M4: unknown command!\n");
    break;
  }
  printf("Intercom M4: Receive callback\n");
  // Interpret command
}

