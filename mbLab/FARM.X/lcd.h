#ifndef LCD_H
#define LCD_H

#include <xc.h>

#define LCD_RS PORTDbits.RD0
#define LCD_EN PORTDbits.RD1
#define LCD_D4 PORTDbits.RD4
#define LCD_D5 PORTDbits.RD5
#define LCD_D6 PORTDbits.RD6
#define LCD_D7 PORTDbits.RD7

#define LCD_RS_DIR TRISDbits.TRISD0
#define LCD_EN_DIR TRISDbits.TRISD1
#define LCD_D4_DIR TRISDbits.TRISD4
#define LCD_D5_DIR TRISDbits.TRISD5
#define LCD_D6_DIR TRISDbits.TRISD6
#define LCD_D7_DIR TRISDbits.TRISD7

void LCD_Init();
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char data);
void LCD_String(const char *str);
void LCD_Clear();
void LCD_SetCursor(unsigned char row, unsigned char col);

#endif

