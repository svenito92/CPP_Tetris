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

Looper looper = Looper();
#ifdef DEBUG_M4_ONLY
volatile uint8_t isM4Ready = 0;
#endif

int main(void)
{
  bootSystem();
  mqtt_intercom__init(TRUE);

  /* Initialize all peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  printf("\n\n\nHello from M7! (%s)\n", __TIME__);

  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  MX_SPI5_Init();

#ifdef DEBUG_M4_ONLY // Used to exclude M7 from running game to help debug M4 Core
  printf("Wait for M4 Core Ready\n");
  while (isM4Ready == 0)
  {
    mqtt_intercom__handle();
  }
  intercom_data_t mqtt_data;
  mqtt_data.cmd = MQTT_SUBSCRIBE;
  sprintf((char*) &mqtt_data.topic, "data_to_m7");
  mqtt_intercom__send(&mqtt_data);
//  mqtt_data.data_length = 10;
//  uint32_t count = 0;
  while (1)
  {
    mqtt_intercom__handle();
  }
#else
  // Looper looper = Looper();
  looper.run();
#endif
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
#ifdef DEBUG_M4_ONLY
  // Interpret command
  HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
  switch (data->cmd)
  {
  case MQTT_RECEIVE:
    printf("Intercom M7: Receive command 'MQTT_RECEIVE'\n");
    printf("Data:\n%s\n", (char*) &data->data);
    break;
  case M4_READY:
    isM4Ready = 1;
    printf("Intercom M7: Receive command 'M4_READY'\n");
    break;
  case ERROR_CMD:
    HSEM_COMMON->ICR = (__HAL_HSEM_SEMID_TO_MASK(INTERCOM_HSEM));
    printf("Intercom M7: Reset interrupt ('ERROR_CMD')\n");
    break;
  default:
    printf("Intercom M7: Invalid command: %d\n", data->cmd);
    break;
  }

#else // Used to exclude M7 from running game to help debug M4 Core
  std::string topic = "test";
  uint8_t i=0;
  switch (data->cmd)
  {

  case M4_READY:
    looper.interCoreComReady = true;
    break;
  case M4_NOT_READY:
    looper.interCoreComReady = false;
    break;

  case MQTT_RECEIVE:
		if (strcmp((const char*)data->topic,"StartGame") == 0){
			if(data->data[0]==0x01){
				looper.gameStartFlag = true;
			}
		}
		else if (strcmp((const char*)data->topic,"Players") == 0){
			i=0;
			if(data->data[0]!=looper.playerNr){
				while(i<20){
					if((data->data[0]!=looper.playerIds[i])&&(looper.playerIds[i]!=0)){
						i++;
					}
					else{
						if(data->data[1]==0){
							while(looper.playerIds[i]!=0){
								looper.playerIds[i]=looper.playerIds[i+1];
								i++;
							}
							i=20;
						}
						else{
							looper.playerIds[i]=data->data[0];
							i=20;
						}
					}
				}
			}
		}
		else if (strcmp((const char*)data->topic,"KillLine") == 0){
			if(data->data[0]!=looper.playerNr){
				looper.toInsertLines=looper.toInsertLines + (uint32_t)data->data[1];
			}
		}
		else if (strcmp((const char*)data->topic,"GameOver") == 0){
			i=0;
			if(data->data[0]!=looper.playerNr){
				while(i<20){
					if((data->data[0]!=looper.gameOverPlayerIds[i])&&(looper.gameOverPlayerIds[i]!=0)){
						i++;
					}
					else{
						looper.gameOverPlayerIds[i]=data->data[0];
						i=20;
					}
				}
			}
		}
		else if (strcmp((const char*)data->topic,"GameWon") == 0){
			if(looper.gameWonPlayerId == 0){
				looper.gameWonPlayerId = data->data[0];
				looper.gameWonFlag = true;
			}
		}
		break;
  default:
    break;

  }
#endif
}
