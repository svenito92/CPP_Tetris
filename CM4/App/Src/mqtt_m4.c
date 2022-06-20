/*
 * mqtt_m4.c
 *
 *  Created on: 17 Jun 2022
 *      Author: Sven
 */

#include "mqtt_m4.h"

#include "stdio.h"
#include "string.h"
#include "mqtt_intercom.h"

// Private variables declaration
static mqtt_client_t *_client;
static ip_addr_t _host;
static uint16_t _port;
static const char *_client_id;
static char _topic[MQTT_M4__MAX_TOPIC_LENGTH];
static mqtt_m4_msg_t _messageBuffer[MQTT_M4__MESSAGE_BUFFER_LENGTH];
static uint8_t _messageCount;

// Private function declaration
static void mqtt_m4__connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_m4__incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_m4__incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_m4__sub_request_cb(void *arg, err_t err);

void mqtt_m4__init(ip_addr_t host, uint16_t port, const char *client_id)
{
  printf("MQTT M4: mqtt_m4__init()\n");
  _host = host;
  _port = port;
  _client_id = client_id;

  // Clear message Buffer
  _messageCount = 0;
  memset(_messageBuffer, 0x00, MQTT_M4__MESSAGE_BUFFER_LENGTH * sizeof(mqtt_m4_msg_t));

  _client = mqtt_client_new();
  if (_client != NULL)
  {
    printf("MQTT M4: Client memory initialised\n");
  }
  else
  {
    printf("MQTT M4: Client memory NOT initialised\n");
  }
}

void mqtt_m4__handler()
{
  intercom_data_t mqtt_data;
  uint16_t length = _messageBuffer[_messageCount - 1].length;
  uint8_t successfulSend;

  if (_messageCount > 0)
  {
    mqtt_data.cmd = MQTT_RECEIVE;
    mqtt_data.data_length = length;
    strcpy((char*) mqtt_data.data, _messageBuffer[_messageCount - 1].topic);
    if (length > 0)
    {
      memcpy(mqtt_data.data, _messageBuffer[_messageCount - 1].message, length);
    }
    printf("MQTT M4: Intercom send from buffer\n");
    successfulSend = mqtt_intercom__send(&mqtt_data);
    if (successfulSend == TRUE)
    {
      _messageCount--;
    }
  }
}

void mqtt_m4__connect()
{
  printf("MQTT M4: mqtt_m4__connect()\n");
  err_t err;
  struct mqtt_connect_client_info_t client_info;

  memset(&client_info, 0, sizeof(client_info));
  client_info.client_id = _client_id;

  err = mqtt_client_connect(_client, &_host, _port, mqtt_m4__connection_cb, (void*) &_topic, &client_info);
  if (err != ERR_OK)
  {
    printf("MQTT M4: mqtt_connect error: %d\n", err);
  }
}

void mqtt_m4__subscribe(const char *topic, uint8_t qos)
{
  printf("MQTT M4: mqtt_m4__subscribe()\n");
  err_t err = ERR_OK;

  err = mqtt_subscribe(_client, topic, qos, mqtt_m4__sub_request_cb, 0);
  if (err == ERR_OK)
  {
    printf("MQTT M4: Successfully subscribed to '%s'\n", topic);
  }
  else
  {
    printf("MQTT M4: Error while subscribing, return: %d\n", err);
  }
}

void mqtt_m4__publish(const char *topic, uint8_t *buf, uint16_t len, uint8_t qos, uint8_t retain)
{
  printf("MQTT M4: mqtt_m4__publish()\n");
  err_t err = ERR_OK;

  err = mqtt_publish(_client, topic, buf, len, qos, retain, mqtt_m4__sub_request_cb, 0);
  if (err == ERR_OK)
  {
    printf("MQTT M4: Successfully published to '%s'\n", topic);
  }
  else
  {
    printf("MQTT M4: Error while publishing, return: %d\n", err);
  }
}

static void mqtt_m4__connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  printf("MQTT M4: mqtt_m4__connection_cb()\n");
//  err_t err = ERR_OK;
  if (status == MQTT_CONNECT_ACCEPTED)
  {
    printf("MQTT M4: Successfully connected\n");

    /* Setup callback for incoming publish requests */
    mqtt_set_inpub_callback(client, mqtt_m4__incoming_publish_cb, mqtt_m4__incoming_data_cb, arg);

//    /* Subscribe to a topic named "subtopic" with QoS level 1, call mqtt_sub_request_cb with result */
//    err = mqtt_subscribe(client, "subtopic", 1, mqtt_m4__sub_request_cb, arg);
//
//    if (err == ERR_OK)
//    {
//      printf("MQTT M4: Successfully connected to \n", err);
//    }
//    else
//    {
//      printf("MQTT M4: mqtt_subscribe return: %d\n", err);
//    }
  }
  else
  {
    printf("MQTT M4: Disconnected, reason: %d\n", status);

    /* Its more nice to be connected, so try to reconnect */
    mqtt_m4__connect();
  }
}

static void mqtt_m4__incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  printf("MQTT M4: Incoming publish callback from topic '%s'\n", topic);
  char *topic_buf = (char*) arg;
  strcpy(topic_buf, topic);

  if (tot_len == 0)
  {
    mqtt_m4__incoming_data_cb(arg, 0, 0, MQTT_DATA_FLAG_LAST);
  }
}

static void mqtt_m4__incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  char *topic_buf = (char*) arg;
  intercom_data_t mqtt_data;
  uint8_t successfulSend;

  printf("MQTT M4: Incoming data callback! Length: %d\n", len);

  if (flags == MQTT_DATA_FLAG_LAST)
  {
    if (len > INTERCOM_DATA_MAX_LENGTH)
    {
      len = INTERCOM_DATA_MAX_LENGTH;
      printf("MQTT M4: Too large publish data (overhead is truncated)!\n");
    }

    mqtt_data.cmd = MQTT_RECEIVE;
    mqtt_data.data_length = len;
    strcpy((char*) mqtt_data.data, topic_buf);
    if (len > 0)
    {
      memcpy(mqtt_data.data, data, len);
    }
    printf("MQTT M4: Intercom send...\n");
    successfulSend = mqtt_intercom__send(&mqtt_data);
    if (successfulSend == FALSE)
    {
      if (_messageCount < MQTT_M4__MESSAGE_BUFFER_LENGTH - 1)
      {
        _messageCount++;
        _messageBuffer[_messageCount].length = len;
        strcpy(_messageBuffer[_messageCount].topic, topic_buf);
        memcpy(_messageBuffer[_messageCount].message, data, len);
      }
      else
      {
        printf("MQTT M4: Message Buffer Overflow!!!\n");
      }
    }
  }
  else
  {
    printf("MQTT M4: Too large publish data!\n");
  }

}

static void mqtt_m4__sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity,
   normal behaviour would be to take some action if subscribe fails like
   notifying user, retry subscribe or disconnect from server */
  printf("MQTT M4: Subscribe result: %d\n", result);
}

