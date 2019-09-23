#ifndef DELAY_H_
#define DELAY_H_

#include <LPC17xx.h>

#define MS 1.e-3 // factor to convert millisec to sec: 15 * MS = 0.015
#define US 1.e-6 // factor to convert microsec to sec: 15 * US = 0.000015

void delay(float delayInSec);

#endif /* DELAY_H_ */
