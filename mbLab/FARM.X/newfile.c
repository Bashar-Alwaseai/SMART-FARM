
#include <xc.h>

//====================================================
// CONFIG BITS
//====================================================
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 8000000

//====================================================
// INPUTS
//====================================================
#define SOIL_DRY     RA0
#define WATER_OK     RA1
#define EMERGENCY_SW RA2
//#define MANUAL_SW    RA3

//====================================================
// OUTPUTS
//====================================================
#define PUMP_LED     RC0
#define Z1_LED       RC1
#define Z2_LED       RC2
#define Z3_LED       RC3
#define ALARM_LED    RC4
#define STATUS_LED   RC5

//====================================================
// LCD PINS
//====================================================
#define LCD_RS RD0
#define LCD_EN RD1
#define LCD_D4 RD4
#define LCD_D5 RD5
#define LCD_D6 RD6
#define LCD_D7 RD7

//====================================================
// 7-SEGMENT PINS (CC)
//====================================================
#define SEG_A RA3
#define SEG_B RA5
#define SEG_C RE0
#define SEG_D RE1
#define SEG_E RE2
#define SEG_F RC6
#define SEG_G RC7

#define DIG1 RD2
#define DIG2 RD3

//====================================================
// MODES
//====================================================
#define M_HOME 0
#define M_MAN  1
#define M_AUTO 2
#define M_SCH  3
#define M_SET  4
#define M_ALM  5

#define R_IDLE 0
#define R_WAIT 1
#define R_Z1   2
#define R_Z2   3
#define R_Z3   4
#define R_DONE 5

#define A_NONE 0
#define A_EMG  1
#define A_WAT  2

//====================================================
// GLOBALS
//====================================================
unsigned char mode = M_HOME;
unsigned char prevMode = M_HOME;
unsigned char runState = R_IDLE;
unsigned char alarmType = A_NONE;

unsigned char T1 = 10;
unsigned char T2 = 8;
unsigned char T3 = 6;
unsigned char Cyc = 2;
unsigned char StartAfter = 15;

unsigned char cnt = 0;
unsigned char zone = 0;
unsigned char cycleNow = 0;
unsigned char doneHold = 0;

unsigned char setItem = 0;
unsigned char editVal = 0;

unsigned char secFlag = 0;
unsigned char blinkFlag = 0;
unsigned char lcdNeed = 1;
unsigned char mux = 0;

unsigned char key = 0;
unsigned char keyOld = 0;

unsigned int t10 = 0;
unsigned char bt = 0;

char txt[4];

const unsigned char segCode[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

//====================================================
// FUNCTION PROTOTYPES
//====================================================
void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_init(void);
void lcd_clear(void);
void lcd_setCursor(unsigned char row, unsigned char col);
void lcd_out(unsigned char row, unsigned char col, const char *str);
void lcd_chr(unsigned char row, unsigned char col, char ch);

void to2(unsigned char v);
void allOff(void);
void setZone(unsigned char z);
void alarmOn(unsigned char a);
void alarmReset(void);
void goHome(void);
void segOut(unsigned char p);
unsigned char getKey(void);
void initSystem(void);

//====================================================
// LCD LOW LEVEL
//====================================================
void lcd_pulseEnable(void) {
    LCD_EN = 1;
    __delay_us(2);
    LCD_EN = 0;
    __delay_us(50);
}

void lcd_send4bit(unsigned char data) {
    LCD_D4 = (data >> 0) & 1;
    LCD_D5 = (data >> 1) & 1;
    LCD_D6 = (data >> 2) & 1;
    LCD_D7 = (data >> 3) & 1;
    lcd_pulseEnable();
}

void lcd_cmd(unsigned char cmd) {
    LCD_RS = 0;
    lcd_send4bit(cmd >> 4);
    lcd_send4bit(cmd & 0x0F);

    if (cmd == 0x01 || cmd == 0x02)
        __delay_ms(2);
    else
        __delay_us(60);
}

void lcd_data(unsigned char data) {
    LCD_RS = 1;
    lcd_send4bit(data >> 4);
    lcd_send4bit(data & 0x0F);
    __delay_us(60);
}

void lcd_init(void) {
    __delay_ms(20);

    LCD_RS = 0;
    LCD_EN = 0;

    lcd_send4bit(0x03);
    __delay_ms(5);

    lcd_send4bit(0x03);
    __delay_us(200);

    lcd_send4bit(0x03);
    __delay_us(200);

    lcd_send4bit(0x02);
    __delay_us(200);

    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
    __delay_ms(2);
}

void lcd_clear(void) {
    lcd_cmd(0x01);
    __delay_ms(2);
}

void lcd_setCursor(unsigned char row, unsigned char col) {
    unsigned char addr;
    if (row == 1) addr = 0x80 + (col - 1);
    else          addr = 0xC0 + (col - 1);
    lcd_cmd(addr);
}

void lcd_out(unsigned char row, unsigned char col, const char *str) {
    lcd_setCursor(row, col);
    while (*str) lcd_data(*str++);
}

void lcd_chr(unsigned char row, unsigned char col, char ch) {
    lcd_setCursor(row, col);
    lcd_data(ch);
}

//====================================================
// HELPERS
//====================================================
void to2(unsigned char v) {
    txt[0] = (v / 10) + '0';
    txt[1] = (v % 10) + '0';
    txt[2] = 0;
}

void allOff(void) {
    PUMP_LED = 0;
    Z1_LED = 0;
    Z2_LED = 0;
    Z3_LED = 0;
    STATUS_LED = 0;
    // ?? ???? ALARM ??? ???? ??? latched
}

void setZone(unsigned char z) {
    zone = z;

    Z1_LED = 0;
    Z2_LED = 0;
    Z3_LED = 0;

    if (z == 1) Z1_LED = 1;
    if (z == 2) Z2_LED = 1;
    if (z == 3) Z3_LED = 1;

    if (z == 0) PUMP_LED = 0;
    else        PUMP_LED = 1;

    lcdNeed = 1;
}

void goHome(void) {
    mode = M_HOME;
    runState = R_IDLE;
    cnt = 0;
    zone = 0;
    cycleNow = 0;
    doneHold = 0;
    setItem = 0;
    setZone(0);
    lcdNeed = 1;
}

void alarmOn(unsigned char a) {
    alarmType = a;
    prevMode = mode;
    mode = M_ALM;
    runState = R_IDLE;
    cnt = 0;
    zone = 0;
    cycleNow = 0;
    doneHold = 0;

    PUMP_LED = 0;
    Z1_LED = 0;
    Z2_LED = 0;
    Z3_LED = 0;

    ALARM_LED = 1;   // ???? ????
    lcdNeed = 1;
}

void alarmReset(void) {
    alarmType = A_NONE;
    mode = M_HOME;
    runState = R_IDLE;
    cnt = 0;
    zone = 0;
    cycleNow = 0;
    doneHold = 0;
    setItem = 0;

    allOff();
    ALARM_LED = 0;   // ?? ???? ??? ???
    lcdNeed = 1;
}

void segOut(unsigned char p) {
    SEG_A = (p >> 0) & 1;
    SEG_B = (p >> 1) & 1;
    SEG_C = (p >> 2) & 1;
    SEG_D = (p >> 3) & 1;
    SEG_E = (p >> 4) & 1;
    SEG_F = (p >> 5) & 1;
    SEG_G = (p >> 6) & 1;
}

//====================================================
// INTERRUPT
//====================================================
void __interrupt() isr(void) {
    unsigned char val, t, o;

    if (TMR1IF) {
        TMR1IF = 0;

        TMR1H = 0xF6;
        TMR1L = 0x3C;

        // 7SEG
        if (mode == M_ALM) {
            if (alarmType == A_EMG)      val = 1;
            else if (alarmType == A_WAT) val = 2;
            else                         val = 0;
        } else {
            val = cnt;
            if (val > 99) val = 99;
        }

        t = val / 10;
        o = val % 10;

        DIG1 = 1;
        DIG2 = 1;

        if (mux == 0) {
            segOut(segCode[t]);
            DIG1 = 0;
            mux = 1;
        } else {
            segOut(segCode[o]);
            DIG2 = 0;
            mux = 0;
        }

        t10++;
        bt++;

        if (bt >= 50) {
            bt = 0;
            blinkFlag = !blinkFlag;
        }

        if (t10 >= 100) {
            t10 = 0;
            secFlag = 1;
        }
    }
}

//====================================================
// KEYPAD
//====================================================
// Mapping:
// ÷ = A
// × = B
// - = C
// + = D
// = = *
// ON/C = #

unsigned char getKey(void) {
    PORTB = 0x0F;
    __delay_us(50);

    // Col 1
    RB0 = 0; RB1 = 1; RB2 = 1; RB3 = 1;
    __delay_us(50);
    if (RB4 == 0) return '7';
    if (RB5 == 0) return '4';
    if (RB6 == 0) return '1';
    if (RB7 == 0) return '#';

    // Col 2
    RB0 = 1; RB1 = 0; RB2 = 1; RB3 = 1;
    __delay_us(50);
    if (RB4 == 0) return '8';
    if (RB5 == 0) return '5';
    if (RB6 == 0) return '2';
    if (RB7 == 0) return '0';

    // Col 3
    RB0 = 1; RB1 = 1; RB2 = 0; RB3 = 1;
    __delay_us(50);
    if (RB4 == 0) return '9';
    if (RB5 == 0) return '6';
    if (RB6 == 0) return '3';
    if (RB7 == 0) return '*';

    // Col 4
    RB0 = 1; RB1 = 1; RB2 = 1; RB3 = 0;
    __delay_us(50);
    if (RB4 == 0) return 'A';
    if (RB5 == 0) return 'B';
    if (RB6 == 0) return 'C';
    if (RB7 == 0) return 'D';

    return 0;
}

//====================================================
// INIT
//====================================================
void initSystem(void) {
    ADCON1 = 0x06;

    TRISA = 0b00010111;
    TRISB = 0b11110000;
    TRISC = 0x00;
    TRISD = 0x00;
    TRISE = 0x00;

    PORTA = 0x00;
    PORTB = 0x0F;
    PORTC = 0x00;
    PORTD = 0x00;
    PORTE = 0x00;

    OPTION_REG &= 0x7F; // pull-up enable

    lcd_init();

    T1CON = 0x31;
    TMR1H = 0xF6;
    TMR1L = 0x3C;
    TMR1IF = 0;
    TMR1IE = 1;
    PEIE = 1;
    GIE = 1;

    allOff();
    ALARM_LED = 0;

    lcd_clear();
    lcd_out(1,1,"SMART FARM");
    lcd_out(2,1,"READY");
    __delay_ms(1200);

    mode = M_HOME;
    lcdNeed = 1;
}

//====================================================
// MAIN
//====================================================
void main(void) {
    unsigned char k;

    initSystem();

    while(1) {

        //================ SAFETY =================
        if (EMERGENCY_SW == 1 && mode != M_ALM) {
            alarmOn(A_EMG);
        }

        if (mode != M_ALM) {
            if ((runState == R_WAIT || runState == R_Z1 || runState == R_Z2 || runState == R_Z3) && (WATER_OK == 0)) {
                alarmOn(A_WAT);
            }
        }

        //================ KEY READ =================
        k = getKey();

        if (k != 0 && keyOld == 0) {
            __delay_ms(20);
            if (getKey() == k) key = k;
            else key = 0;
        } else {
            key = 0;
        }

        keyOld = k;

        //================ ALARM RESET =================
        if (mode == M_ALM && key == '#') {
            alarmReset();
        }

        //================ HOME MENU =================
        if (mode == M_HOME) {
            if (key == 'A') {
                mode = M_MAN;
                runState = R_IDLE;
                setZone(0);
                lcdNeed = 1;
            }
            else if (key == 'B') {
                mode = M_AUTO;
                runState = R_IDLE;
                setZone(0);
                lcdNeed = 1;
            }
            else if (key == 'C') {
                mode = M_SCH;
                runState = R_WAIT;
                cnt = StartAfter;
                cycleNow = 0;
                doneHold = 0;
                setZone(0);
                lcdNeed = 1;
            }
            else if (key == 'D') {
                prevMode = M_HOME;
                mode = M_SET;
                setItem = 9;
                lcdNeed = 1;
            }
        }

        //================ RETURN TO HOME =================
        if (mode != M_HOME && mode != M_ALM && key == '*') {
            goHome();
        }

        //================ MANUAL SWITCH =================
        //if (MANUAL_SW == 1 && mode != M_ALM) {
        //    mode = M_MAN;
        //    runState = R_IDLE;
        //    cnt = 0;
        //    cycleNow = 0;
        //    doneHold = 0;
        //    setItem = 0;
        //    setZone(0);
        //    lcdNeed = 1;
        //}

        //================ MANUAL =================
        if (mode == M_MAN) {
            if (key == '1') setZone(1);
            else if (key == '2') setZone(2);
            else if (key == '3') setZone(3);
            else if (key == '0') setZone(0);
        }

        //================ AUTO START =================
        if (mode == M_AUTO && runState == R_IDLE) {
            if (SOIL_DRY == 1 && WATER_OK == 1) {
                runState = R_Z1;
                cycleNow = 1;
                cnt = T1;
                setZone(1);
                lcdNeed = 1;
            }
        }

        //================ SETTINGS =================
        if (mode == M_SET) {
            if (setItem == 9) {
                if (key == '1') { setItem = 1; editVal = T1; lcdNeed = 1; }
                else if (key == '2') { setItem = 2; editVal = T2; lcdNeed = 1; }
                else if (key == '3') { setItem = 3; editVal = T3; lcdNeed = 1; }
                else if (key == '4') { setItem = 4; editVal = Cyc; lcdNeed = 1; }
                else if (key == '5') { setItem = 5; editVal = StartAfter; lcdNeed = 1; }
            }
            else {
                if (key == '8') {
                    if (setItem == 4) {
                        if (editVal < 9) editVal++;
                    } else {
                        if (editVal < 99) editVal++;
                    }
                    lcdNeed = 1;
                }
                else if (key == '2') {
                    if (editVal > 1) editVal--;
                    lcdNeed = 1;
                }
                else if (key == '#') {
                    if (setItem == 1) T1 = editVal;
                    else if (setItem == 2) T2 = editVal;
                    else if (setItem == 3) T3 = editVal;
                    else if (setItem == 4) Cyc = editVal;
                    else if (setItem == 5) StartAfter = editVal;

                    setItem = 9;
                    lcdNeed = 1;
                }
            }
        }

        //================ 1 SECOND ENGINE =================
        if (secFlag) {
            secFlag = 0;

            if (mode == M_ALM) {
                STATUS_LED = blinkFlag;
                lcdNeed = 1;
            } else {
                STATUS_LED = !STATUS_LED;

                if (runState == R_DONE) {
                    if (doneHold < 3) doneHold++;
                    else {
                        doneHold = 0;
                        runState = R_IDLE;
                        cycleNow = 0;
                        cnt = 0;
                        setZone(0);
                    }
                    lcdNeed = 1;
                }

                else if (mode == M_SCH && runState == R_WAIT) {
                    if (cnt > 0) cnt--;

                    if (cnt == 0) {
                        if (SOIL_DRY == 1 && WATER_OK == 1) {
                            runState = R_Z1;
                            cycleNow = 1;
                            cnt = T1;
                            setZone(1);
                        } else {
                            runState = R_IDLE;
                            setZone(0);
                        }
                    }
                    lcdNeed = 1;
                }

                else if (runState == R_Z1 || runState == R_Z2 || runState == R_Z3) {
                    if (cnt > 0) cnt--;

                    if (cnt == 0) {
                        if (runState == R_Z1) {
                            runState = R_Z2;
                            cnt = T2;
                            setZone(2);
                        }
                        else if (runState == R_Z2) {
                            runState = R_Z3;
                            cnt = T3;
                            setZone(3);
                        }
                        else if (runState == R_Z3) {
                            if (cycleNow < Cyc) {
                                cycleNow++;
                                runState = R_Z1;
                                cnt = T1;
                                setZone(1);
                            } else {
                                runState = R_DONE;
                                cnt = 0;
                                setZone(0);
                            }
                        }
                    }
                    lcdNeed = 1;
                }
            }
        }

        //================ LCD =================
        if (lcdNeed) {
            lcdNeed = 0;
            lcd_clear();

            if (mode == M_HOME) {
                lcd_out(1,1,"A:MAN B:AUTO");
                lcd_out(2,1,"C:SCH D:SET");
            }

            else if (mode == M_ALM) {
                lcd_out(1,1,"!! ALARM !!");
                if (alarmType == A_EMG) lcd_out(2,1,"EMERG #RESET");
                else if (alarmType == A_WAT) lcd_out(2,1,"WATER #RESET");
            }

            else if (mode == M_SET) {
                if (setItem == 9) {
                    lcd_out(1,1,"1T1 2T2 3T3");
                    lcd_out(2,1,"4CY 5ST *=BK");
                } else {
                    if (setItem == 1) lcd_out(1,1,"SET T1");
                    else if (setItem == 2) lcd_out(1,1,"SET T2");
                    else if (setItem == 3) lcd_out(1,1,"SET T3");
                    else if (setItem == 4) lcd_out(1,1,"SET CY");
                    else if (setItem == 5) lcd_out(1,1,"SET ST");

                    to2(editVal);
                    lcd_out(2,1,"V=");
                    lcd_out(2,3,txt);
                    lcd_out(2,7,"8+2-#S");
                }
            }

            else {
                if (mode == M_MAN)      lcd_out(1,1,"M:MAN");
                else if (mode == M_AUTO) lcd_out(1,1,"M:AUTO");
                else if (mode == M_SCH)  lcd_out(1,1,"M:SCH");

                if (runState == R_WAIT) {
                    lcd_out(1,8,"WT");
                    to2(cnt);
                    lcd_out(1,11,txt);
                } else {
                    lcd_out(1,8,"Z:");
                    if (zone == 1)      lcd_chr(1,10,'1');
                    else if (zone == 2) lcd_chr(1,10,'2');
                    else if (zone == 3) lcd_chr(1,10,'3');
                    else                lcd_chr(1,10,'0');

                    lcd_out(1,12,"T:");
                    to2(cnt);
                    lcd_out(1,14,txt);
                }

                lcd_out(2,1,"C:");
                lcd_chr(2,3,cycleNow + '0');
                lcd_out(2,4,"/");
                lcd_chr(2,5,Cyc + '0');

                if (mode == M_MAN)           lcd_out(2,8,"*:HOME");
                else if (runState == R_IDLE) lcd_out(2,8,"IDLE");
                else if (runState == R_WAIT) lcd_out(2,8,"READY");
                else if (runState == R_DONE) lcd_out(2,8,"DONE");
                else                         lcd_out(2,8,"RUN");
            }
        }
    }
}



