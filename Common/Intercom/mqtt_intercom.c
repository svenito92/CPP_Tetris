/*
 * intercom.c
 *
 *  Created on: Jun 17, 2022
 *      Author: Sven
 */

#include "mqtt_intercom.h"
#include "string.h"

static void mqtt_intercom__release_hsem();

intercom_data_t __attribute__ ((section(".intercom_buffer"))) intercom_data;
volatile intercom_state_t mqtt_intercom__state = INTERCOM_NO_INIT;

void mqtt_intercom__init(void)
{
  mqtt_intercom__state = INTERCOM_IDLE;

  // Clear memory
  while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
  memset(&intercom_data, 0x00, INTERCOM_MEM_SIZE);
  mqtt_intercom__release_hsem();
}

void mqtt_intercom__set_m4_ready(void)
{
  while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
  intercom_data.cmd = M4_READY;
  mqtt_intercom__release_hsem();
}

void mqtt_intercom__send(intercom_data_t *data)
{
  mqtt_intercom__state = INTERCOM_SEND;

  while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
  memcpy(&intercom_data, data, sizeof(intercom_data_t));
  mqtt_intercom__release_hsem();
}

void mqtt_intercom__hsem_it(void)
{
  if (mqtt_intercom__state == INTERCOM_IDLE)
  {
    mqtt_intercom__state = INTERCOM_RECEIVE;

    while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
    mqtt_intercom__receive_cb(&intercom_data);
    mqtt_intercom__release_hsem();

    // Only go back to idle, if no send command was given during callback
    if (mqtt_intercom__state == INTERCOM_RECEIVE)
    {
      mqtt_intercom__state = INTERCOM_IDLE;
    }
  }
  else
  {
    // Acknowledge of successful send
  }
}

static void mqtt_intercom__release_hsem()
{
  HAL_HSEM_DeactivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_INTERCOM));
  HAL_HSEM_Release(HSEM_INTERCOM, 0);
  HSEM_COMMON->ICR = (__HAL_HSEM_SEMID_TO_MASK(HSEM_INTERCOM));
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_INTERCOM));
}

__weak void mqtt_intercom__receive_cb(intercom_data_t *data)
{
  UNUSED(data);
}

