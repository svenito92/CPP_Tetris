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

#define HSEM_INTERCOM (1U)

#define INTERCOM_MEM_SIZE 1024
#define INTERCOM_TOPIC_MAX_LENGTH 128
#define INTERCOM_DATA_MAX_LENGTH 512



typedef enum
{
  MQTT_PUBLISH, MQTT_RECEIVE, MQTT_SUBSCRIBE
} intercom_cmd_t;

typedef enum
{
  INTERCOM_NO_INIT, INTERCOM_IDLE, INTERCOM_SEND, INTERCOM_RECEIVE
} intercom_state_t;

typedef struct
{
  uint8_t m4_ready;
  intercom_cmd_t cmd;
  char topic[INTERCOM_TOPIC_MAX_LENGTH];
  uint8_t data[INTERCOM_DATA_MAX_LENGTH];
} intercom_data_t;

void mqtt_intercom__init(void);
void mqtt_intercom__set_m4_ready(void);
void mqtt_intercom__send(intercom_data_t *data);
void mqtt_intercom__hsem_it(void);
void mqtt_intercom__receive_cb(intercom_data_t * data);

extern volatile intercom_state_t mqtt_intercom__state;

#ifdef __cplusplus
}
#endif

#endif /* INTERCOM_MQTT_INTERCOM_H_ */
