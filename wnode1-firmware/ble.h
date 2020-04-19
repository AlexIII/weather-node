/*
 * Weather Node v1.0
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

#ifndef BLE_H_INCLUDED
#define BLE_H_INCLUDED
#include <stdint.h>

/**
Initialize BLE.
@return Pointer to payload buffer. Fill it with BLE-correct user data. (Maximum 21 byte!)
*/
uint8_t* BLE_init();

/**
De-initialize BLE.
*/
void BLE_deinit();

/**
Prepare BLE packet. Call it each time after payload buffer (see BLE_init()) has been set.
@param mac is this device's MAC (6 bytes).
@param payload_size is payload size (which has been written to payload buffer).
*/
void BLE_prepare(const uint8_t mac[6], const uint8_t payload_size);

/**
Send prepared packet via radio channel (may be called several times with different channel_idx).
@param channel_idx is an index of an advertisement channel (0, 1 or 3) over which the packet will be sent.
channel_idx == 0 : BLE channel 37 (2402 MHz)
channel_idx == 1 : BLE channel 38 (2426 MHz)
channel_idx == 2 : BLE channel 39 (2480 MHz)
*/
void BLE_send(const uint8_t channel_idx);

#endif // BLE_H_INCLUDED
