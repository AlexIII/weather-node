/*
 * Weather Node v1.0
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

#ifndef DHT22_H_INCLUDED
#define DHT22_H_INCLUDED

#include "gpio.h"

#define DHT22_DATA_PIN GPIO_PIN_ID_P1_6

typedef struct {
    uint8_t humidity[2];
    uint8_t temperature[2];
    uint8_t crc;
} dht22_data_t;

bool dht22_read(dht22_data_t* dht22_data);

#endif // DHT22_H_INCLUDED
