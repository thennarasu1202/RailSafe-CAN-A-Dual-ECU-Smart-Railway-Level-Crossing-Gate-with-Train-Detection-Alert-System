/* ECU1 : Train Detection  (LPC2129 + MCP2551)
   IR1 (approach side) -> P0.16
   IR2 (exit side)     -> P0.17
   Sensor output is LOW when train is detected.
   Sends CAN frame ID 0x100 : 0x01 = CLOSE GATE, 0x02 = OPEN GATE        */
#include <LPC21xx.H>
#include "delay.h"
#include "can.h"

#define IR1 (1<<16)
#define IR2 (1<<17)

#define IDLE           0
#define TRAIN_ON_TRACK 1

int main(void)
{
    unsigned char state = IDLE;

    IODIR0 &= ~(IR1 | IR2);       /* inputs */
    can_init();

    while(1)
    {
        if(state == IDLE)
        {
            if((IOPIN0 & IR1) == 0)           /* train reached IR1 */
            {
                delay_ms(50);                 /* debounce */
                if((IOPIN0 & IR1) == 0)
                {
                    can_tx(ID_TRAIN_CMD, CMD_CLOSE_GATE);
                    state = TRAIN_ON_TRACK;
                    while((IOPIN0 & IR1) == 0);   /* wait till IR1 clears */
                }
            }
        }
        else /* TRAIN_ON_TRACK */
        {
            if((IOPIN0 & IR2) == 0)           /* train reached IR2 (exit) */
            {
                delay_ms(50);
                if((IOPIN0 & IR2) == 0)
                {
                    while((IOPIN0 & IR2) == 0);   /* wait till train fully passes */
                    delay_ms(500);
                    can_tx(ID_TRAIN_CMD, CMD_OPEN_GATE);
                    state = IDLE;
                }
            }
        }
    }
}
