#include<LPC21XX.H>

#define CAN_TRAIN_STATUS 0x100   // ECU1 -> ECU2
#define CAN_GATE_STATUS  0x200   // ECU2 -> ECU1
#define CAN_EMERGENCY    0x7FF   // either

/* CAN1: P0.24 -> TD1, P0.25 -> RD1  (project description)
   CONFIRM this PINSEL1 value in the LPC2129 datasheet before use */
#define CAN_PINSEL 0x00050000

typedef struct CAN_MSG
{
	unsigned int id;
	unsigned int byteA;      // data byte0..byte3 (byte0 = lowest 8 bits)
	unsigned int byteB;      // data byte4..byte7
	unsigned char rtr;
	unsigned char dlc;
}CAN_MSG;

/* id1 and id2 are the two standard IDs this node accepts (id1 < id2) */
void can1_init(unsigned int id1,unsigned int id2)
{
	unsigned int *ptr=(unsigned int *)0xE0038000;   // acceptance filter RAM
	PINSEL1|=CAN_PINSEL;     // CAN1 pins
	C1MOD=0x01;              // CAN1 into Reset Mode
	C1BTR=0x00390000;        // 1Mbps @ PCLK=15MHz (BRP=0, 15 quanta per bit)
	ptr[0]=((0x0000|id1)<<16)|(0x0000|id2);   // 0x0000 = CAN1 controller
	SFF_sa=0x0;              // offset of start addr of individual addr table
	SFF_GRP_sa=0x4;
	EFF_sa=0x4;
	EFF_GRP_sa=0x4;
	ENDofTable=0x4;
	AFMR=0x0;                // Enable Acceptance Filter
	C1MOD=0x0;               // CAN1 into Normal Mode
}

void can1_tx(CAN_MSG m)
{
	unsigned int t;
	if(C1MOD&0x01)           // bus-off recovery
		C1MOD=0x0;
	if(!(C1SR&(1<<2)))       // Tx buffer1 still busy (no ACK from other node)
	{
		C1CMR=0x02;          // abort old frame
		for(t=0;t<20000&&!(C1SR&(1<<2));t++);
		if(!(C1SR&(1<<2)))
			return;
	}
	C1TFI1=((unsigned int)m.dlc<<16)|((unsigned int)m.rtr<<30);
	C1TID1=m.id;
	C1TDA1=m.byteA;
	C1TDB1=m.byteB;
	C1CMR=0x21;              // transmit request + select Tx buffer1
}

/* returns 1 if a frame was received and copied into *m */
unsigned char can1_rx(CAN_MSG *m)
{
	if(C1GSR&0x02)           // data overrun, clear it
		C1CMR=0x08;
	if(!(C1GSR&0x01))        // RBS = 0 -> nothing received
		return 0;
	m->id=C1RID;
	m->dlc=(C1RFS>>16)&0x0F;
	m->rtr=(C1RFS>>30)&0x01;
	m->byteA=C1RDA;
	m->byteB=C1RDB;
	C1CMR=0x04;              // release receive buffer
	return 1;
}
