# Weather Node

<img style="border-radius: 15px;" width="170" align="right" src="img/wns-icon.png" />

**Weather Node** is a small battery-powered temperature and humidity sensor that transmits measurements via Bluetooth LE to your phone.

The battery is expected to last for 3 - 6 months (depending on battery and ambient temperature).

You can easily assemble your Weather Node as it requires very few components:
- MCU board with nRF24LE1 microcontroller;
- DHT22 (or DTH11) temperature/humidity sensor;
- battery socket for CR2032;
- two resistors (10k), two capacitors (0.1uF and 4.7uF), and an LED.

After assembling, you could get something like this: 

<img width="600" align="center" src="img/wnode1-photo.jpg" />

Schematics:

<img width="400" align="center" src="img/wn1-schematic.png" />

In order to upload firmware to the nRF24LE1 board you may also need FT232RL usb-uart convertor or an Arduino. Check out these links for more info on nRF24LE1 flashing options.

- [Program nRF24LE1 with FT232R](https://github.com/jdelfes/nrf24le1_flasher)
- [Program nRF24LE1 with Arduino](https://github.com/DeanCording/nRF24LE1_Programmer)

## Weather Node Station
<img width="300" align="right" src="img/wns-screen.png" />

**Weather Node Station** is an app for your phone that receives data from Weather Nodes.

Only Android version is available now (`.apk`). After installation you will need to give the app permissions to access Bluetooth (on modern Android versions it also requires Location permissions as well). 

The app is developed with react-native framework, so it's possible to compile it for IOS (I'm sure it'll require some fixes though). You're very welcome to try and do that, especially if you have the necessary environment installed.

## Project Files

- wnode1-firmware/ - firmware for Weather Node MCU (nRF24LE1). Project for [Code::Blocks](http://www.codeblocks.org/) with [SDCC](http://sdcc.sourceforge.net/)
- wnodestation/ - [React Native](http://reactnative.dev) app for phone

## Known Issues

1. DHT22 turned out to be a crappy sensor. It showed very bad stability under freezing temperatures and low voltage power supply. It will be replaced with superior AHT10 the moment I'll get my hands on it.
<img width="300" src="img/mad-DHT22.png" />
Funny readings from DHT22 after 12 hours in my fridge (~ +5°C).

## BLE Protocol Details

Weather Node emulates BLE advertisement packages. All meaningful data is packed in "Manufacturer Data" section. Layout of the section:

| Field       | Offset (bytes) | Size (bytes) | Value                                                        |
| ----------- | -------------- | ------------ | ------------------------------------------------------------ |
| UUID        | 0              | 2            | UUID[0] == 0xA9, UUID[1] ==0x53                              |
| temperature | 2              | 2            | Temperature in DHT22 format                                  |
| humidity    | 4              | 2            | Humidity in DHT22 format                                     |
| flags       | 6              | 1            | Bits 0, 1: battery level (0 - HIGH, 1 - MED_HIGH, 2 - MED_LOW, 3 - LOW) <br />Bit 2: sensor failure flag |

## License

All files in this repo, except `wnode1-firmware/nRF24LE1_SDK`  go by MIT License © github.com/AlexIII

**Note**, `wnode1-firmware/nRF24LE1_SDK` is a separate project. It's licensed under LGPL.

Due to SDK licensing, precompiled firmware (`*.hex`) for nRF24LE1 is under LGPL.

