/*
 * intercom.c
 *
 *  Created on: Jun 17, 2022
 *      Author: Sven
 */

#include "mqtt_intercom.h"
#include "string.h"

static inline void hsem_release();
static inline uint8_t hsem_obtained();

intercom_data_t temp_data;
intercom_data_t __attribute__ ((section(".intercom_buffer"))) intercom_data;
volatile intercom_state_t mqtt_intercom__state = INTERCOM_NO_INIT;

volatile uint8_t hsem_wait = FALSE;
volatile uint8_t send_pending = FALSE;
volatile uint8_t m4_ready_pending = FALSE;
volatile uint8_t receive_pending = FALSE;
volatile uint8_t ack_pending = FALSE;

const char *states[] = { "ERROR_STATE", "INTERCOM_NO_INIT", "INTERCOM_M4_READY", "INTERCOM_IDLE", "INTERCOM_SEND",
    "INTERCOM_RECEIVE", "INTERCOM_ACK_WAIT" };
uint32_t timestamp = 0;

uint8_t mqtt_intercom__init(uint8_t erase)
{
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(INTERCOM_HSEM));
  HAL_NVIC_ClearPendingIRQ(HSEMx_IRQn);
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(INTERCOM_HSEM));

  if (erase == TRUE)
  {
    // Clear memory
    uint32_t ts = HAL_GetTick();
    while (hsem_obtained() == FALSE)
    {
      if (HAL_GetTick() - ts > INTERCOM_TIMEOUT)
      {
        return FALSE;
      }
    }
    memset(&intercom_data, 0x00, INTERCOM_MEM_SIZE);
    hsem_release();
  }
  mqtt_intercom__state = INTERCOM_IDLE;
  return TRUE;
}

// State Machine
void mqtt_intercom__handle(void)
{
#ifdef DEBUG_M4_ONLY
  if (HAL_GetTick() - timestamp > 100)
  {
    timestamp = HAL_GetTick();
    printf("IC State: %s\n", states[mqtt_intercom__state]);
#endif
  switch (mqtt_intercom__state)
  {
  case INTERCOM_M4_READY:
    if (hsem_obtained() == TRUE)
    {
      printf("IC: M4 rdy\n");
      intercom_data.cmd = M4_READY;
      hsem_release();
      m4_ready_pending = FALSE;
      mqtt_intercom__state = INTERCOM_ACK_WAIT;
    }
    break;
  case INTERCOM_SEND:
    if (hsem_obtained() == TRUE)
    {
      printf("IC: send\n");
      memcpy(&intercom_data, &temp_data, sizeof(intercom_data_t));
      hsem_release();
      send_pending = FALSE;
      mqtt_intercom__state = INTERCOM_ACK_WAIT;
    }
    break;
  case INTERCOM_RECEIVE:
    if (hsem_obtained() == TRUE)
    {
      printf("IC: receive\n");
      mqtt_intercom__receive_cb(&intercom_data);
      hsem_release();
      receive_pending = FALSE;
      mqtt_intercom__state = INTERCOM_IDLE;
    }
    break;
  case INTERCOM_IDLE:
    if (send_pending == TRUE)
    {
      mqtt_intercom__state = INTERCOM_SEND;
    }
    if (m4_ready_pending == TRUE)
    {
      mqtt_intercom__state = INTERCOM_M4_READY;
    }
    if (receive_pending == TRUE)
    {
      mqtt_intercom__state = INTERCOM_RECEIVE;
    }
    break;
  case INTERCOM_ACK_WAIT:
    if (ack_pending == TRUE)
    {
      mqtt_intercom__state = INTERCOM_IDLE;
      ack_pending = FALSE;
    }
    break;
  default:
    break;
  }
#ifdef DEBUG_M4_ONLY
  }
#endif
}

// Flag for M7 to show that M4 Core is ready for communication
uint8_t mqtt_intercom__set_m4_ready(void)
{
  //printf("set_ready command...");
  if (m4_ready_pending == FALSE)
  {
    //printf("Successful\n!");
    m4_ready_pending = TRUE;
    return TRUE;
  }
  else
  {
    //printf("Error!\n");
    return FALSE;
  }
}

uint8_t mqtt_intercom__send(intercom_data_t *data)
{
  //  printf("Intercom Mx: Send data. Going to send.\n");
  //printf("send command...");
  if (send_pending == FALSE)
  {
    //printf("Successful\n!");
    send_pending = TRUE;
    memcpy(&temp_data, data, sizeof(intercom_data_t));
    return TRUE;
  }
  else
  {
    //printf("Error!\n");
    return FALSE;
  }
}

uint8_t mqtt_intercom__send_blocking(intercom_data_t *data, uint32_t timeout)
{
  uint32_t ts = HAL_GetTick();
  while (mqtt_intercom__send(data) == FALSE)
  {
    mqtt_intercom__handle();
    if (HAL_GetTick() - ts > timeout)
    {
      return FALSE;
    }
  }
  // Call handler until transmission (incl. ACK) is complete
  while ((mqtt_intercom__state != INTERCOM_IDLE) || (send_pending == TRUE))
  {
    mqtt_intercom__handle();
    if (HAL_GetTick() - ts > timeout)
    {
      return FALSE;
    }
  }
  return TRUE;
}

void mqtt_intercom__hsem_it(void)
{
  switch (mqtt_intercom__state)
  {
  case INTERCOM_IDLE:
    receive_pending = TRUE;
    // NO BREAK intentional!
  case INTERCOM_SEND:
  case INTERCOM_M4_READY:
  case INTERCOM_RECEIVE:
    // Ensure that HSEM is taken as fast as possible
    // HAL_HSEM_FastTake() will later be successful even when HSEM is already taken
    hsem_obtained();
    break;
  case INTERCOM_ACK_WAIT:
    ack_pending = TRUE;
    break;
  default:
    break;
  }
}

static inline uint8_t hsem_obtained()
{
  uint8_t got_hsem = FALSE;

  if (HAL_HSEM_FastTake(INTERCOM_HSEM) == HAL_OK)
  {
    HAL_NVIC_DisableIRQ(HSEMx_IRQn);
    hsem_wait = FALSE;
    got_hsem = TRUE;
  }
  else // Wait for HSEM release interrupt
  {
    hsem_wait = TRUE;
    HAL_NVIC_EnableIRQ(HSEMx_IRQn);
  }
  return got_hsem;
}

static inline void hsem_release()
{
  HAL_HSEM_Release(INTERCOM_HSEM, 0);
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(INTERCOM_HSEM));
  HAL_NVIC_ClearPendingIRQ(HSEMx_IRQn);
  HAL_NVIC_EnableIRQ(HSEMx_IRQn);
}

__weak void mqtt_intercom__receive_cb(intercom_data_t *data)
{
  UNUSED(data);
}

