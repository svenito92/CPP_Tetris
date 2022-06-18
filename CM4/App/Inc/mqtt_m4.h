/*
 * mqtt_m4.h
 *
 *  Created on: 17 Jun 2022
 *      Author: Sven
 */

#ifndef INC_MQTT_M4_H_
#define INC_MQTT_M4_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "mqtt.h"
#include "lwip.h"

#define MQTT_M4__RETAIN     1
#define MQTT_M4__NO_RETAIN  0
#define MQTT_M4__QOS_MAX_ONCE 0
#define MQTT_M4__QOS_MIN_ONCE 1
#define MQTT_M4__QOS_EXACT_ONCE 2

#define MQTT_M4__MAX_TOPIC_LENGTH 128


void mqtt_m4__init(ip_addr_t host, uint16_t port, const char *client_id);
void mqtt_m4__connect();
void mqtt_m4__subscribe(const char *topic, uint8_t qos);
void mqtt_m4__publish(const char *topic, uint8_t *buf, uint16_t len, uint8_t qos, uint8_t retain);

#ifdef __cplusplus
}
#endif

#endif /* INC_MQTT_M4_H_ */
