#include<LPC21XX.H>

#define CAN_TRAIN_STATUS 0x100   // ECU1 -> ECU2
#define CAN_GATE_STATUS  0x200   // ECU2 -> ECU1
#define CAN_EMERGENCY    0x7FF   // either

typedef struct CAN_MSG
{
	unsigned int id;
	unsigned int byteA;      // data byte0..byte3 (byte0 = lowest 8 bits)
	unsigned int byteB;      // data byte4..byte7
	unsigned char rtr;
	unsigned char dlc;
}CAN_MSG;

/* id1 and id2 are the two standard IDs this node accepts (id1 < id2) */
void can2_init(unsigned int id1,unsigned int id2)
{
	unsigned int *ptr=(unsigned int *)0xE0038000;   // acceptance filter RAM
	PINSEL1|=0x00014000;     // P0.23 -> RD2, P0.24 -> TD2
	C2MOD=0x01;              // CAN2 into Reset Mode
	C2BTR=0x00390007;        // 125kbps @ PCLK=15MHz (BRP=7, 15 quanta per bit)
	ptr[0]=((0x2000|id1)<<16)|(0x2000|id2);   // 0x2000 = CAN2 controller
	SFF_sa=0x0;              // offset of start addr of individual addr table
	SFF_GRP_sa=0x4;
	EFF_sa=0x4;
	EFF_GRP_sa=0x4;
	ENDofTable=0x4;
	AFMR=0x0;                // Enable Acceptance Filter
	C2MOD=0x0;               // CAN2 into Normal Mode
}

void can2_tx(CAN_MSG m)
{
	unsigned int t;
	if(C2MOD&0x01)           // bus-off recovery
		C2MOD=0x0;
	if(!(C2SR&(1<<2)))       // Tx buffer1 still busy (no ACK from other node)
	{
		C2CMR=0x02;          // abort old frame
		for(t=0;t<20000&&!(C2SR&(1<<2));t++);
		if(!(C2SR&(1<<2)))
			return;
	}
	C2TFI1=((unsigned int)m.dlc<<16)|((unsigned int)m.rtr<<30);
	C2TID1=m.id;
	C2TDA1=m.byteA;
	C2TDB1=m.byteB;
	C2CMR=0x21;              // transmit request + select Tx buffer1
}

/* returns 1 if a frame was received and copied into *m */
unsigned char can2_rx(CAN_MSG *m)
{
	if(C2GSR&0x02)           // data overrun, clear it
		C2CMR=0x08;
	if(!(C2GSR&0x01))        // RBS = 0 -> nothing received
		return 0;
	m->id=C2RID;
	m->dlc=(C2RFS>>16)&0x0F;
	m->rtr=(C2RFS>>30)&0x01;
	m->byteA=C2RDA;
	m->byteB=C2RDB;
	C2CMR=0x04;              // release receive buffer
	return 1;
}
