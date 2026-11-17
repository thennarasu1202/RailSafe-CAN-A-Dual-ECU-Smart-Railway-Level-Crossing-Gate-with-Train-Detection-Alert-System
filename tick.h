#include<LPC21XX.H>

/* Timer1 runs freely, 1 count = 1 ms (PCLK = 15 MHz).
   Timer0 is used only by delay_ms(), so they never disturb each other. */
void tick_init(void)
{
	T1PR=15000-1;
	T1TCR=0x01;
}
unsigned int millis(void)
{
	return T1TC;
}
