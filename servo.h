#include<LPC21XX.H>

/* Servo on PWM2 = P0.7  (project description)
   PWMPR=14 -> 1 count = 1 us at PCLK 15 MHz
   PWMMR0 = period (20 ms), PWMMR2 = duty (pulse width)
   0 deg = 1000 us , 90 deg = 1500 us  (change SERVO_MIN_US if your servo differs) */
#define SERVO_PERIOD_US 20000
#define SERVO_MIN_US    1000

void servo_write(unsigned int angle)
{
	PWMMR2=SERVO_MIN_US+(angle*1000)/180;   // Duty Cycle Register
	PWMLER=0x04;                            // latch new MR2 value
}

void servo_init(void)
{
	PINSEL0|=0x00008000;     // P0.7 as PWM2
	PWMPR=15-1;              // 1 us tick
	PWMMR0=SERVO_PERIOD_US;  // Period Register
	PWMMR2=SERVO_MIN_US;     // Duty Cycle Register (0 deg)
	PWMMCR=0x00000002;       // PWMTC=0 if PWMMR0 = PWMTC
	PWMPCR=0x00000400;       // PWM2 output enabled
	PWMTCR=0x00000009;       // PWM & Counter Enable
}
