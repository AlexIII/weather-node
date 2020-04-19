/*
 * Weather Node v1.0
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

/*
    WARNING!
    VERY CRAPPY IMPLEMENTATION!
    (In the respect to the protocol timings.)
    ---
    dht22_wait_level() was supposed to return microseconds, but it's like 10 times slower,
    so it returns ~tens of microseconds instead.
    By some miracle this thing works now, but don't rely on it.
*/

#include "dht22.h"
#include "delay.h"

#define DHT22_DATA_PIN_DOWN gpio_pin_configure(DHT22_DATA_PIN, GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT)
#define DHT22_DATA_PIN_RELEASE gpio_pin_configure(DHT22_DATA_PIN, 0)
#define DHT22_DATA_PIN_READ() gpio_pin_val_read(DHT22_DATA_PIN)

static uint8_t dht22_wait_level(const bool lvl) {
    uint8_t time = 0;
    while(DHT22_DATA_PIN_READ() != lvl) {
        if(time > 200) return 0xFF;
        time += 1;
    }
    return time;
}

static uint8_t dht22_read_bit() {
    if(dht22_wait_level(1) > 70) return 0xFF; //low 50us
    const uint8_t v = dht22_wait_level(0); //"0" - 28us, "1" - 70us
    if(v > 110) return 0xFF;
    return v > 2? 1 : 0;
}

static int16_t dht22_read_byte() {
    uint8_t val = 0;
    for(uint8_t i = 0; i < 8; ++i) {
        const uint8_t bit = dht22_read_bit();
        if(bit > 1) return -1;

        val <<= 1;
        val |= bit;
    }
    return val;
}

static bool dht22_read_bytes(uint8_t* data, uint8_t len) {
    while(len--) {
        const int16_t byte = dht22_read_byte();
        if(byte < 0) return false;
        *data++ = byte;
    }
    return true;
}

bool dht22_read(dht22_data_t* dht22_data) {
    bool ok = false;
    gpio_pin_configure(DHT22_DATA_PIN, 0);
    dht22_data->humidity[0] = 0;
    dht22_data->humidity[1] = 0;
    dht22_data->temperature[0] = 0;
    dht22_data->temperature[1] = 0;
    dht22_data->crc = 1;

    //master pull
    DHT22_DATA_PIN_DOWN;
    delay_us(1200);
    DHT22_DATA_PIN_RELEASE;
    delay_us(1);

    do {
        //device pull
        if(dht22_wait_level(0) > 200) { //20-40us
            //printf_fast("read fail 0\r\n");
            break;
        }
        if(dht22_wait_level(1) > 120) { //80us
            //printf_fast("read fail 1\r\n");
            break;
        }

        //start of first bit
        if(dht22_wait_level(0) > 120) { //80us
            //printf_fast("read fail 2\r\n");
            break;
        }

        //read data
        uint8_t* const data = (uint8_t*)dht22_data;
        if(!dht22_read_bytes(data, 5)) {
            //printf_fast("read fail 3\r\n");
            break;
        }

        //check crc
        if((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) {
            //printf_fast("read fail crc\r\n");
            break;
        }

        ok = true;

    } while(0);

    return ok;
}
