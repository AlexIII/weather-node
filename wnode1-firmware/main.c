/*
 * Weather Node v1.0
 * author: github.com/AlexIII
 * e-mail: endoftheworld@bk.ru
 * license: MIT
 */

#include <string.h>
#include <stdio.h>
#include "gpio.h"
#include "rf.h"
#include "ble.h"
#include "delay.h"
#include "uart.h"
#include "rtc2.h"
#include "pwr_clk_mgmt.h"
#include "dht22.h"
#include "rng.h"

/* LOGIC */
#define POLL_SENSOR_EVERY_N_WAKEUPS 60 //once in 2 min
#define SENSOR_FAIL_READ_THRESHOLD 10

/* WIREING */
#define LED_PIN GPIO_PIN_ID_P0_0
#define DHT22_PWR_PIN GPIO_PIN_ID_P1_5

/* --- BLE device-specific --- */
#define BLE_DEVICE_NAME "wNode1" //max 6 chars
#define BLE_DEVICE_NAME_CHARS() (sizeof(BLE_DEVICE_NAME) - 1)
#define UUID_TEMP2_HUM2 {0xA9, 0x53}

typedef enum {
    BATTERY_LEVEL_HIGH      = 0,
    BATTERY_LEVEL_MED_HIGH  = 1,
    BATTERY_LEVEL_MED_LOW   = 2,
    BATTERY_LEVEL_LOW       = 3
} battery_level_t;

//gonna be replaced with random mac
static uint8_t ble_mac[6] = {0xCB, 0x71, 0x1D, 0xBB, 0xA5, 0x6A};

//max 8 bytes
typedef struct {
    uint8_t uuid[2];
    uint8_t humidity[2];
    uint8_t temperature[2];
    struct flags_t {
        battery_level_t battery_level   : 2;
        bool            sensor_fail     : 1;
        uint8_t         reserve         : 5;
    } flags;
    uint8_t reserve;
} manuf_data_t;

//returns length
static uint8_t BLE_set_manuf_data(uint8_t* payload_start, const manuf_data_t* manuf_data) {
    uint8_t* payload = payload_start;
    //flags chunk
    *payload++ = 2;
    *payload++ = 0x01;
    *payload++ = 0x05;

    //name chunk
    *payload++ = BLE_DEVICE_NAME_CHARS() + 1;
    *payload++ = 0x09;
    memcpy(payload, BLE_DEVICE_NAME, BLE_DEVICE_NAME_CHARS());
    payload += BLE_DEVICE_NAME_CHARS();

    //Manufacturer data chunk
    *payload++ = 1 + sizeof(manuf_data_t);
    *payload++ = 0xFF; //Manufacturer Specific Data
    memcpy(payload, manuf_data, sizeof(manuf_data_t));
    payload += sizeof(manuf_data_t);

    return payload - payload_start;
}

static uint8_t next_adv_channel_idx() {
    static uint8_t idx = 2;
    if(++idx > 3) idx = 0;
    return idx;
}

static void BLE_send_manuf_data(const manuf_data_t* manuf_data, uint8_t trys) {
    uint8_t* payload = BLE_init();

    //check if the same data
    static uint8_t prvData[sizeof(manuf_data_t)];
    if(memcmp(prvData, manuf_data, sizeof(manuf_data_t)) != 0) {
        const uint8_t payload_size = BLE_set_manuf_data(payload, manuf_data);
        BLE_prepare(ble_mac, payload_size);
        memcpy(prvData, manuf_data, sizeof(manuf_data_t));
    }

    while(trys--) {
        BLE_send(next_adv_channel_idx());
    }
    BLE_deinit();
}

/* --- MAC generation --- */

static void genRandomMac(uint8_t mac[6]) {
    rng_configure(RNG_CONFIG_OPTION_RUN | RNG_CONFIG_CORRECTOR_ENABLE);
    for(uint8_t i = 0; i < 6; ++i)
        mac[i] = rng_get_next_byte();
    rng_configure(RNG_CONFIG_OPTION_STOP);
    mac[0] |= 0xC0; //static random address should have two topmost bits set
}

/*
//Save generated MAC in flash

#define MAGIC_VAL 0xA1  //magic value that indicates that MAC has been generated earlier
#define NV_EXT_P0_MAGIC_VAL_OFFSET 0
#define NV_EXT_P0_MAC_OFFSET 1
#define NV_EXT_P0_DATA_END 7
static void initMac() {
    uint8_t buffer[NV_EXT_P0_DATA_END];
    memory_flash_read_bytes(MEMORY_FLASH_NV_EXT_END_START_ADDRESS, NV_EXT_P0_DATA_END, buffer);
    if(buffer[NV_EXT_P0_MAGIC_VAL_OFFSET] != MAGIC_VAL) {
        //generate and save MAC
        genRandomMac(buffer + NV_EXT_P0_MAC_OFFSET);
        buffer[NV_EXT_P0_MAGIC_VAL_OFFSET] = MAGIC_VAL;
        memory_flash_erase_page(MEMORY_FLASH_NV_EXT_END_FIRST_PAGE_NUM);
        memory_flash_write_bytes(MEMORY_FLASH_NV_EXT_END_START_ADDRESS, NV_EXT_P0_DATA_END, buffer);
    }
    memcpy(ble_mac, buffer + NV_EXT_P0_MAC_OFFSET, 6);
}
*/

static void initMac() {
    genRandomMac(ble_mac);
}

/* --- MCU on-chip functions --- */

static void initSysTick() {
    //cloack init
    pwr_clk_mgmt_cclk_configure(
        PWR_CLK_MGMT_CCLK_CONFIG_OPTION_CLK_FREQ_16_MHZ |
        PWR_CLK_MGMT_CCLK_CONFIG_OPTION_WKUP_INT_ON_XOSC16M_DISABLE |
        PWR_CLK_MGMT_CCLK_CONFIG_OPTION_START_XOSC16M_AND_RCOSC16M |
        PWR_CLK_MGMT_CCLK_CONFIG_OPTION_CLK_SRC_XOSC16M_OR_RCOSC16M |
        PWR_CLK_MGMT_CCLK_CONFIG_OPTION_XOSC16M_IN_REGISTER_RET_OFF
    );
    pwr_clk_mgmt_clklf_configure(PWR_CLK_MGMT_CLKLF_CONFIG_OPTION_CLK_SRC_RCOSC32K);
    pwr_clk_mgmt_wait_until_clklf_is_ready();

    //wakeup init
    pwr_clk_mgmt_wakeup_sources_configure(PWR_CLK_MGMT_WAKEUP_CONFIG_OPTION_WAKEUP_ON_RTC2_TICK_ALWAYS);

    //run RTC2
    rtc2_configure(
        RTC2_CONFIG_OPTION_ENABLE | RTC2_CONFIG_OPTION_COMPARE_MODE_0_RESET_AT_IRQ,
        0xFFFF //0.5Hz
    );
}

static void sleep(const uint16_t ticks) {
    rtc2_set_compare_val(ticks);
    pwr_clk_mgmt_enter_pwr_mode_register_ret();
    pwr_clk_mgmt_wait_until_cclk_src_is_xosc16m();
}

static battery_level_t get_battery_level() {
    static battery_level_t cur = BATTERY_LEVEL_HIGH;

    //switch to lower level
    if(pwr_clk_mgmt_is_vdd_below_bor_threshold()) {
        if(cur == BATTERY_LEVEL_HIGH) cur = BATTERY_LEVEL_MED_HIGH;
        else if(cur == BATTERY_LEVEL_MED_HIGH) cur = BATTERY_LEVEL_MED_LOW;
        else if(cur == BATTERY_LEVEL_MED_LOW) cur = BATTERY_LEVEL_LOW;
    }
    //configure for new level
    pwr_clk_mgmt_pwr_failure_configure((
            cur == BATTERY_LEVEL_HIGH? PWR_CLK_MGMT_PWR_FAILURE_CONFIG_OPTION_POF_THRESHOLD_2_7V :
            cur == BATTERY_LEVEL_MED_HIGH? PWR_CLK_MGMT_PWR_FAILURE_CONFIG_OPTION_POF_THRESHOLD_2_5V :
            PWR_CLK_MGMT_PWR_FAILURE_CONFIG_OPTION_POF_THRESHOLD_2_3V
        ) | PWR_CLK_MGMT_PWR_FAILURE_CONFIG_OPTION_POF_ENABLE
    );

    return cur;
}

/* --- Sensor functions --- */

#define dht22_power_on() gpio_pin_configure(DHT22_PWR_PIN,                  \
        GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT |                                 \
        GPIO_PIN_CONFIG_OPTION_OUTPUT_VAL_SET |                             \
        GPIO_PIN_CONFIG_OPTION_PIN_MODE_OUTPUT_BUFFER_HIGH_DRIVE_STRENGTH)
#define dht22_power_off() gpio_pin_configure(DHT22_PWR_PIN, 0)

static bool updateSensorData(manuf_data_t* device_data) {
    dht22_data_t dht22_data;
    dht22_power_on();
    sleep(0x8000);
    const bool ok = dht22_read(&dht22_data);

    if(ok) {
        device_data->humidity[0] = dht22_data.humidity[0];
        device_data->humidity[1] = dht22_data.humidity[1];
        device_data->temperature[0] = dht22_data.temperature[0];
        device_data->temperature[1] = dht22_data.temperature[1];
/*
        uint8_t* const data = (uint8_t*)dht22_data;
        uint8_t tmp = data[0];
        data[0] = data[1];
        data[1] = tmp;
        tmp = data[2];
        data[2] = data[3];
        data[3] = tmp;
        printf_fast("read: %d.%d%% %d.%d*C\r\n", (*(int16_t*)dht22_data.temperature)/10, (*(int16_t*)dht22_data.temperature)%10, (*(int16_t*)dht22_data.humidity)/10, (*(int16_t*)dht22_data.humidity)%10);
*/
    } else {
        //printf_fast("read fail\r\n");
    }

    dht22_power_off();
    return ok;
}

/* --- Main --- */

void main(void) {
    initSysTick();
    initMac();

    //led
    gpio_pin_configure(LED_PIN,
        GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT |
        GPIO_PIN_CONFIG_OPTION_OUTPUT_VAL_CLEAR |
        GPIO_PIN_CONFIG_OPTION_PIN_MODE_OUTPUT_BUFFER_HIGH_DRIVE_STRENGTH);

    //uart
    gpio_pin_configure(GPIO_PIN_ID_FUNC_TXD,
        GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT |
        GPIO_PIN_CONFIG_OPTION_OUTPUT_VAL_SET |
        GPIO_PIN_CONFIG_OPTION_PIN_MODE_OUTPUT_BUFFER_HIGH_DRIVE_STRENGTH);
    uart_configure_8_n_1_38400();
    printf_fast("started\r\n");

    manuf_data_t device_data = {
        UUID_TEMP2_HUM2,
        {27, 0}, //temp
        {73, 0}, //hum
        {0, 0, 0}, //flags
        0
    };

    uint8_t wakeups = POLL_SENSOR_EVERY_N_WAKEUPS;
    uint8_t sensorErrors = 0;
    while(1) {
        ++wakeups;
        if(wakeups >= POLL_SENSOR_EVERY_N_WAKEUPS) {
            if(!updateSensorData(&device_data)) {
                if(sensorErrors < 255) ++sensorErrors;
            } else sensorErrors = 0;
            //update sensor fail flag
            device_data.flags.sensor_fail = sensorErrors > SENSOR_FAIL_READ_THRESHOLD;
            //update battery level
            device_data.flags.battery_level = get_battery_level();
            wakeups = 0;
        }

        gpio_pin_val_set(LED_PIN);
        BLE_send_manuf_data(&device_data, 3);

        gpio_pin_val_clear(LED_PIN);
        sleep(0xFFFF);
        //gpio_pin_val_set(LED_PIN);
    }
}
