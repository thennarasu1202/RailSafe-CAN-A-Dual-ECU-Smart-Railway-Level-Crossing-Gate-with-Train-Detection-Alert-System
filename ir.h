#include<LPC21XX.H>

#define IR1 (1<<16)          // entry sensor P1.16
#define IR2 (1<<17)          // exit  sensor P1.17
#define IR_ACTIVE_LOW 1      // 1: sensor output is 0 when train detected
#define IR_DEBOUNCE 3        // 3 scans x 20ms = 60ms

typedef struct IR_DB
{
	unsigned char cnt;       // consecutive detections
	unsigned char latched;   // 1 = this train already counted
}IR_DB;

void ir_init(void)
{
	PINSEL2&=~(1<<3);        // P1.16-P1.25 as GPIO
	IODIR1&=~(IR1|IR2);      // inputs
}

unsigned char ir_active(unsigned int pin)
{
#if IR_ACTIVE_LOW
	return ((IOPIN1&pin)==0);
#else
	return ((IOPIN1&pin)!=0);
#endif
}

/* returns 1 only once per train, after debounce */
unsigned char ir_event(IR_DB *d,unsigned int pin)
{
	if(ir_active(pin))
	{
		if(d->cnt<IR_DEBOUNCE)
			d->cnt++;
		if(d->cnt==IR_DEBOUNCE&&d->latched==0)
		{
			d->latched=1;
			return 1;
		}
	}
	else
	{
		d->cnt=0;
		d->latched=0;
	}
	return 0;
}
