#include<LPC21XX.H>

#define RED   (1<<16)        // P1.16
#define GREEN (1<<17)        // P1.17
#define BUZZ  (1<<18)        // P1.18

void signal_init(void)
{
	PINSEL2&=~(1<<3);        // P1.16-P1.25 as GPIO
	IODIR1|=RED|GREEN|BUZZ;  // outputs
	IOCLR1=RED|BUZZ;
	IOSET1=GREEN;            // power-up: gate open, green on
}
void red_on(void)    { IOSET1=RED;   }
void red_off(void)   { IOCLR1=RED;   }
void green_on(void)  { IOSET1=GREEN; }
void green_off(void) { IOCLR1=GREEN; }
void buzzer_on(void) { IOSET1=BUZZ;  }
void buzzer_off(void){ IOCLR1=BUZZ;  }
