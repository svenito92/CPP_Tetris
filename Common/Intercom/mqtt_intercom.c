/*
 * intercom.c
 *
 *  Created on: Jun 17, 2022
 *      Author: Sven
 */

#include "mqtt_intercom.h"
#include "string.h"

intercom_data_t __attribute__ ((section(".intercom_buffer"))) intercom_data;
volatile intercom_state_t mqtt_intercom__state = INTERCOM_NO_INIT;

void mqtt_intercom__init(void)
{
  mqtt_intercom__state = INTERCOM_IDLE;

  // Activate Notification for completed callback (Implement HAL_HSEM_FreeCallback)
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_INTERCOM));

  // First Deactivate. Only activate when sending
  HAL_HSEM_DeactivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_INTERCOM));

  // Clear memory
  HAL_HSEM_FastTake(HSEM_INTERCOM);
  memset(&intercom_data, 0x00, INTERCOM_MEM_SIZE);
  HAL_HSEM_Release(HSEM_INTERCOM, 0);
}

void mqtt_intercom__set_m4_ready(void)
{
  while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
  intercom_data.m4_ready = 1;
  HAL_HSEM_Release(HSEM_INTERCOM, 0);
}

void mqtt_intercom__send(intercom_data_t *data)
{
  mqtt_intercom__state = INTERCOM_SEND;

  data->m4_ready = intercom_data.m4_ready; // Do not allow m4_ready to change

  // Copy when HSEM available
  while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
  memcpy(&intercom_data.data, sizeof(intercom_data_t));
  HAL_HSEM_Release(HSEM_INTERCOM, 0);
}

void mqtt_intercom__receive(void)
{
  mqtt_intercom__state = INTERCOM_RECEIVE;

  while (HAL_HSEM_FastTake(HSEM_INTERCOM) != HAL_OK);
  mqtt_intercom__receive_cb(&intercom_data);
  HAL_HSEM_Release(HSEM_INTERCOM, 0);

  // Only go back to idle, if no send command was given during callback
  if(mqtt_intercom__state == INTERCOM_RECEIVE)
  {
    mqtt_intercom__state = INTERCOM_IDLE;
  }
}

__weak void mqtt_intercom__receive_cb(intercom_data_t * data)
{
  UNUSED(data);
}

