#include "DELAY.h"

void delay(float delayInSec) {
	// 25 MHz => 25Mticks/s * delayInSec = ticks/delay
	uint32_t numTicks = (int32_t)(25000000 * delayInSec);
	LPC_TIM0->TCR = 0x02;
	// reset timer
	LPC_TIM0->PR = 0x00;
	// set prescaler to zero
	LPC_TIM0->MR0 = numTicks; // number of clock ticks to count
	LPC_TIM0->IR = 0x3F; // reset all interrupts
	LPC_TIM0->MCR = 0x04; // stop timer on match MR0
	LPC_TIM0->TCR = 0x01; // start timer
	while(LPC_TIM0->TCR & 0x01); // wait until delay time has elapsed
}

	