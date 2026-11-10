/* ECU2 : Gate Control (LPC2129 + MCP2551 + L293D + 16x2 LCD)
   L293D IN1 -> P0.12 , IN2 -> P0.13  (IN1 = close gate, IN2 = open gate)
   LCD  RS -> P0.5, E -> P0.6, D4-D7 -> P0.17-P0.20                      */
#include <LPC21xx.H>
#include "lcd.h"
#include "can.h"

#define IN1 (1<<12)
#define IN2 (1<<13)
#define GATE_TIME 2000            /* ms the motor runs to open/close the gate */

#define GATE_OPEN   0
#define GATE_CLOSED 1

static void gate_close(void)
{
    LCD_COMMAND(0x01);
    LCD_STRING("TRAIN COMING!");
    LCD_COMMAND(0xC0);
    LCD_STRING("GATE CLOSING...");
    IOSET0 = IN1;  IOCLR0 = IN2;
    delay_ms(GATE_TIME);
    IOCLR0 = IN1 | IN2;
    LCD_COMMAND(0x01);
    LCD_STRING("GATE CLOSED");
}

static void gate_open(void)
{
    LCD_COMMAND(0x01);
    LCD_STRING("TRAIN PASSED");
    LCD_COMMAND(0xC0);
    LCD_STRING("GATE OPENING...");
    IOSET0 = IN2;  IOCLR0 = IN1;
    delay_ms(GATE_TIME);
    IOCLR0 = IN1 | IN2;
    LCD_COMMAND(0x01);
    LCD_STRING("GATE OPEN");
}

int main(void)
{
    unsigned int  id;
    unsigned char cmd;
    unsigned char gate = GATE_OPEN;

    IODIR0 |= IN1 | IN2;
    IOCLR0  = IN1 | IN2;

    LCD_INIT();
    LCD_STRING("RAILSAFE CAN");
    LCD_COMMAND(0xC0);
    LCD_STRING("LEVEL CROSSING");
    delay_ms(1500);

    can_init();
    LCD_COMMAND(0x01);
    LCD_STRING("GATE OPEN");

    while(1)
    {
        if(can_rx(&id, &cmd) && id == ID_TRAIN_CMD)
        {
            if(cmd == CMD_CLOSE_GATE && gate == GATE_OPEN)
            {
                gate_close();
                gate = GATE_CLOSED;
            }
            else if(cmd == CMD_OPEN_GATE && gate == GATE_CLOSED)
            {
                gate_open();
                gate = GATE_OPEN;
            }
        }
    }
}
