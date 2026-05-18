#include "lcd.h"

#define _XTAL_FREQ 8000000

void LCD_Pulse() {
    LCD_EN = 1;
    __delay_us(1);
    LCD_EN = 0;
    __delay_us(100);
}

void LCD_Send4Bit(unsigned char data) {
    LCD_D4 = (data >> 0) & 1;
    LCD_D5 = (data >> 1) & 1;
    LCD_D6 = (data >> 2) & 1;
    LCD_D7 = (data >> 3) & 1;
    LCD_Pulse();
}

void LCD_Command(unsigned char cmd) {
    LCD_RS = 0;
    LCD_Send4Bit(cmd >> 4);
    LCD_Send4Bit(cmd & 0x0F);
    __delay_ms(2);
}

void LCD_Char(unsigned char data) {
    LCD_RS = 1;
    LCD_Send4Bit(data >> 4);
    LCD_Send4Bit(data & 0x0F);
    __delay_ms(2);
}

void LCD_String(const char *str) {
    while (*str) {
        LCD_Char(*str++);
    }
}

void LCD_Clear() {
    LCD_Command(0x01);
    __delay_ms(2);
}

void LCD_SetCursor(unsigned char row, unsigned char col) {
    unsigned char pos;
    if (row == 1)
        pos = 0x80 + (col - 1);
    else
        pos = 0xC0 + (col - 1);

    LCD_Command(pos);
}

void LCD_Init() {

    LCD_RS_DIR = 0;
    LCD_EN_DIR = 0;
    LCD_D4_DIR = 0;
    LCD_D5_DIR = 0;
    LCD_D6_DIR = 0;
    LCD_D7_DIR = 0;

    __delay_ms(20);

    LCD_RS = 0;
    LCD_Send4Bit(0x03);
    __delay_ms(5);
    LCD_Send4Bit(0x03);
    __delay_us(100);
    LCD_Send4Bit(0x03);
    LCD_Send4Bit(0x02);

    LCD_Command(0x28); // 4-bit, 2 line
    LCD_Command(0x0C); // display on
    LCD_Command(0x06); // entry mode
    LCD_Clear();
}
