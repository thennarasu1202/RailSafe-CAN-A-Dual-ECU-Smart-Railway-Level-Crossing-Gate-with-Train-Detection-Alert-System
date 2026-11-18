#include<LPC21XX.H>

/* Servo on PWM5 = P0.21  (class notes: PINSEL1 = 0x400)
   PWMPR=14 -> 1 count = 1 us at PCLK 15 MHz
   PWMMR0 = period (20 ms), PWMMR5 = duty (pulse width)
   0 deg = 1000 us , 90 deg = 1500 us  (change SERVO_MIN_US if your servo differs) */
#define SERVO_PERIOD_US 20000
#define SERVO_MIN_US    1000

void servo_write(unsigned int angle)
{
	PWMMR5=SERVO_MIN_US+(angle*1000)/180;   // Duty Cycle Register
	PWMLER=0x20;                            // latch new MR5 value
}

void servo_init(void)
{
	PINSEL1|=0x00000400;     // P0.21 as PWM5
	PWMPR=15-1;              // 1 us tick
	PWMMR0=SERVO_PERIOD_US;  // Period Register
	PWMMR5=SERVO_MIN_US;     // Duty Cycle Register (0 deg)
	PWMMCR=0x00000002;       // PWMTC=0 if PWMMR0 = PWMTC
	PWMPCR=0x00002000;       // PWM5 output enabled
	PWMTCR=0x00000009;       // PWM & Counter Enable
}
