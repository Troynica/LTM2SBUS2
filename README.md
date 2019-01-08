# LTM2SBUS2
Firmware for a UAV telemetry to Futaba's S.BUS2 interface

This firmware is an Arduino sketch that takes LTM messages from a flight controller (running iNav of similar software) on its serial port and pushes interpreted data to an I2C-to-S.BUS2 interface developped by Thomas Hedegaard Jørgensen. The LTM protocol interpreter was written by Paweł Spychalski (DzikuVx) (https://github.com/DzikuVx/ltm_telemetry_reader).
I only added a tiny bit of code to combine these products.

This Arduino sketch depends on the Wire.h library for i2c communication. It was tested on ATmega328p and ATmega168p chips but should run on any chip (with at least a UART and an I2C port) that you can compile the sketch for.

LTM: https://github.com/KipK/Ghettostation/blob/master/GhettoStation/LightTelemetry.cpp

I2C-to-S.BUS2 interface: https://shop.tje.dk/catalog/product_info.php?cPath=22&products_id=42

LTM reader: https://github.com/DzikuVx/ltm_telemetry_reader
