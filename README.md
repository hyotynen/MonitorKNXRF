# MonitorKNXRF

## General

This program is developed to be used on Raspberry Pi together with a CC1101 radiomodule.

The program will listen to interrupts generated from the CC1101. When an interrupt is recieved the data will be stored to a variable length buffer of class SensorKNXRF and saved into JSON file.

## Pin layout

| Raspberry pin # | Name | CC1101 |
| --: | --- | --- |
| 17 | 3.3V              | VCC |
| 18 | GPIO24            | GDO0 |
| 19 | GPIO10 (SPI_MOSI) | SI |
| 20 | Ground            | GND |
| 21 | GPIO09 (SPI_MISO) | SO |
| 22 | GPIO25            | GDO2 |
| 23 | GPIO11 (SPI_CLK)  | SCLK1 |
| 24 | GPI008 (SP_CE0_N) | CSn |

Note: GDO0 is not used in the program. which GDO for interrupts to use is specifed in the configuration of the chip, see cc1101.cpp (look for IOCFG[0..2] and the data sheet for the CC1101.

## Example

Compile the program by running make:
```
pi@hassbian:~/knxrf $ make
g++ -c -Wall -Iinclude/ monitorknxrf.cpp include/*.cpp
g++  -o monknxrf monitorknxrf.o cc1101.o sensorKNXRF.o -lwiringPi -lsystemd
```
