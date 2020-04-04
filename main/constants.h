#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <stdint.h>

/*
2: OnBoard LED
4:
5: pwm on boot
6-11: integrated SPI flash
12: boot fail if pulled high
13:
14: pwm on boot
15: pwm on boot
16:
17:
18:
22:
23:
25: DAC1
26: DAC2
27:
32:
33:
34: input only, pull up/down yok
35: input only, pull up/down yok
36: input only, pull up/down yok
39: input only, pull up/down yok

*/

#define BT_CLASSIC_NAME "Punto"

// I2S
#define I2S_BCK 25
#define I2S_DIN 26
#define I2S_LCK 27

// CAN
#define CAN_RX 21
#define CAN_TX 22
#define CAN_SPEED CAN_TIMING_CONFIG_50KBITS()

#endif
