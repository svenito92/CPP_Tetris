/*
 * intercom.c
 *
 *  Created on: Jun 17, 2022
 *      Author: Sven
 */

#include <mqtt_intercom.h>
#include "string.h"

intercom_data_t __attribute__ ((section(".intercom_buffer"))) intercom_data;

void mqtt_intercom__mem_init(void)
{
  memset(&intercom_data, 0xA5, INTERCOM_MEM_SIZE);
}

