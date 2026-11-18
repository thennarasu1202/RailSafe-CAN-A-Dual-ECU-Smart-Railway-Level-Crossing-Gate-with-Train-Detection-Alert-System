#include<LPC21XX.H>
#include"delay.h"

#define LCD_D 0xF<<22
#define RS 1<<20
#define E 1<<21
void LCD_INIT(void);
void LCD_COMMAND(unsigned char);
void LCD_DATA(unsigned char);

void LCD_NIBBLE(unsigned char n)
	{
	IOCLR1=LCD_D;
	IOSET1=(n&0x0F)<<22;
	IOSET1=E;
	delay_ms(5);
	IOCLR1=E;
	}
void LCD_INIT(void)
	{
	PINSEL2&=~(1<<3);          // P1.16-P1.25 as GPIO
	IODIR1|=LCD_D|RS|E;
	delay_ms(20);
	IOCLR1=RS;
	LCD_NIBBLE(0x03);
	delay_ms(5);
	LCD_NIBBLE(0x03);
	delay_ms(1);
	LCD_NIBBLE(0x03);
	delay_ms(1);
	LCD_NIBBLE(0x02);          // switch to 4-bit mode
	LCD_COMMAND(0x28);         // 4-bit, 2 lines
	LCD_COMMAND(0x0C);
	LCD_COMMAND(0x06);
	LCD_COMMAND(0x01);
	}
void LCD_COMMAND(unsigned char c)
	{
	IOCLR1=RS;
	LCD_NIBBLE(c>>4);
	LCD_NIBBLE(c&0x0F);
	}
void LCD_DATA(unsigned char d)
	{
	IOSET1=RS;
	LCD_NIBBLE(d>>4);
	LCD_NIBBLE(d&0x0F);
	}
void LCD_STRING(char*str)
{
unsigned int i=0;
while(*str!='\0')
{
LCD_DATA(*str++);
i++;
if(i==16)
LCD_COMMAND(0xc0);
}
}
void LCD_INT(unsigned int val){

	unsigned int arr[50];

	int j=0;

	while(val!=0){

		arr[j++]=val%10;

		val=val/10;

	}

	for(j--;j>=0;j--){

		LCD_DATA(arr[j]+48);

	}

}
