#include <string.h>
#include "LabArm.h"

uint8_t MenuMode;
typedef struct
{
  uint32_t Adc;
  int16_t Dac; /* 1 - 4095 */
  int16_t Out; /* 1 - 9999 */
  int8_t DotPosition; /* 0,1,2,3,4 */
  int8_t Ok; /* Fiilled flag */
  int16_t Res;
}Calibration_t;



Calibration_t VMax;
Calibration_t VMin;
Calibration_t IMax;
Calibration_t IMin;
static Calibration_t* Cur;
static char* Text;
__IO static uint32_t* CurDac;
uint8_t  Contrast;
MenuFunction_t  AfterContrast;

void ContrastMenu(void)
{
  if (Event == EV_FUNC_FIRST)
  {
//    Contrast = CurrentSettings.VoltageDAC>>12;
    LcdBlank(); /* Clear screen */
    LcdChr(Y_POSITION*1 + X_POSITION * 0 + 14, "Contrast");
    LcdChr(Y_POSITION*4 + X_POSITION * 0 + 14, "Press");
    LcdChr(Y_POSITION*5 + X_POSITION * 0 + 14, "to exit");
    goto redraw;
  }
  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        if ( Contrast < 15 )
          Contrast++;
        break;
      case KEY_DOWN:
        if ( Contrast > 0 )
          Contrast--;
        break;
      case KEY_ENTER:
        LcdContrast ( Contrast*3+42 );
        CurrentFunc(AfterContrast);
         return;
    }
redraw:
    LcdContrast ( Contrast*3+42 );
    {
      char Buf[2];
      uint8_t Counter = Contrast;
      
      Buf[0] = Counter/10 + '0';
      Counter = Counter%10;
      Buf[1] = Counter + '0';
      LcdChr(BIG_UP+Y_POSITION*2 + X_POSITION * 2 + 2, Buf);
      LcdChr(BIG_DOWN+Y_POSITION*3 + X_POSITION * 2 + 2, Buf);
    }
  }
}


static float Offset(float YMax, float YMin, float XMax, float XMin)
{
  return (YMin*XMax - YMax*XMin)/(XMax-XMin);
}
static void CalibrationToConfig(Config_t* Config)
{
  int i;

  Config->V.ADCRamp = ((float)(VMax.Out - VMin.Out))/(VMax.Adc-VMin.Adc);
  Config->V.ADCOffset = Offset(VMax.Out, VMin.Out, VMax.Adc, VMin.Adc);
  Config->I.ADCRamp = ((float)(IMax.Out - IMin.Out))/(IMax.Adc-IMin.Adc);
  Config->I.ADCOffset = Offset(IMax.Out, IMin.Out, IMax.Adc, IMin.Adc);
  Config->V.DACRamp = ((float)(VMax.Out - VMin.Out))/(VMax.Dac-VMin.Dac);
  Config->V.DACOffset = Offset(VMax.Out, VMin.Out, VMax.Dac, VMin.Dac);
  Config->I.DACRamp = ((float)(IMax.Out - IMin.Out))/(IMax.Dac-IMin.Dac);
  Config->I.DACOffset = Offset(IMax.Out,IMin.Out,IMax.Dac, IMin.Dac);
  Config->V.DotPosition = VMax.DotPosition;
  Config->I.DotPosition = IMax.DotPosition;
  
  /* Check menu correctness */
  for (i=0; i<12; i++)
  {
    UserMenu_t* Menu = &CurrentConfig.Menu[i];
    if ( (Menu->IVFlag == MENU_I_FLAG && IMax.DotPosition != Menu->DotPosition ) ||
         (Menu->IVFlag == MENU_V_FLAG && VMax.DotPosition != Menu->DotPosition ) ) /* Dot position was changed */
    {
      memset(Menu, 0xFF, sizeof(*Menu)); /* Clear the menu */
    }
  }
}


void OutValue(uint8_t Y, uint8_t X, uint16_t Num, uint8_t DotPosition, uint8_t SelectPos)
{
  int i;
  int Div = 1000;
  uint8_t DisplayFlag = 0;
  
  for(i=0; i<4; i++)
  {
    char Chr;
    uint32_t Light = 0;

    if (i == DotPosition )
    {
      LcdChr ( Y_POSITION*(Y)+X_POSITION*X+1, " " );
      LcdChr ( Y_POSITION*(Y+1)+X_POSITION*X+1, "." );
      X=X+1;
      DisplayFlag++;
    }

    if ( DisplayFlag == 0 && i == (DotPosition - 1))
    {
      DisplayFlag++;
    }

    Chr = Num / Div;
    if ( DisplayFlag == 0 && Chr == 0)
      Chr = ' ';
    else
    {
      DisplayFlag = 1;
      Chr = Chr + '0';
    }
    if (i == SelectPos)
    {
      Light = INVERSE;
    } 
    LcdChr (Light + Y_POSITION*Y+X_POSITION*X+BIG_UP+1, &Chr );
    LcdChr (Light + Y_POSITION*(Y+1)+X_POSITION*X+BIG_DOWN+1, &Chr );
    X = X + 2;
    Num = Num % Div;
    Div = Div / 10;
  }
  if ( DotPosition == 4)
  {
    LcdChr ( Y_POSITION*(Y)+X_POSITION*X+1, " " );
    LcdChr ( Y_POSITION*(Y+1)+X_POSITION*X+1, " " );
  }
}

void OutValueSmall(uint8_t Y, uint8_t X, uint16_t Num, uint8_t DotPosition, uint8_t InverseFlag)
{
  int i;
  int Div = 1000;
  uint32_t Light = InverseFlag?INVERSE:0;
  uint8_t DisplayFlag=0;
  
  for(i=0; i<4; i++)
  {
    char Chr;

    if (i == DotPosition)
    {
      DisplayFlag = 1;
      LcdChr ( Light + Y_POSITION*(Y)+X_POSITION*X+1, "." );
      X=X+1;     
      DisplayFlag++;
    }

    if ( DisplayFlag == 0 && i == (DotPosition - 1))
    {
      DisplayFlag++;
    }

    Chr = Num / Div;
    if ( DisplayFlag == 0 && Chr == 0)
      Chr = ' ';
    else
    {
      DisplayFlag = 1;
      Chr = Chr + '0';
    }
    LcdChr (Light + Y_POSITION*Y+X_POSITION*X+1, &Chr );
    X = X + 1;
    Num = Num % Div;
    Div = Div / 10;
  }
  if ( DotPosition == 4)
  {
    LcdChr ( Y_POSITION*(Y)+X_POSITION*X+1, " " );
  }
}


void CalibrationMenuInternal(void);

void SaveMenu(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    MenuMode = 0;
    LcdBlank(); /* Clear screen */
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        if ( MenuMode == 0 )
          MenuMode = 1;
        break;
      case KEY_DOWN:
        if ( MenuMode != 0 )
          MenuMode = 0;
        break;
      case KEY_ENTER:
        if (MenuMode != 0)
        {
          SaveConfig();
        }
        CurrentFunc(StartFunction);
        return;
    }
  }
  else
    return;

redraw:
  LcdChr(X_POSITION*0+Y_POSITION*1+14 + (0==MenuMode)*INVERSE, "Save to memory");
  LcdChr(X_POSITION*0+Y_POSITION*2+14 + (1==MenuMode)*INVERSE, "Save to flash");
}


void SetDac(void);

void SelectDot(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        Cur->DotPosition++;
        if (Cur->DotPosition>4)
          Cur->DotPosition = 4;
        goto redraw;
      case KEY_DOWN:
        Cur->DotPosition--;
        if (Cur->DotPosition<0)
          Cur->DotPosition = 0;
        goto redraw;
      case KEY_ENTER:
        CurrentFunc(SetDac);
    }
  }

  return;
redraw:
  LcdChr(X_POSITION*0+Y_POSITION*1+14, Text);
  LcdChr(X_POSITION*0+Y_POSITION*2+14, "Set Dot");
  OutValue(3, 1, Cur->Out, Cur->DotPosition, 0xFF);
}

int16_t IncrementMul;
int8_t  ChangeNumberFlag;
void SetOut(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuMode = 0;
    IncrementMul = 1000;
    ChangeNumberFlag = 0;
    LcdChr(X_POSITION*0+Y_POSITION*1+14, Text);
    LcdChr(X_POSITION*0+Y_POSITION*2+14, "Set Out");
    LcdChr(X_POSITION*0+Y_POSITION*5+14, "Longkey to end");
    goto redraw;
  }
  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        if ( ChangeNumberFlag )
        {
          Cur->Out += IncrementMul;
          goto redraw;
        }
        else
        {
          if ( MenuMode < 3 )
          {
            IncrementMul /= 10;
            MenuMode++;
            goto redraw;
          }
        }
        return;
      case KEY_DOWN:
        if ( ChangeNumberFlag )
        {
          Cur->Out -= IncrementMul;
          goto redraw;
        }
        else
        {
          if ( MenuMode > 0 )
          {
            IncrementMul *= 10;
            MenuMode--;
            goto redraw;
          }
        }
        return;
      case KEY_ENTER:
        if ( ChangeNumberFlag == 0 )
          ChangeNumberFlag = 1;
        else
          ChangeNumberFlag = 0;
        return;
    }
  }
  if ( (Event & EV_MASK) == EV_KEY_LONG )
  {
    Cur->Ok = 1;
    CurrentFunc(CalibrationMenuInternal);
  }

  return;

redraw:
  OutValue(3, 1, Cur->Out, Cur->DotPosition, MenuMode);
}

void WaitMeasure(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    LcdChr(X_POSITION*0+Y_POSITION*3+14, "Please wait");
    MenuMode = 0;
    return;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED &&
       (Event & KEY_MASK) == KEY_ADC ) /* End of conversion */
  {
    if (MenuMode == 2)
    {
      CurrentFunc(SetOut);
      if ( Cur == &VMax || Cur == &VMin )
        Cur->Adc = ADCVoltage;
      else
        Cur->Adc = ADCCurrent;
    }
    MenuMode++;
  }
}

void SetDac(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    LcdChr(X_POSITION*0+Y_POSITION*1+14, Text);
    LcdChr(X_POSITION*0+Y_POSITION*2+14, "Set Out");
    EncCounter = Cur->Dac;
    goto redraw;
  }
  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
      case KEY_DOWN:
        Cur->Dac = EncCounter;
        goto redraw;
   case KEY_ENTER:
        CurrentFunc(WaitMeasure);
    }
  }

  return;
redraw:
  OutValue(3, 1, Cur->Dac, 4,  0xFF);
  *CurDac = Cur->Dac;
  OutValueSmall(5,0, EncStep, 4, 0);
}


void CalibrationMenu(void)
{
  /* Init The calibration */
  VMax.Ok = 0;
  VMin.Ok = 0;
  IMax.Ok = 0;
  IMin.Ok = 0;
  CurrentFunc(CalibrationMenuInternal);
}

void CalibrationMenuInternal(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    MenuMode = 0;
    LcdBlank(); /* Clear screen */
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_DOWN:
        if ( MenuMode == 0 )
          MenuMode = 4;
        else
          MenuMode = MenuMode - 1;
        goto redraw;
      case KEY_UP:
        if ( MenuMode == 4 )
          MenuMode = 0;
        else
          MenuMode = MenuMode + 1;
        goto redraw;
      case KEY_ENTER:
        switch(MenuMode)
        {
          case 0:
            Cur = &VMax;
            Text = "Max Voltage";
            Cur->Dac = 3700;
            Cur->Out = 5555;
            Cur->DotPosition = 2;
            CurDac = &DAC_V;
            CurrentFunc(SelectDot);
            return;
          case 1:
            if (VMax.Ok == 0 ) /* The max value has not been set */
              return;
            Cur = &VMin;
            Text = "Min Voltage";
            Cur->Dac = 350;
            Cur->Out = 555;
            Cur->DotPosition = VMax.DotPosition;
            CurDac = &DAC_V;
            CurrentFunc(SetDac);
            return;
          case 2:
            Cur = &IMax;
            Text = "Max Current";
            Cur->Dac = 3700;
            Cur->Out = 5555;
            Cur->DotPosition = 2;
            CurDac = &DAC_I;
            DAC_V = 2000; /* Set intermidiate voltage value !!! */
            CurrentFunc(SelectDot);
            return;
          case 3:
            if (IMax.Ok == 0 ) /* The max value has not been set */
              return;
            Cur = &IMin;
            Text = "Min Current";
            Cur->Dac = 350;
            Cur->Out = 555;
            Cur->DotPosition = IMax.DotPosition;
            CurDac = &DAC_I;
            CurrentFunc(SetDac);
            return;
          default:
            if ( VMax.Ok && VMin.Ok && IMax.Ok && IMin.Ok )
            {
              CalibrationToConfig(&CurrentConfig);
              CurrentFunc(SaveMenu);
            }
            return;
        }
    }
  }
  return;
  
redraw:
  LcdChr(X_POSITION*0+Y_POSITION*1+14 + (0==MenuMode)*INVERSE, "Max Voltage");
  LcdChr(X_POSITION*0+Y_POSITION*2+14 + (1==MenuMode)*INVERSE, "Min Voltage");
  LcdChr(X_POSITION*0+Y_POSITION*3+14 + (2==MenuMode)*INVERSE, "Max Current");
  LcdChr(X_POSITION*0+Y_POSITION*4+14 + (3==MenuMode)*INVERSE, "Min Current");
  if ( VMax.Ok && VMin.Ok && IMax.Ok && IMin.Ok )
  {
    LcdChr(X_POSITION*0+Y_POSITION*5+14 + (4==MenuMode)*INVERSE, "Ok");
  }
  else
  {
    LcdChr(X_POSITION*0+Y_POSITION*5+14 + (4==MenuMode)*INVERSE, "");
  }
}
