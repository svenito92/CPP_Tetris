/*
 * intercom.h
 *
 *  Created on: Jun 17, 2022
 *      Author: Sven
 */

#ifndef INTERCOM_MQTT_INTERCOM_H_
#define INTERCOM_MQTT_INTERCOM_H_

#include "main.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define INTERCOM_HSEM (1U)

#define INTERCOM_TIMEOUT 1000
#define INTERCOM_MEM_SIZE 1024
#define INTERCOM_TOPIC_MAX_LENGTH 128
#define INTERCOM_DATA_MAX_LENGTH 256

#ifdef CORE_CM4 //Set interrupts according to how HAL library handles it
#define HSEMx_IRQn HSEM2_IRQn
#else
#define HSEMx_IRQn HSEM1_IRQn
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef enum
{
  ERROR_CMD=0, NO_CMD, MQTT_PUBLISH, MQTT_RECEIVE, MQTT_SUBSCRIBE, M4_READY, M4_NOT_READY
} intercom_cmd_t;

typedef enum
{
  ERROR_STATE=0, INTERCOM_NO_INIT, INTERCOM_M4_READY, INTERCOM_IDLE, INTERCOM_SEND, INTERCOM_RECEIVE, INTERCOM_ACK_WAIT
} intercom_state_t;

typedef struct
{
  intercom_cmd_t cmd;
  uint16_t data_length;
  char topic[INTERCOM_TOPIC_MAX_LENGTH];
  uint8_t data[INTERCOM_DATA_MAX_LENGTH];
} intercom_data_t;

uint8_t mqtt_intercom__init(uint8_t erase);
void mqtt_intercom__handler(void);
uint8_t mqtt_intercom__set_m4_ready(void);
uint8_t mqtt_intercom__send(intercom_data_t *data);
uint8_t mqtt_intercom__send_blocking(intercom_data_t *data, uint32_t timeout);
void mqtt_intercom__hsem_it(void);
void mqtt_intercom__receive_cb(intercom_data_t * data);
void print_hsem_flags();

extern volatile intercom_state_t mqtt_intercom__state;

#ifdef __cplusplus
}
#endif

#endif /* INTERCOM_MQTT_INTERCOM_H_ */
