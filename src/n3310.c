#include "n3310.h"

/*--------------------------------------------------------------------------------------------------
                                Private function prototypes
--------------------------------------------------------------------------------------------------*/
//  Function prototypes are mandatory otherwise the compiler generates unreliable code.

const uint8_t FontLookup [] =
{
     0x00, 0x00, 0x00, 0x00, 0x00 ,  // sp
     0x00, 0x00, 0x2f, 0x00, 0x00 ,   // !
     0x00, 0x07, 0x00, 0x07, 0x00 ,   // "
     0x14, 0x7f, 0x14, 0x7f, 0x14 ,   // #
     0x24, 0x2a, 0x7f, 0x2a, 0x12 ,   // $
     0xc4, 0xc8, 0x10, 0x26, 0x46 ,   // %
     0x36, 0x49, 0x55, 0x22, 0x50 ,   // &
     0x00, 0x05, 0x03, 0x00, 0x00 ,   // '
     0x00, 0x1c, 0x22, 0x41, 0x00 ,   // (
     0x00, 0x41, 0x22, 0x1c, 0x00 ,   // )
     0x14, 0x08, 0x3E, 0x08, 0x14 ,   // *
     0x08, 0x08, 0x3E, 0x08, 0x08 ,   // +
     0x00, 0x00, 0x50, 0x30, 0x00 ,   // ,
     0x10, 0x10, 0x10, 0x10, 0x10 ,   // -
     0x00, 0x60, 0x60, 0x00, 0x00 ,   // .
     0x20, 0x10, 0x08, 0x04, 0x02 ,   // /
     0x3E, 0x51, 0x49, 0x45, 0x3E ,   // 0
     0x00, 0x42, 0x7F, 0x40, 0x00 ,   // 1
     0x42, 0x61, 0x51, 0x49, 0x46 ,   // 2
     0x21, 0x41, 0x45, 0x4B, 0x31 ,   // 3
     0x18, 0x14, 0x12, 0x7F, 0x10 ,   // 4
     0x27, 0x45, 0x45, 0x45, 0x39 ,   // 5
     0x3C, 0x4A, 0x49, 0x49, 0x30 ,   // 6
     0x01, 0x71, 0x09, 0x05, 0x03 ,   // 7
     0x36, 0x49, 0x49, 0x49, 0x36 ,   // 8
     0x06, 0x49, 0x49, 0x29, 0x1E ,   // 9
     0x00, 0x36, 0x36, 0x00, 0x00 ,   // :
     0x00, 0x56, 0x36, 0x00, 0x00 ,   // ;
     0x08, 0x14, 0x22, 0x41, 0x00 ,   // <
     0x14, 0x14, 0x14, 0x14, 0x14 ,   // =
     0x00, 0x41, 0x22, 0x14, 0x08 ,   // >
     0x02, 0x01, 0x51, 0x09, 0x06 ,   // ?
     0x32, 0x49, 0x59, 0x51, 0x3E ,   // @
     0x7E, 0x11, 0x11, 0x11, 0x7E ,   // A
     0x7F, 0x49, 0x49, 0x49, 0x36 ,   // B
     0x3E, 0x41, 0x41, 0x41, 0x22 ,   // C
     0x7F, 0x41, 0x41, 0x22, 0x1C ,   // D
     0x7F, 0x49, 0x49, 0x49, 0x41 ,   // E
     0x7F, 0x09, 0x09, 0x09, 0x01 ,   // F
     0x3E, 0x41, 0x49, 0x49, 0x7A ,   // G
     0x7F, 0x08, 0x08, 0x08, 0x7F ,   // H
     0x00, 0x41, 0x7F, 0x41, 0x00 ,   // I
     0x20, 0x40, 0x41, 0x3F, 0x01 ,   // J
     0x7F, 0x08, 0x14, 0x22, 0x41 ,   // K
     0x7F, 0x40, 0x40, 0x40, 0x40 ,   // L
     0x7F, 0x02, 0x0C, 0x02, 0x7F ,   // M
     0x7F, 0x04, 0x08, 0x10, 0x7F ,   // N
     0x3E, 0x41, 0x41, 0x41, 0x3E ,   // O
     0x7F, 0x09, 0x09, 0x09, 0x06 ,   // P
     0x3E, 0x41, 0x51, 0x21, 0x5E ,   // Q
     0x7F, 0x09, 0x19, 0x29, 0x46 ,   // R
     0x46, 0x49, 0x49, 0x49, 0x31 ,   // S
     0x01, 0x01, 0x7F, 0x01, 0x01 ,   // T
     0x3F, 0x40, 0x40, 0x40, 0x3F ,   // U
     0x1F, 0x20, 0x40, 0x20, 0x1F ,   // V
     0x3F, 0x40, 0x38, 0x40, 0x3F ,   // W
     0x63, 0x14, 0x08, 0x14, 0x63 ,   // X
     0x07, 0x08, 0x70, 0x08, 0x07 ,   // Y
     0x61, 0x51, 0x49, 0x45, 0x43 ,   // Z
     0x00, 0x7F, 0x41, 0x41, 0x00 ,   // [
     0x55, 0x2A, 0x55, 0x2A, 0x55 ,   // 55
     0x00, 0x41, 0x41, 0x7F, 0x00 ,   // ]
     0x04, 0x02, 0x01, 0x02, 0x04 ,   // ^
     0x40, 0x40, 0x40, 0x40, 0x40 ,   // _
     0x00, 0x01, 0x02, 0x04, 0x00 ,   // '
     0x20, 0x54, 0x54, 0x54, 0x78 ,   // a
     0x7F, 0x48, 0x44, 0x44, 0x38 ,   // b
     0x38, 0x44, 0x44, 0x44, 0x20 ,   // c
     0x38, 0x44, 0x44, 0x48, 0x7F ,   // d
     0x38, 0x54, 0x54, 0x54, 0x18 ,   // e
     0x08, 0x7E, 0x09, 0x01, 0x02 ,   // f
     0x0C, 0x52, 0x52, 0x52, 0x3E ,   // g
     0x7F, 0x08, 0x04, 0x04, 0x78 ,   // h
     0x00, 0x44, 0x7D, 0x40, 0x00 ,   // i
     0x20, 0x40, 0x44, 0x3D, 0x00 ,   // j
     0x7F, 0x10, 0x28, 0x44, 0x00 ,   // k
     0x00, 0x41, 0x7F, 0x40, 0x00 ,   // l
     0x7C, 0x04, 0x18, 0x04, 0x78 ,   // m
     0x7C, 0x08, 0x04, 0x04, 0x78 ,   // n
     0x38, 0x44, 0x44, 0x44, 0x38 ,   // o
     0x7C, 0x14, 0x14, 0x14, 0x08 ,   // p
     0x08, 0x14, 0x14, 0x18, 0x7C ,   // q
     0x7C, 0x08, 0x04, 0x04, 0x08 ,   // r
     0x48, 0x54, 0x54, 0x54, 0x20 ,   // s
     0x04, 0x3F, 0x44, 0x40, 0x20 ,   // t
     0x3C, 0x40, 0x40, 0x20, 0x7C ,   // u
     0x1C, 0x20, 0x40, 0x20, 0x1C ,   // v
     0x3C, 0x40, 0x30, 0x40, 0x3C ,   // w
     0x44, 0x28, 0x10, 0x28, 0x44 ,   // x
     0x0C, 0x50, 0x50, 0x50, 0x3C ,   // y
     0x44, 0x64, 0x54, 0x4C, 0x44     // z
};



/*--------------------------------------------------------------------------------------------------

  Name         :  LcdInit

  Description  :  Performs PINS & LCD controller initialization.

  Argument(s)  :  None.

  Return value :  None.

--------------------------------------------------------------------------------------------------*/
void LcdInit ( uint8_t Contrast )
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; /* Port B Clock */

  GPIOB->CRH &= ~(((GPIO_CRL_MODE0|GPIO_CRL_CNF0)<<(LCD_DC_PIN-8)*4) |
                  ((GPIO_CRL_MODE0|GPIO_CRL_CNF0)<<(LCD_IN_PIN-8)*4) |
                  ((GPIO_CRL_MODE0|GPIO_CRL_CNF0)<<(LCD_CLK_PIN-8)*4)|
                  ((GPIO_CRL_MODE0|GPIO_CRL_CNF0)<<(LCD_RST_PIN-8)*4) ); // Reset all pins


  GPIOB->CRH |=  (((GPIO_CRL_MODE0_1)<<(LCD_DC_PIN-8)*4) |
                  ((GPIO_CRL_MODE0_1)<<(LCD_IN_PIN-8)*4) |
                  ((GPIO_CRL_MODE0_1)<<(LCD_CLK_PIN-8)*4)|
                  ((GPIO_CRL_MODE0_1)<<(LCD_RST_PIN-8)*4) ); // Pins in push-pull 10MHz
    
  GPIOB->BRR = 1<<LCD_RST_PIN; /* RESET to 0 */
  if (Contrast == 0)
    Contrast = 60;
  if (Contrast > 90)
    Contrast = 90;
  /* Some delay */
  {
    __IO uint32_t Counter;
    for ( Counter = 0; Counter <10; Counter++)
      ; /* Blank */
  }
  GPIOB->BSRR = 1<<LCD_RST_PIN; /* RESET to 1 */
#define FUNCTION_SET 0x20
#define FUNCTION_PD  0x04
#define FUNCTION_V   0x02
#define FUNCTION_EXT 0x01
  LcdSend( FUNCTION_SET|FUNCTION_EXT, LCD_CMD );  // LCD Extended Commands.
#define SET_VOP      0x80
  LcdSend( SET_VOP|Contrast, LCD_CMD );  // Set LCD Vop (Contrast). 3.06+0.06*72=4.38V
#define TEMPERATURE_COEFFICIENT 0x4
  LcdSend( TEMPERATURE_COEFFICIENT | 2, LCD_CMD );  // Set Temp coefficent. 2 (0-3)
#define SET_BIAS 0x10
#define BIAS_1_10 0x07
#define BIAS_1_18 0x06
#define BIAS_1_24 0x05
#define BIAS_1_40 0x04
#define BIAS_1_48 0x03
#define BIAS_1_65 0x02
#define BIAS_1_80 0x01
#define BIAS_1_100 0x00
  LcdSend( SET_BIAS|BIAS_1_48, LCD_CMD );  // LCD bias mode 1:48.

  LcdSend( FUNCTION_SET, LCD_CMD );  // LCD Standard Commands, Horizontal addressing mode.
#define DISPLAY_CONTROL 0x08
#define DISPLAY_BLANK   0x00
#define DISPLAY_NORMAL  0x04
#define DISPLAY_ALLON   0x01
#define DISPLAY_INVERSE 0x05
  LcdSend( DISPLAY_CONTROL|DISPLAY_NORMAL, LCD_CMD );  // LCD in normal mode.

#define SET_Y 0x40 /* from 0 to 5 */
#define SET_X 0x80 /* from 0 to 83 */
//  LcdClear();
}

//Clear display
void LcdClear()
{
	uint16_t j = 7;
  
	while ( j-- )
	{
      uint16_t i = 84;

      LcdSend(SET_Y|j, LCD_CMD ); 
      LcdSend(SET_X|0, LCD_CMD);
      while ( i-- )
      {
        LcdSend(0/*xff*/, LCD_DATA);
      }  
	}
}

/*--------------------------------------------------------------------------------------------------

  Name         :  LcdContrast

  Description  :  Set display contrast.

  Argument(s)  :  contrast -> Contrast value from 0x00 to 0x7F.

  Return value :  None.

  Notes        :  No change visible at ambient temperature.

--------------------------------------------------------------------------------------------------*/
void LcdContrast ( uint8_t Contrast )
{
    //  LCD Extended Commands.
  LcdSend( FUNCTION_SET|FUNCTION_EXT, LCD_CMD );  // LCD Extended Commands.

    // Set LCD Vop (Contrast).
    if ( Contrast > 90 ) /* Limit by 8.5 V */
      Contrast = 90;
  LcdSend( SET_VOP | Contrast, LCD_CMD );

    //  LCD Standard Commands, horizontal addressing mode.
  LcdSend( FUNCTION_SET, LCD_CMD );
}

void LcdGotoXY(uint8_t X, uint8_t Y)
{
    LcdSend( SET_X|X, LCD_CMD );
    LcdSend( SET_Y|Y, LCD_CMD ); 
}


/*--------------------------------------------------------------------------------------------------

  Name         :  LcdChr

  Description  :  Displays a character at current cursor location and increment cursor location.

  Argument(s)  :  Ctrl = X*X_MUL+y*Y_MUL+ BIG1 + BIG2 + OutLen
                  NoInverse == 0 - inverse output
                  Str - out string

  Return value :  None.

--------------------------------------------------------------------------------------------------*/

void LcdChr ( uint32_t Ctrl, const char* Str )
{
  unsigned char ch;
  uint8_t Inverse;
#if defined(BIG)
#define INTERNAL_BIG_DOWN
  uint32_t Big = Ctrl;
#endif  
  Inverse = ((Ctrl & INVERSE) != 0);
  LcdGotoXY((Ctrl/X_POSITION & 0x0F)*6 + ((Ctrl/X_OFFSET)& 0x3F), 
            (Ctrl/Y_POSITION) & 0x07);

  Ctrl = Ctrl & 0x7F; /* Only Len */
  while ( Ctrl != 0 )
  {
    const uint8_t* Start;

    if ( (ch = *Str) != 0)
      Str++;
    Ctrl--;
    if ( ch == 0 ) 
      ch = ' ';
    if ( (ch < 0x20) || (ch > 0x7b) )
    {
        //  Convert to a printable character.
        ch = 92;
    }
    ch = ch - 32;

    Start = &FontLookup[ch*4+ch];

#if defined(BIG)
    if ( (Big & (BIG_UP|BIG_DOWN)) == 0 )
#endif
    { 
      int i;
      for (i=0; i<=4; i++)
      {
        uint8_t E = Start[i];
        if ( Inverse )
        {
          E = ~E;
        }
        LcdSend( E, LCD_DATA );
      }
      if ( !Inverse )
        LcdSend( 0, LCD_DATA );
      else
        LcdSend( 0xFF, LCD_DATA );
    }
#if defined(BIG)
    else
    {
      int i;
      for (i=0; i<=4; i++)
      {
        uint8_t Element = 0;
        uint8_t E = Start[i];

        if ( Big & BIG_UP )
        {
          if(E & 0x01) Element |= 0x03;
          if(E & 0x02) Element |= 0x0C;
          if(E & 0x04) Element |= 0x30;
          if(E & 0x08) Element |= 0xC0;
        }
        else
        {
          if(E & 0x10) Element |= 0x03;
          if(E & 0x20) Element |= 0x0C;
          if(E & 0x40) Element |= 0x30;
          if(E & 0x80) Element |= 0xC0;
        }
        if (Inverse)
          Element = ~Element;
        LcdSend( Element, LCD_DATA );
        LcdSend( Element, LCD_DATA );
      } /* For */
      if ( !Inverse )
      {
        LcdSend( 0, LCD_DATA );
        LcdSend( 0, LCD_DATA );
      }
      else
      {
        LcdSend( 0xFF, LCD_DATA );
        LcdSend( 0xFF, LCD_DATA );
      }
    }
#endif /* BIG */
  }
}


/*--------------------------------------------------------------------------------------------------

  Name         :  LcdSend

  Description  :  Sends data to display controller.

  Argument(s)  :  data -> Data to be sent
                  cd   -> Command or data (see/use enum)

  Return value :  None.

--------------------------------------------------------------------------------------------------*/
void LcdSend ( uint8_t Data, LcdCmdData cd )
{
  int i;

  if ( cd != LCD_CMD ) /* If data */
  {
    GPIOB->BSRR = 1<<LCD_DC_PIN; /* DC pin to hight */
  }
  else
  {
    GPIOB->BRR = 1<<LCD_DC_PIN; /* DC pin to low */
  }
  LCD_DELAY;  
  i=8;
  do
  {
    if ( Data & 0x80 )
      GPIOB->BSRR = 1<<LCD_IN_PIN; /* IN pin to hight */
    else
      GPIOB->BRR  = 1<<LCD_IN_PIN; /* IN pin to low */
	LCD_DELAY;
    Data = Data<<1; /* Some additional delay */
    GPIOB->BSRR  = 1<<LCD_CLK_PIN; /* CLK pin to hight */
	LCD_DELAY;
	i--;
    GPIOB->BRR  = 1<<LCD_CLK_PIN; /* CLK pin to low */
  }while(i);
}

/*--------------------------------------------------------------------------------------------------
                                     Character generator

             This table defines the standard ASCII characters in a 5x7 dot format.
--------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------
                                         End of file.
--------------------------------------------------------------------------------------------------*/



#if defined(LCDDEBUG)
void SystemInit()
{
}

int main(void)
{
  RCC->CFGR |= (RCC_CFGR_HPRE_1|RCC_CFGR_HPRE_3); /* div 8 */
  LcdInit();
  LcdClear();
  LcdChr ( Y_POSITION*1+X_POSITION*1+13, "Hello world" );
  LcdChr ( Y_POSITION*2+X_POSITION*1+13+INVERSE+X_OFFSET*3, "Hello world" );
  LcdChr ( Y_POSITION*4+X_POSITION*0+2+BIG_UP, "15" );  
  LcdChr ( Y_POSITION*5+X_POSITION*0+2+BIG_DOWN, "15" );  
  return 0;
}
#endif
