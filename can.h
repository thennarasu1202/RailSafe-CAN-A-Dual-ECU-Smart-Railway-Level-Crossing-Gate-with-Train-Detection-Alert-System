/* CAN1 driver for LPC2129 (PCLK = 15 MHz, 125 kbps, 11-bit ID)
   Transceiver: MCP2551   TD1 -> P0.24 , RD1 -> P0.23
   (verify against your LPC2129 datasheet / board schematic)      */

#define ID_TRAIN_CMD   0x100      /* ECU1 -> ECU2 */
#define CMD_CLOSE_GATE 0x01
#define CMD_OPEN_GATE  0x02

void can_init(void)
{
    PINSEL1 |= 0x00014000;        /* P0.23 = RD1, P0.24 = TD1 */
    C1MOD  = 0x01;                /* reset mode */
    AFMR   = 0x02;                /* acceptance filter bypass: accept all */
    C1BTR  = 0x0014000E;          /* 125 kbps: BRP=15, TSEG1=5, TSEG2=2 */
    C1MOD  = 0x00;                /* normal mode */
}

void can_tx(unsigned int id, unsigned char data)
{
    while((C1SR & (1<<2)) == 0);  /* wait TX buffer 1 free (TBS1) */
    C1TFI1 = (1<<16);             /* DLC = 1, standard frame */
    C1TID1 = id;
    C1TDA1 = data;
    C1CMR  = 0x21;                /* TR + select TX buffer 1 */
    while((C1SR & (1<<3)) == 0);  /* wait transmission complete (TCS1) */
}

/* returns 1 if a frame was received */
unsigned char can_rx(unsigned int *id, unsigned char *data)
{
    if(C1GSR & 0x01)              /* RBS: receive buffer full */
    {
        *id   = C1RID;
        *data = (unsigned char)(C1RDA & 0xFF);
        C1CMR = 0x04;             /* release receive buffer */
        return 1;
    }
    return 0;
}
