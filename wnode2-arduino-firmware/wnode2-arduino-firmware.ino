#include <SPI.h>
#include <RF24.h>
#include "BLE.h"
#include <avr/sleep.h>
#include <avr/wdt.h>

// (C)2019 Pawel A. Hernik
// (C)2020 github.com/AlexIII

/* PINOUT
  DHT11 pinout from left:
  VCC (DATA -> 8) NC GND

  nRF24L01 from pin side/top:
  -------------
  |1 3 5 7    |
  |2 4 6 8    |
  |           |
  |           |
  |           |
  |           |
  |           |
  -------------
  1 - GND  blk   GND
  2 - VCC  wht   3V3
  3 - CE   orng  A0
  4 - CSN  yell  10
  5 - SCK  grn   13
  6 - MOSI blue  11
  7 - MISO viol  12
  8 - IRQ  gray  2

 More info about nRF24L01:
 http://arduinoinfo.mywikis.net/wiki/Nrf24L01-2.4GHz-HowTo#po1
*/

/* Select board clock */
//#define F_CRYSTAL_8MHZ
#define F_CRYSTAL_16MHZ

#if F_CPU != 8000000
  #error "F_CPU is not 8 MHz! Select board with 8MHz clock even if you actually use 16MHz (in which case also uncomment #define F_CRYSTAL_16MHZ at the beginning of *.ino file)."
#endif


#define DEBUG_BAUD 115200

// def - DHT11
// ndef - internal ATMEGA temperature
//#define BEACON_DH11

#define RF24_CE_PIN A0
#define RF24_CSN_PIN 10
#define DHT11_PIN 8

RF24 radio(RF24_CE_PIN, RF24_CSN_PIN);
BTLE btle(&radio);

// -- Weather Node Data --
#define BLE_DEVICE_NAME "wNode2" //max 6 chars
#define UUID_TEMP2_HUM2 {0xA9, 0x53}

struct WeatherNodeData {
  enum battery_level_t {
      BATTERY_LEVEL_HIGH      = 0,
      BATTERY_LEVEL_MED_HIGH  = 1,
      BATTERY_LEVEL_MED_LOW   = 2,
      BATTERY_LEVEL_LOW       = 3
  };
  WeatherNodeData(const int16_t temperature, const int16_t humidity, const bool sensorFail, const battery_level_t batteryLevel) :
    temperature(hton(temperature)), humidity(hton(humidity)), flags({batteryLevel, sensorFail, 0}) {}
  WeatherNodeData(const int16_t temperature, const bool sensorFail, const battery_level_t batteryLevel) :
    temperature(hton(temperature)), humidity(hton(INT16_MIN)), flags({batteryLevel, sensorFail, 0}) {}
  
  uint8_t uuid[2] = UUID_TEMP2_HUM2;
  uint16_t humidity;
  uint16_t temperature;
  struct flags_t {
    battery_level_t batteryLevel    : 2;
    bool            sensorFail      : 1;
    uint8_t         reserve         : 5;
  } flags;
  uint8_t reserve = 0;

  static battery_level_t toBatteryLevel(const int volatage_mul_10) {
    if(volatage_mul_10 > 30) return BATTERY_LEVEL_HIGH;
    if(volatage_mul_10 > 28) return BATTERY_LEVEL_MED_HIGH;
    if(volatage_mul_10 > 26) return BATTERY_LEVEL_MED_LOW;
    return BATTERY_LEVEL_LOW;
  }

private:
  static uint16_t hton(const int16_t v) {
    const uint16_t v2 = abs(v) | (v < 0? 0x8000 : 0);
    return (v2<<8) | (v2>>8);
  }
};

// -------------------------

long readVcc() 
{
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(4); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // calc AVcc in mV
  return result;
}

// -------------------------
#include <avr/boot.h>
float readIntTemp() 
{
  static const int8_t TS_OFFSET = boot_signature_byte_get(4);
  static const uint8_t TS_GAIN = boot_signature_byte_get(3);
  
  // Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(4); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  return 25. + float(long( int(ADC) - (373 - TS_OFFSET) )*128) / TS_GAIN;
}

// -------------------------

#define DHT_OK         0
#define DHT_CHECKSUM  -1
#define DHT_TIMEOUT   -2
int temp1,temp10;
int humidity = -1;
float temperature;

int readDHT11(int pin)
{
  uint8_t bits[5];
  uint8_t bit = 7;
  uint8_t idx = 0;

  for (int i = 0; i < 5; i++) bits[i] = 0;

  // REQUEST SAMPLE
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(18);
  digitalWrite(pin, HIGH);
  delayMicroseconds(40);
  pinMode(pin, INPUT_PULLUP);

  // ACKNOWLEDGE or TIMEOUT
  unsigned int loopCnt = 10000;
  while(digitalRead(pin) == LOW) if(!loopCnt--) return DHT_TIMEOUT;

  loopCnt = 10000;
  while(digitalRead(pin) == HIGH) if(!loopCnt--) return DHT_TIMEOUT;

  // READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
  for (int i = 0; i < 40; i++) {
    loopCnt = 10000;
    while(digitalRead(pin) == LOW) if(!loopCnt--) return DHT_TIMEOUT;

    unsigned long t = micros();
    loopCnt = 10000;
    while(digitalRead(pin) == HIGH) if(!loopCnt--) return DHT_TIMEOUT;

    if(micros() - t > 40) bits[idx] |= (1 << bit);
    if(bit == 0) {
      bit = 7;    // restart at MSB
      idx++;      // next byte!
    }
    else bit--;
  }

  humidity = bits[0];
  temp1    = bits[2];
  temp10   = bits[3];
  temperature = abs(temp1+temp10/10.0);

  if(bits[4] != bits[0]+bits[1]+bits[2]+bits[3]) return DHT_CHECKSUM;
  return DHT_OK;
}

// -------------------------
enum wdt_time {
  SLEEP_15MS,
  SLEEP_30MS, 
  SLEEP_60MS,
  SLEEP_120MS,
  SLEEP_250MS,
  SLEEP_500MS,
  SLEEP_1S,
  SLEEP_2S,
  SLEEP_4S,
  SLEEP_8S,
  SLEEP_FOREVER
};

ISR(WDT_vect) { wdt_disable(); }

void powerDown(uint8_t time)
{
  //turn off serial
  Serial.flush();
  Serial.end();
  
  ADCSRA &= ~(1 << ADEN);  // turn off ADC
  if(time != SLEEP_FOREVER) { // use watchdog timer
    wdt_enable(time);
    WDTCSR |= (1 << WDIE);  
  }
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // most power saving
  cli();
  sleep_enable();
  sleep_bod_disable();
  sei();
  sleep_cpu();
  // ... sleeping here
  sleep_disable();
  ADCSRA |= (1 << ADEN); // turn on ADC

  //turn on serial
  Serial.begin(DEBUG_BAUD);
}
// -------------------------

uint8_t randByte() {
  static uint16_t c = 0xA7E2;
  c = (c << 1) | (c >> 15);
  c = (c << 1) | (c >> 15);
  c = (c << 1) | (c >> 15);
  c = analogRead(A2) ^ analogRead(A3) ^ analogRead(A4) ^ analogRead(A5) ^ analogRead(A6) ^ analogRead(A7) ^ c;
  return c;
}

void setup()
{
#ifdef F_CRYSTAL_16MHZ
  CLKPR = (1<<CLKPCE);
  CLKPR = (1<<CLKPS0); //clock prescale = 2 (lower CPU clock from 16 to 8 MHz)
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(DEBUG_BAUD);
  btle.begin(BLE_DEVICE_NAME);
  btle.setMAC(randByte(),randByte(),randByte(),randByte(),randByte(),randByte() | 0xC0);
}

// -------------------------
bool sensorFailFlag = false;
uint8_t bleReinitCnt = 0;
void loop() 
{
  //reinit radio periodically
  if(bleReinitCnt == 0) btle.begin(BLE_DEVICE_NAME);
  ++bleReinitCnt;
  
  digitalWrite(LED_BUILTIN, HIGH);

  //get readings
  long v = readVcc();
  Serial.print("Batt: "); Serial.println(v/1000.0);

#ifdef BEACON_DH11
  sensorFailFlag = readDHT11(DHT11_PIN) != DHT_OK;
  if(sensorFailFlag) Serial.println("DHT11 error!");
#else
  temperature = readIntTemp();
#endif
  Serial.print(F("Temp: ")); Serial.println(temperature);

  //prepare packet
  WeatherNodeData wnData(round(temperature*10), humidity < 0? INT16_MIN : humidity, sensorFailFlag, WeatherNodeData::toBatteryLevel(round(v/100.)));

  //send packet
  radio.powerUp();
  for(uint8_t i = 0; i < 3; ++i) {
    if(!btle.advertise(&wnData, sizeof(wnData))) Serial.println("Send fail!");
    btle.hopChannel();
  }
  radio.powerDown();

  //power down and sleep
  digitalWrite(LED_BUILTIN, LOW);
  powerDown(SLEEP_2S);
}
