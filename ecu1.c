/*
 * RailSafe CAN - ECU1 (Train Detection and Dashboard Unit)
 * IR1 (entry) + IR2 (exit) -> train count/state -> LCD dashboard -> CAN to ECU2
 * MCU : LPC2129, PCLK = 15 MHz
 */
#include<LPC21XX.H>
#include"lcd.h"          // lcd.h also includes delay.h
#include"tick.h"
#include"ir.h"
#include"can.h"

#define ST_CLEAR        0
#define ST_APPROACHING  1
#define ST_ON_CROSSING  2
#define ST_CLEARED      3

#define SCAN_MS         20
#define CAN_TX_MS       100
#define CAN_TIMEOUT_MS  500
#define CLEAR_DELAY_MS  2000     // safety delay after last train leaves

unsigned char train_count=0;
unsigned char train_state=ST_CLEAR;
unsigned char gate_state=0;      // received from ECU2
unsigned char link=0;            // 1 = ECU2 alive
unsigned char gate_rx_ever=0;
unsigned char last_galive=0;
unsigned char clear_wait=0;
unsigned char alive=0;
unsigned int last_gate_rx=0;
unsigned int clear_t=0;
unsigned int cleared_t=0;
unsigned int tx_t=0;

IR_DB db1={0,0};
IR_DB db2={0,0};

/* last value drawn on LCD (0xFF = not drawn yet) */
unsigned char d_s1=0xFF,d_s2=0xFF,d_link=0xFF,d_cnt=0xFF,d_state=0xFF,d_gate=0xFF;

/* ---------------- CAN ---------------- */
void can_receive(void)
{
	CAN_MSG rx;
	unsigned char a;
	while(can2_rx(&rx))
	{
		if(rx.id==CAN_GATE_STATUS&&rx.dlc==4)
		{
			gate_state=rx.byteA&0xFF;
			a=(rx.byteA>>24)&0xFF;
			if(a!=last_galive||gate_rx_ever==0)    // alive counter must change
			{
				last_galive=a;
				last_gate_rx=millis();
				gate_rx_ever=1;
			}
		}
	}
}

void can_send(void)
{
	CAN_MSG tx;
	if((millis()-tx_t)>=CAN_TX_MS)
	{
		tx_t=millis();
		tx.id=CAN_TRAIN_STATUS;
		tx.rtr=0;
		tx.dlc=4;
		tx.byteA=(unsigned int)train_state|((unsigned int)train_count<<8)|((unsigned int)0<<16)|((unsigned int)alive<<24);
		tx.byteB=0;
		alive++;
		can2_tx(tx);
	}
}

void link_update(void)
{
	link=(gate_rx_ever&&((millis()-last_gate_rx)<CAN_TIMEOUT_MS))?1:0;
}

/* ---------------- train logic ---------------- */
void train_update(void)
{
	unsigned int e;
	if(ir_event(&db1,IR1))                    // train entering
	{
		if(train_count<9)
			train_count++;
		clear_wait=0;                         // cancel pending CLEARED
		if(train_state==ST_CLEAR||train_state==ST_CLEARED)
			train_state=ST_APPROACHING;
	}
	if(ir_event(&db2,IR2))                    // train leaving
	{
		if(train_count>0)
		{
			train_count--;
			if(train_count==0)
			{
				clear_wait=1;                 // start 2 s safety delay
				clear_t=millis();
			}
		}
	}
	if(clear_wait&&(millis()-clear_t)>=CLEAR_DELAY_MS)
	{
		clear_wait=0;
		train_state=ST_CLEARED;
		cleared_t=millis();
	}
	if(train_state==ST_APPROACHING&&train_count>0&&link&&gate_state==2)
		train_state=ST_ON_CROSSING;           // gate is down, train on crossing
	if(train_state==ST_CLEARED)
	{
		e=millis()-cleared_t;
		if((link&&gate_state==0&&e>=500)||e>=5000)
			train_state=ST_CLEAR;
	}
}

/* ---------------- LCD dashboard ----------------
   Row1 : I1:xxx I2:xxx  OK
   Row2 : T:n  state  gate
   only ONE field is redrawn per call, so IR scanning is not blocked long */
char *state_text(unsigned char s)
{
	switch(s)
	{
		case ST_APPROACHING: return "APPROACH";
		case ST_ON_CROSSING: return "ON CROSS";
		case ST_CLEARED:     return "CLEARED ";
		default:             return "CLEAR   ";
	}
}

char *gate_text(unsigned char g)
{
	switch(g)
	{
		case 0:  return "OPEN";
		case 1:  return "CLSG";
		case 2:  return "CLSD";
		case 3:  return "OPNG";
		default: return "LOST";
	}
}

void dashboard(unsigned char s1,unsigned char s2)
{
	unsigned char g=link?gate_state:4;
	if(s1!=d_s1)
	{
		d_s1=s1;
		LCD_COMMAND(0x83);
		LCD_STRING(s1?"ON ":"OFF");
		return;
	}
	if(s2!=d_s2)
	{
		d_s2=s2;
		LCD_COMMAND(0x8A);
		LCD_STRING(s2?"ON ":"OFF");
		return;
	}
	if(link!=d_link)
	{
		d_link=link;
		LCD_COMMAND(0x8E);
		LCD_STRING(link?"OK":"NO");
		return;
	}
	if(train_count!=d_cnt)
	{
		d_cnt=train_count;
		LCD_COMMAND(0xC2);
		if(train_count==0)
			LCD_DATA('0');
		else
			LCD_INT(train_count);
		return;
	}
	if(train_state!=d_state)
	{
		d_state=train_state;
		LCD_COMMAND(0xC4);
		LCD_STRING(state_text(train_state));
		return;
	}
	if(g!=d_gate)
	{
		d_gate=g;
		LCD_COMMAND(0xCC);
		LCD_STRING(gate_text(g));
		return;
	}
}

/* ---------------- main ---------------- */
int main()
{
	unsigned char s1,s2;

	tick_init();
	ir_init();
	delay_ms(20);                   // LCD power-up time
	LCD_INIT();
	can2_init(CAN_GATE_STATUS,CAN_EMERGENCY);

	LCD_COMMAND(0x80);
	LCD_STRING("RAILSAFE CAN");
	LCD_COMMAND(0xC0);
	LCD_STRING("ECU1 STARTING");
	delay_ms(1000);
	LCD_COMMAND(0x01);

	LCD_COMMAND(0x80);              // fixed labels
	LCD_STRING("I1:");
	LCD_COMMAND(0x87);
	LCD_STRING("I2:");
	LCD_COMMAND(0xC0);
	LCD_STRING("T:");

	tx_t=millis();

	while(1)
	{
		train_update();
		can_receive();
		link_update();
		can_send();

		s1=ir_active(IR1);          // live sensor status
		s2=ir_active(IR2);
		dashboard(s1,s2);

		delay_ms(SCAN_MS);
	}
}
