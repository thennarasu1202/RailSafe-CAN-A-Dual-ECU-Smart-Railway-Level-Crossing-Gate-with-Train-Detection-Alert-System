/*
 * RailSafe CAN - ECU2 (Gate and Signal Unit)
 * CAN receive -> servo gate + red/green LED + buzzer -> GATE_STATUS to ECU1
 * MCU : LPC2129, PCLK = 15 MHz
 */
#include<LPC21XX.H>
#include"delay.h"
#include"tick.h"
#include"servo.h"
#include"led_buzzer.h"
#include"can.h"

#define ST_CLEAR        0
#define ST_APPROACHING  1
#define ST_ON_CROSSING  2
#define ST_CLEARED      3

#define GATE_OPEN       0        // degrees
#define GATE_CLOSED     90
#define STEP_MS         30       // 1 degree per 30 ms -> gate moves in about 2.7 s
#define WARN_MS         3000     // warning before gate moves
#define FAILSAFE_MS     500      // no valid frame -> safe state
#define CAN_TX_MS       100

unsigned char train_state=ST_CLEAR;
unsigned char train_count=0;
unsigned char last_alive=0;
unsigned char alive=0;
unsigned char emerg=0;
unsigned char warn_done=0;
unsigned char prev_state=ST_CLEAR;
unsigned char gate_angle=GATE_OPEN;
unsigned char gate_target=GATE_OPEN;
unsigned char sig_red=0;         // 0 green, 1 red
unsigned char buzz=0;
unsigned int last_valid=0;
unsigned int emerg_t=0;
unsigned int warn_t=0;
unsigned int step_t=0;
unsigned int tx_t=0;

/* ---------------- CAN ---------------- */
void can_receive(void)
{
	CAN_MSG rx;
	unsigned char a;
	while(can2_rx(&rx))
	{
		if(rx.id==CAN_TRAIN_STATUS&&rx.dlc==4)
		{
			a=(rx.byteA>>24)&0xFF;
			if(a!=last_alive)                 // alive counter must change
			{
				last_alive=a;
				last_valid=millis();
				train_state=rx.byteA&0xFF;
				train_count=(rx.byteA>>8)&0xFF;
			}
		}
		else if(rx.id==CAN_EMERGENCY&&rx.dlc>=1)
		{
			if((rx.byteA&0xFF)==0xFF)
			{
				emerg=1;
				emerg_t=millis();
			}
		}
	}
}

void can_send(void)
{
	CAN_MSG tx;
	unsigned char g;
	if((millis()-tx_t)>=CAN_TX_MS)
	{
		tx_t=millis();
		if(gate_angle==gate_target)
			g=(gate_target==GATE_CLOSED)?2:0;     // closed / open
		else
			g=(gate_target>gate_angle)?1:3;       // closing / opening
		tx.id=CAN_GATE_STATUS;
		tx.rtr=0;
		tx.dlc=4;
		tx.byteA=(unsigned int)g|((unsigned int)sig_red<<8)|((unsigned int)buzz<<16)|((unsigned int)alive<<24);
		tx.byteB=0;
		alive++;
		can2_tx(tx);
	}
}

/* ---------------- gate / signals ---------------- */
void gate_step(void)
{
	if((millis()-step_t)>=STEP_MS)
	{
		step_t=millis();
		if(gate_angle<gate_target)
		{
			gate_angle++;
			servo_write(gate_angle);
		}
		else if(gate_angle>gate_target)
		{
			gate_angle--;
			servo_write(gate_angle);
		}
	}
}

void outputs_update(unsigned char flash)
{
	if(sig_red)
	{
		green_off();
		if(flash)
			red_on();
		else
			red_off();
	}
	else
	{
		red_off();
		green_on();
	}
	if(buzz)
		buzzer_on();
	else
		buzzer_off();
}

/* decide gate, signal and buzzer from received train state */
void control(void)
{
	unsigned char safe;
	unsigned char flash=1;                 // 1 = red lit (steady unless flashing)

	safe=((millis()-last_valid)>FAILSAFE_MS)||(emerg&&(millis()-emerg_t)<1000);

	if(safe)                               // fail-safe: gate closed, red on, buzzer on
	{
		gate_target=GATE_CLOSED;
		sig_red=1;
		buzz=1;
	}
	else if(train_state==ST_APPROACHING)
	{
		if(prev_state!=ST_APPROACHING)     // warning delay starts
		{
			warn_t=millis();
			warn_done=0;
		}
		if(!warn_done&&(millis()-warn_t)>=WARN_MS)
			warn_done=1;
		if(warn_done||gate_angle>=GATE_CLOSED)
			gate_target=GATE_CLOSED;
		sig_red=1;
		buzz=1;
		if(gate_angle<GATE_CLOSED)
			flash=(millis()/250)&1;        // flashing red until gate is down
	}
	else if(train_state==ST_ON_CROSSING||train_count>0)
	{
		gate_target=GATE_CLOSED;           // gate never opens while count > 0
		sig_red=1;
		buzz=1;
	}
	else                                   // CLEAR / CLEARED and count = 0
	{
		gate_target=GATE_OPEN;
		sig_red=0;                         // green on, buzzer off
		buzz=0;
	}
	prev_state=train_state;
	outputs_update(flash);
}

/* ---------------- main ---------------- */
int main()
{
	tick_init();
	signal_init();
	servo_init();
	servo_write(GATE_OPEN);
	can2_init(CAN_TRAIN_STATUS,CAN_EMERGENCY);

	last_valid=millis();
	tx_t=millis();

	while(1)
	{
		can_receive();
		control();
		gate_step();
		can_send();
		delay_ms(10);
	}
}
