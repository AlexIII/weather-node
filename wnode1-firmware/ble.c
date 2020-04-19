/*
 * Weather Node v1.0
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

#include <string.h>
#include "rf.h"
#include "ble.h"

typedef struct btle_adv_pdu_t {
  // packet header
  uint8_t pdu_type; // PDU type
  uint8_t pl_size;  // payload size
  // MAC address
  uint8_t mac[6];
  // payload (including 3 bytes for CRC)
  uint8_t payload[24];
} btle_adv_pdu_t;

static btle_adv_pdu_t ble_buffer;
static const uint8_t BLE_adv_channel[3]   = {37, 38, 39};  // logical BTLE channel number (37-39)
static const uint8_t BLE_adv_frequency[3] = { 2, 26, 80};  // physical frequency (2400+x MHz)

static void initRF() {
    //set advertisement address: 0x8E89BED6 (bit-reversed -> 0x6B7D9171)
    const uint8_t addr[5] = {0x71, 0x91, 0x7D, 0x6B, 0x00};
	rf_configure(RF_CONFIG_PWR_UP, //tx
				 true,
				 RF_EN_AA_ENAA_NONE,            //no auto ack
				 RF_EN_RXADDR_ERX_NONE,         //no rx
				 RF_SETUP_AW_4BYTES,            //4 byte addr
				 RF_SETUP_RETR_DISABLE,         //no retry
				 0,                             //channel
				 RF_RF_SETUP_RF_DR_1_MBPS | RF_RF_SETUP_RF_PWR_0_DBM,      //speed 1MBPS, 0dbm
				 NULL,
				 NULL,
				 RF_RX_ADDR_P2_DEFAULT_VAL,
				 RF_RX_ADDR_P3_DEFAULT_VAL,
				 RF_RX_ADDR_P4_DEFAULT_VAL,
				 RF_RX_ADDR_P5_DEFAULT_VAL,
				 addr,
				 RF_RX_PW_P0_DEFAULT_VAL,
				 RF_RX_PW_P1_DEFAULT_VAL,
				 RF_RX_PW_P2_DEFAULT_VAL,
				 RF_RX_PW_P3_DEFAULT_VAL,
				 RF_RX_PW_P4_DEFAULT_VAL,
				 RF_RX_PW_P5_DEFAULT_VAL,
				 RF_DYNPD_DPL_NONE,
				 RF_FEATURE_NONE);
}

//returns pointer to payload (max 21 byte)
uint8_t* BLE_init() {
    initRF();
    return ble_buffer.payload;
}

void BLE_deinit() {
    rf_power_down();
}

static void BLE_crc(uint8_t* dst, const uint8_t* buf, uint8_t len) {
    // initialize 24-bit shift register in "wire bit order"
    // dst[0] = bits 23-16, dst[1] = bits 15-8, dst[2] = bits 7-0
    dst[0] = 0xAA;
    dst[1] = 0xAA;
    dst[2] = 0xAA;

    while (len--) {
        uint8_t d = *(buf++);
        for (uint8_t i = 1; i; i <<= 1, d >>= 1) {
            // save bit 23 (highest-value), left-shift the entire register by one
            uint8_t t = dst[0] & 0x01;         dst[0] >>= 1;
            if (dst[1] & 0x01) dst[0] |= 0x80; dst[1] >>= 1;
            if (dst[2] & 0x01) dst[1] |= 0x80; dst[2] >>= 1;

            // if the bit just shifted out (former bit 23) and the incoming data
            // bit are not equal (i.e. bit_out ^ bit_in == 1) => toggle tap bits
            if (t != (d & 1)) {
                // toggle register tap bits (=XOR with 1) according to CRC polynom
                dst[2] ^= 0xDA; // 0b11011010 inv. = 0b01011011 ^= x^6+x^4+x^3+x+1
                dst[1] ^= 0x60; // 0b01100000 inv. = 0b00000110 ^= x^10+x^9
            }
        }
    }
}

static void BLE_whiten(uint8_t channel, uint8_t* buf, uint8_t len) {
    // initialize LFSR with current channel, set bit 6
    uint8_t lfsr = channel | 0x40;
    while (len--) {
        uint8_t res = 0;
        // LFSR in "wire bit order"
        for (uint8_t i = 1; i; i <<= 1) {
            if (lfsr & 0x01) {
                lfsr ^= 0x88;
                res |= i;
            }
            lfsr >>= 1;
        }
        *(buf++) ^= res;
    }
}

static void BLE_swapbuf(uint8_t* buf, uint8_t len) {
    while (len--) {
        uint8_t b = *buf;
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        *(buf++) = b;
    }
}

void BLE_prepare(const uint8_t mac[6], const uint8_t payload_size) {
    if(payload_size < 1 || payload_size > 21) return;

    //init buffer
    ble_buffer.pdu_type = 0x42; // PDU type: ADV_NONCONN_IND, TX address is random
    ble_buffer.pl_size = 6 + payload_size; //add mac size to total size
    memcpy(ble_buffer.mac, mac, 6); //set MAC

    //calculate CRC over header+MAC+payload, append after payload
    BLE_crc(ble_buffer.payload + payload_size, (const uint8_t*)ble_buffer, 8 + payload_size);
}

void BLE_send(const uint8_t channel_idx) {
    const uint8_t data_size = 5 + ble_buffer.pl_size;
    if(data_size > 32) return;

    //copy data
    static uint8_t data_copy[32];
    memcpy(data_copy, (uint8_t*)ble_buffer, data_size);

    //whiten header+MAC+payload+CRC
    BLE_whiten(BLE_adv_channel[channel_idx], data_copy, data_size);
    //swap bit order
    BLE_swapbuf(data_copy, data_size);

    //send over radio
    rf_set_rf_channel(BLE_adv_frequency[channel_idx]);
    rf_write_tx_payload(data_copy, data_size, true);
    while(!rf_tx_fifo_is_empty());
}
