#include<LPC21XX.H>
#include"delay.h"

/* 16x2 LCD, 4-bit mode (D4-D7 on P0.17-P0.20) so P0.23/P0.24 stay free for CAN1 */
#define LCD_D (0x0F<<17)
#define RS (1<<5)
#define E  (1<<6)

void LCD_INIT(void);
void LCD_COMMAND(unsigned char);
void LCD_DATA(unsigned char);
void LCD_STRING(char*);

static void lcd_nibble(unsigned char n)
{
    IOCLR0 = LCD_D;
    IOSET0 = ((unsigned int)(n & 0x0F)) << 17;
    IOSET0 = E;
    delay_ms(2);
    IOCLR0 = E;
}
void LCD_COMMAND(unsigned char c)
{
    IOCLR0 = RS;
    lcd_nibble(c >> 4);
    lcd_nibble(c);
    delay_ms(2);
}
void LCD_DATA(unsigned char d)
{
    IOSET0 = RS;
    lcd_nibble(d >> 4);
    lcd_nibble(d);
    delay_ms(2);
}
void LCD_INIT(void)
{
    IODIR0 |= LCD_D | RS | E;     /* |= so other pins are not disturbed */
    delay_ms(20);
    IOCLR0 = RS;
    lcd_nibble(0x03); delay_ms(5);
    lcd_nibble(0x03); delay_ms(1);
    lcd_nibble(0x03);
    lcd_nibble(0x02);             /* switch to 4-bit */
    LCD_COMMAND(0x28);            /* 2 line, 5x7 */
    LCD_COMMAND(0x0C);            /* display on, cursor off */
    LCD_COMMAND(0x06);
    LCD_COMMAND(0x01);
}
void LCD_STRING(char*str)
{
    while(*str)
        LCD_DATA(*str++);
}
