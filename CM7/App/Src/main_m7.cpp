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
#include "mqtt_intercom.h"
#include <string>

// App Includes
#include "Looper.h"

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

void bootSystem(void);
void setupExternalInterrupts(void);

Looper looper  = Looper();

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
 // Looper looper = Looper();
  looper.run();
#endif
  HAL_Delay(10000);
  intercom_data_t mqtt_data;
  mqtt_data.cmd = MQTT_PUBLISH;
  sprintf((char*)&mqtt_data.topic, "data_from_m7");
  mqtt_data.data_length = 10;
  uint32_t count = 0;
  while (1)
  {
    /* This is where the interrupt would be generated. */

    count++;
    sprintf((char*)&mqtt_data.data, "%0*ld", 10, count);

    mqtt_intercom__send(&mqtt_data);
//    while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
//    HAL_HSEM_Release(HSEM_INTERCOM, 0);
    HAL_Delay(5000);
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
  MPU_Config_ext();

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
//  // Setup incoming interrupt EXTI0
//  HAL_EXTI_EdgeConfig( EXTI_LINE0, EXTI_RISING_EDGE);
//  HAL_NVIC_SetPriority(EXTI0_IRQn, 0xFU, 0U);
//  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
//
//  // Setup outgoing Interrupt EXTI1
//  HAL_EXTI_D1_EventInputConfig(EXTI_LINE1, EXTI_MODE_IT, DISABLE);
//  HAL_EXTI_D2_EventInputConfig(EXTI_LINE1, EXTI_MODE_IT, ENABLE);
}

//void EXTI0_IRQHandler(uint16_t GPIO_Pin)
//{
//  printf("Main M7: HAL_GPIO_EXTI_Callback()\n");
//  UNUSED(GPIO_Pin);
//  HAL_EXTI_D1_ClearFlag( EXTI_LINE0);
//}

void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
  printf("HSEM M7: FreeCallback!\n");
  if(SemMask & __HAL_HSEM_SEMID_TO_MASK(HSEM_INTERCOM))
  {
      mqtt_intercom__hsem_it();
  }
}

void mqtt_intercom__receive_cb(intercom_data_t * data)
{
  printf("Intercom M7: Receive callback");
  // Interpret command
std::string topic = "test";

	switch(data->cmd){

	case M4_READY:		looper.interCoreComReady = true;
					break;
	case M4_NOT_READY:	looper.interCoreComReady = false;
					break;

	case MQTT_RECEIVE:	if(topic.compare("test")==0){
						looper.gameStartFlag=true;
					}
					else{
						looper.gameStartFlag=false;
					}
					break;
	default	:		break;

  }
}
