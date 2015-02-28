#if !defined(_N3310_H_)
#define _N3310_H_
#include "stm32f10x.h"
/*--------------------------------------------------------------------------------------------------
                                  General purpose constants
--------------------------------------------------------------------------------------------------*/
#define LCD_DC_PIN                 12
#define LCD_IN_PIN                 13
#define LCD_CLK_PIN                14
#define LCD_RST_PIN                15

#define LCD_DELAY { int i=16; while(i) {__NOP(); i--;}; }
typedef enum
{
    LCD_CMD  = 0,
    LCD_DATA = 1
} LcdCmdData;
void LcdSend    ( uint8_t data, LcdCmdData cd );

/*--------------------------------------------------------------------------------------------------
                                 Public function prototypes
--------------------------------------------------------------------------------------------------*/
void LcdInit       ( uint8_t Contrast );
#define X_POSITION 0x0100
#define Y_POSITION 0x1000
#define BIG_UP   0x8000 
#define BIG_DOWN 0x0080
#define INVERSE  0x80000000
#define X_OFFSET 0x00010000
/*--------------------------------------------------------------------------------------------------

  Name         :  LcdChr
  Argument(s)  :  Ctrl = X*X_MUL+y*Y_MUL+ BIG1 + BIG2 + OutLen
                  Inverse == 1 - inverse output
                  Str - out string
 Description  :  Displays a character at current cursor location and increment cursor location.
              Len from 0 to 84
              y from 0 to 5
              x from 0 to 13
              BIG_UP -  upper part of 16 point string
              BIG_DOWN -lower part of 16 point string 
              BIG_UP   | Y2 | Y1 | Y0 | X3 | X2 | X1 | X0
              BIG_DOWN | L6 | L5 | L4 | L3 | L2 | L1 | L0
              It is parts of Ctrl. Ctrl = X_MUL*X+Y_MUL*Y+LENGTH+(BIG_UP||BIG_DOWN||0)
              Length should be >= strlen(Str). 
              Example:LcdChr(5*X_MUL+3*Y_MUL+8, 1, "123") - will out string 123 in position 6 row 4 8 points font.
              The 5 symbols afrer 123 will be cleaned ( 8 - strlen("123"))

  Return value :  None.

--------------------------------------------------------------------------------------------------*/
void LcdChr ( uint32_t Ctrl, const char* Str );
/*  LcdChr ( Y_POSITION*2+X_POSITION*1+13+INVERSE+X_OFFSET*3, "Hello world" );*/

void LcdClear(void);
void LcdContrast ( uint8_t Contrast );
void LcdGotoXY(uint8_t X, uint8_t Y);

#endif  //  _N3310_H_
/*--------------------------------------------------------------------------------------------------
                                         End of file.
--------------------------------------------------------------------------------------------------*/
