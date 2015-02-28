#include <string.h>
#include "LabArm.h"

Config_t CurrentConfig;
Settings_t CurrentSettings;
#if defined(RAW)
uint8_t DisplayRaw;
#endif
uint8_t MenuPos;
uint16_t Flags;

void ApplyOut(void);
void PredefinedMenu(void);
void Configure(void);


static UserMenu_t* Menu;

static CancelButton(void)
{
  if (!(Flags & (AUTO_APPLY_FLAG|IMEDDIATE_APPLY_FLAG)) && /* New values are set but not applied */
       (DAC->DOR1 != 0 && DAC->DOR2 !=0 )) /* Only if the output is enabled 05.10.11 */
  {
    CurrentSettings.VoltageDAC = DAC->DOR1;
    CurrentSettings.CurrentDAC = DAC->DOR2;
  }
  CurrentFunc(StartFunction);
  return;
}

void HandleUserMenu(void)
{
  int NewValue;
  SubConfig_t* SubConfig;

  if ( Menu->IVFlag == MENU_V_FLAG)
    SubConfig = &CurrentConfig.V;
  else
    SubConfig = &CurrentConfig.I;

  NewValue = (Menu->Value - SubConfig->DACOffset)/ SubConfig->DACRamp;
  if ( NewValue < 0 )
    NewValue = 0;
  if ( NewValue > 4095 )
    NewValue = 4095;
  if ( Menu->IVFlag == MENU_V_FLAG)
    CurrentSettings.VoltageDAC = NewValue;
  else
    CurrentSettings.CurrentDAC = NewValue;
  if ( Flags & (AUTO_APPLY_FLAG|IMEDDIATE_APPLY_FLAG) )
    ApplyOut();
  else
    CurrentFunc(StartFunction);
}

#define MENU_0       0
#define MENU_1       1
#define MENU_2       2
#define MENU_3       3
#define MENU_4       4
#define MENU_5       5
#define MENU_6       6
#define MENU_CNCL_RET 7
#define MENU_7       8
#define MENU_8       9
#define MENU_9       10
#define MENU_10      11
#define MENU_11      12
#define MENU_12      13
#define MENU_13      14

static void DisplayMenu(int IsSetMenu)
{
  int i;
  char* Text;

  for (i=MENU_0; i<=MENU_13;i++)
  {
    int x = i/5 * 5;
    int y = i%5 + 1;
    int TextPos = (i< MENU_7)?i:i-1;
    Text = CurrentConfig.Menu[TextPos].Text;
    LcdChr(X_POSITION*x+Y_POSITION*y+4 + (i==MenuPos)*INVERSE, Text);
  }

  if (!IsSetMenu)
    Text = "Cncl";
  else
    Text = "Retu";
  LcdChr(X_POSITION*5+Y_POSITION*3+4 + (MENU_CNCL_RET==MenuPos)*INVERSE, Text);
}

void ConfigureTheMenu(void);

void UConfigureTheMenu()
{
  char* Text;
  SubConfig_t* Config;

  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = 0;

    if (Menu->IVFlag != MENU_I_FLAG) /* It can be polluted */
      Menu->IVFlag = MENU_V_FLAG;
    else
      Menu->IVFlag = MENU_I_FLAG;

    if (Menu->Text[0] == 0)
      Menu->Text[0] = '0';
    if (Menu->Text[1] == 0)
      Menu->Text[1] = '1';
    if (Menu->Text[2] == 0)
      Menu->Text[2] = '2';
    if (Menu->Text[3] == 0)
      Menu->Text[3] = '3';
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
      case KEY_DOWN:
        switch(MenuPos)
        {
          case 0:
            if (Menu->IVFlag & MENU_I_FLAG)
              Menu->IVFlag = MENU_V_FLAG;
            else
              Menu->IVFlag = MENU_I_FLAG;
            goto redraw;
          case 1:
            if (EncCounter > 999 )
              EncCounter = 999;
            Menu->Value = EncCounter*10 + 1; /* it's better to see changes from 9001 to 9000 that 9000 to 8999 */
            goto redraw;
          default:
            if (EncCounter > 'z')
              EncCounter = 'z';
            Menu->Text[MenuPos-2] = EncCounter;
            goto redraw;
        }
      case KEY_ENTER:
        switch(MenuPos)
        {
          case 0:
            if (Menu->IVFlag & MENU_V_FLAG)
              Menu->IVFlag = MENU_V_FLAG;
            else
              Menu->IVFlag = MENU_I_FLAG;
            EncCounter = Menu->Value;
            break;
          default:
            EncCounter = Menu->Text[MenuPos - 1];
        }
        MenuPos++;
        if (MenuPos == 6)
          MenuPos = 0;
        goto redraw;
    }
  }
  if ( (Event & EV_MASK) == EV_KEY_LONG &&
       (Event & KEY_MASK) == KEY_ENTER )
  {
    CurrentFunc(ConfigureTheMenu);
  }
  return;

redraw:
  if (Menu->IVFlag != MENU_I_FLAG) /* Voltage */
  {
    Text = " Voltage";
    Config = &CurrentConfig.V; 
  }
  else
  {
    Text = " Current";
    Config = &CurrentConfig.I; 
  }
  LcdChr(X_POSITION*0+Y_POSITION*1+14 + (0==MenuPos)*INVERSE, Text);
  OutValueSmall(3, 3, Menu->Value, Config->DotPosition, MenuPos == 1);
  LcdChr(X_POSITION*2+Y_POSITION*5+1 + (2==MenuPos)*INVERSE, &Menu->Text[0]);
  LcdChr(X_POSITION*3+Y_POSITION*5+1 + (3==MenuPos)*INVERSE, &Menu->Text[1]);
  LcdChr(X_POSITION*4+Y_POSITION*5+1 + (4==MenuPos)*INVERSE, &Menu->Text[2]);
  LcdChr(X_POSITION*5+Y_POSITION*5+1 + (5==MenuPos)*INVERSE, &Menu->Text[3]);
}


void YesNoCalib(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = 0;
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
      case KEY_DOWN:
        if(MenuPos == 0)
          MenuPos++;
        else
          MenuPos = 0;
        goto redraw;
      case KEY_ENTER:
        if(MenuPos)
          CurrentFunc(CalibrationMenu);
        else
          CurrentFunc(Configure);
    }
  }
  return;
redraw:
  LcdChr(X_POSITION*0+Y_POSITION*1+14, "Do you want");
  LcdChr(X_POSITION*0+Y_POSITION*2+14, "to start the");
  LcdChr(X_POSITION*0+Y_POSITION*3+14, "calibration");
  LcdChr(X_POSITION*0+Y_POSITION*5+7 + (0==MenuPos)*INVERSE,  "No");
  LcdChr(X_POSITION*7+Y_POSITION*5+7 + (0!=MenuPos)*INVERSE,  "Yes");
}

#define  CONF_MENU_EDIT 0
#define  CONF_CALIBR    1
#define  CONF_BACKUP    2
#define  CONF_CLOCK     3
#define  CONF_CONTRAST  4

void Configure(void)
{
 if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = CONF_CONTRAST;
    goto redraw;
  } 

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
       case KEY_DOWN:
        if ( MenuPos == 0 )
          MenuPos = 4;
        else
          MenuPos = MenuPos - 1;
        goto redraw;
      case KEY_UP:
        if ( MenuPos == 4 )
          MenuPos = 0;
        else
          MenuPos = MenuPos + 1;
        goto redraw;
      case KEY_ENTER:
        switch(MenuPos)
        {
          case CONF_MENU_EDIT: /* Menu set up */
            CurrentFunc(ConfigureTheMenu);
            return;
          case CONF_CALIBR: /* Calibration */
            CurrentFunc(YesNoCalib);
            return;
          case CONF_BACKUP: /* Save to backup */
            if ( Flags&BACKUP_SAVE_FLAG )
              Flags &= ~BACKUP_SAVE_FLAG;
            else
              Flags |= BACKUP_SAVE_FLAG;
            goto redraw;
          case CONF_CLOCK: /* Time */
#if defined(CLOCK_ENABLED)
            if (IS_ON_CLOCK)
            {
              CurrentFunc(SetupTheClock);
              return;
            }
            else
            {
              SwitchOnTheClock();
            }
#endif
            goto redraw;
          case CONF_CONTRAST:
            AfterContrast = StartFunction;
            CurrentFunc(ContrastMenu);
        }
    }
  }
  return;   

redraw:
  LcdChr (Y_POSITION*1+X_POSITION*0+14+(CONF_MENU_EDIT==MenuPos)*INVERSE, "Menu Edit" );
  LcdChr (Y_POSITION*2+X_POSITION*0+14+(CONF_CALIBR==MenuPos)*INVERSE, "Calibration" );
  LcdChr (Y_POSITION*3+X_POSITION*1+14+(CONF_BACKUP==MenuPos)*INVERSE, "Save Backup");
  LcdChr (Y_POSITION*3+X_POSITION*0+1+ (CONF_BACKUP==MenuPos)*INVERSE, Flags&BACKUP_SAVE_FLAG?"+":"-" );
  LcdChr (Y_POSITION*4+X_POSITION*0+14+(CONF_CLOCK==MenuPos)*INVERSE, 
#if defined(CLOCK_ENABLED)
                              IS_ON_CLOCK?"Setup time":"Time on" 
#else
                              "Reserved"
#endif  
                                                       );
  LcdChr (Y_POSITION*5+X_POSITION*0+14+(CONF_CONTRAST==MenuPos)*INVERSE, "Contrast" );

}

void ConfigureTheMenu(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = MENU_CNCL_RET;
    goto redraw;
  } 
  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_DOWN:
        if ( MenuPos == 0 )
          MenuPos = 14;
        else
          MenuPos = MenuPos - 1;
        goto redraw;
      case KEY_UP:
        if ( MenuPos == 14 )
          MenuPos = 0;
        else
          MenuPos = MenuPos + 1;
        goto redraw;
      case KEY_ENTER:
        switch(MenuPos)
        {
          case MENU_0:
          case MENU_1:
          case MENU_2:
          case MENU_3:
          case MENU_4:
          case MENU_5:
          case MENU_6:
            Menu=&CurrentConfig.Menu[MenuPos];
            goto usermenu;
          case MENU_7:
          case MENU_8:
          case MENU_9:
          case MENU_10:
          case MENU_11:
          case MENU_12:
          case MENU_13:
            Menu=&CurrentConfig.Menu[MenuPos - 1];
            goto usermenu;
          default:
            CurrentFunc(SaveMenu);
            return;
        } /* switch MenuPos*/
    } /* switch key */
  }
  return;
usermenu:
  CurrentFunc(UConfigureTheMenu);

  return;
redraw:
  DisplayMenu(1);
}

void PredefinedMenu(void)
{
 /* 1    6    10 */
 /* 2    7    11  */
 /* 3    cncl 12 */
 /* 4    8    13  */
 /* 5    9    14  */

  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = MENU_CNCL_RET;
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_DOWN:
        if ( MenuPos == 0 )
          MenuPos = 14;
        else
          MenuPos = MenuPos - 1;
        goto redraw;
      case KEY_UP:
        if ( MenuPos == 14 )
          MenuPos = 0;
        else
          MenuPos = MenuPos + 1;
        goto redraw;
      case KEY_ENTER:
        switch(MenuPos)
        {
          case MENU_CNCL_RET: /* Default cancel input*/
            CancelButton();
            return;
          case MENU_0:
          case MENU_1:
          case MENU_2:
          case MENU_3:
          case MENU_4:
          case MENU_5:
          case MENU_6:
            Menu=&CurrentConfig.Menu[MenuPos];
            goto usermenu;
          case MENU_7:
          case MENU_8:
          case MENU_9:
          case MENU_10:
          case MENU_11:
          case MENU_12:
          case MENU_13:
            Menu=&CurrentConfig.Menu[MenuPos-1];
            goto usermenu;
        } /* switch MenuPos*/
    } /* switch key */
  }
  return;
usermenu:
  if ( (Menu->IVFlag != MENU_I_FLAG) &&
     (Menu->IVFlag != MENU_V_FLAG))  /* Do nothing - invalid menu */
    return;
  CurrentFunc(HandleUserMenu);
  return;
redraw:
  DisplayMenu(0);
}


void ApplyOut(void)
{
  int Delta;

  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    goto redraw;
  }
  if ( CurrentSettings.VoltageDAC == DAC->DOR1 &&
          CurrentSettings.CurrentDAC == DAC->DOR2 )
  {
     CurrentFunc(StartFunction);
     return;
  }

  Delta = (int)CurrentSettings.VoltageDAC - (int)(DAC->DOR1);
  if ( Delta > 50 )
    Delta = 50;
  if ( Delta < -50)
    Delta = -50;
  DAC_V = (int)(DAC->DOR1) + Delta;

  Delta = (int)CurrentSettings.CurrentDAC - (int)(DAC->DOR2);
  if ( Delta > 50 )
    Delta = 50;
  if ( Delta < -50)
    Delta = -50;
  DAC_I = (int)(DAC->DOR2) + Delta;

  OutValueSmall(2, 9, HumanV, CurrentConfig.V.DotPosition, 0);
  OutValueSmall(5, 9, HumanI, CurrentConfig.I.DotPosition, 0);
  return;

redraw:
  LcdChr(Y_POSITION*1+X_POSITION*0+14, " Applying...");
  OutValue(2, 0, VoltageFromDac(), CurrentConfig.V.DotPosition, 0xFF);
  OutValue(4, 0, VoltageFromDac(), CurrentConfig.V.DotPosition, 0xFF);
}

uint32_t OriginalValue;
void SetNewCurrent(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    LcdChr ( Y_POSITION*1+X_POSITION*0+14, "New current" );

    EncCounter = CurrentSettings.CurrentDAC;
    OriginalValue = CurrentSettings.CurrentDAC;
    goto redraw;
  } 

  if ( (Event& EV_MASK) != EV_KEY_PRESSED )
    return;

  switch (Event & KEY_MASK)
  {
    case KEY_UP:
    case KEY_DOWN:
      CurrentSettings.CurrentDAC = EncCounter;
	  if (Flags&IMEDDIATE_APPLY_FLAG)
	  DAC_I	= EncCounter;
      goto redraw;
    case KEY_ENTER:
      if ( CurrentSettings.CurrentDAC != OriginalValue ) /* Current is changed */
      {
          if ( (Flags&(AUTO_APPLY_FLAG|IMEDDIATE_APPLY_FLAG)) )
            CurrentFunc(ApplyOut);
          else
            CurrentFunc(StartFunction);
      }
      else
        CurrentFunc(StartFunction);
  }
  return;
redraw:
  OutValue(3,2, CurrentFromDac(), CurrentConfig.I.DotPosition, 0xFF);
  OutValueSmall(5, 0, HumanI, CurrentConfig.I.DotPosition, 0);
}

void SetNewVoltage(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    LcdChr ( Y_POSITION*1+X_POSITION*0+14, "New voltage" );

    EncCounter = CurrentSettings.VoltageDAC;
    OriginalValue = CurrentSettings.VoltageDAC;
    goto redraw;
  } 

  if ( (Event& EV_MASK) != EV_KEY_PRESSED )
    return;

  switch (Event & KEY_MASK)
  {
    case KEY_UP:
    case KEY_DOWN:
      CurrentSettings.VoltageDAC = EncCounter;
	  if (Flags&IMEDDIATE_APPLY_FLAG)
	  DAC_V	= EncCounter;
      goto redraw;
    case KEY_ENTER:
      if ( CurrentSettings.VoltageDAC != OriginalValue ) /* Voltage is changed */
      {
          if ( (Flags&(AUTO_APPLY_FLAG|IMEDDIATE_APPLY_FLAG)) )
            CurrentFunc(ApplyOut);
          else
            CurrentFunc(StartFunction);
      }
      else
        CurrentFunc(SetNewCurrent);
  }
  return;
redraw:
  OutValue(3,2, VoltageFromDac(), CurrentConfig.V.DotPosition, 0xFF);
  OutValueSmall(5, 0, HumanV, CurrentConfig.V.DotPosition, 0);
}

void SmallSettings(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = 2; /* Cancel */
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        MenuPos++;
        if ( MenuPos > 5 )
          MenuPos = 0;
        goto redraw;
      case KEY_DOWN:
        if ( MenuPos == 0 )
          MenuPos = 5;
        else
          MenuPos--;
        goto redraw;
      case KEY_ENTER:
        switch(MenuPos)
        {
          case 0: /* Configuration */
            CurrentFunc(Configure);
            return;
          case 1:
            if (Flags&AUTO_APPLY_FLAG)
            {
              Flags &= ~AUTO_APPLY_FLAG;
              Flags |= IMEDDIATE_APPLY_FLAG;
            }
            else if (Flags & IMEDDIATE_APPLY_FLAG)
            {
              Flags &= ~(AUTO_APPLY_FLAG|IMEDDIATE_APPLY_FLAG);
            }
            else
            {
              Flags |= AUTO_APPLY_FLAG;
            }
            goto redraw;
          case 2: /* Cancel */
            CancelButton();
            return;
          case 3: /* Current graph */
#if defined(GRAPH)
            GraphData.GraphArray = IGraphArray;
            GraphData.DotPosition = CurrentConfig.I.DotPosition;
            CurrentFunc(DisplayGraph);
#endif
            return;
          case 4: /* Voltage graph */
#if defined(GRAPH)
            GraphData.GraphArray = VGraphArray;
            GraphData.DotPosition = CurrentConfig.V.DotPosition;
            CurrentFunc(DisplayGraph);
            return;
#endif
          case 5: /* Timer set up */
#if defined(CLOCK_ENABLED)
            if (IS_ON_CLOCK)
              CurrentFunc(TimerSetup);
#endif
            return;

        } /* switch MenuPos*/
    } /* switch key */
  }
  return;
redraw:
  LcdChr(14+1*Y_POSITION+0*X_POSITION + (MenuPos==0?INVERSE:0), "Settings");
  LcdChr(
#if !defined(CLOCK_ENABLED)
  14
#else
  5
#endif  
     + 2*Y_POSITION+0*X_POSITION + (MenuPos==1?INVERSE:0), 
    ( Flags & AUTO_APPLY_FLAG )?
	      "Auto":
	  ( (Flags & IMEDDIATE_APPLY_FLAG)? "Immd":"Manu"));
  LcdChr(14+3*Y_POSITION+0*X_POSITION + (MenuPos==2?INVERSE:0), "Cancel");
#if defined(GRAPH)
  LcdChr(14+4*Y_POSITION+0*X_POSITION + (MenuPos==3?INVERSE:0), "I Graph");
  LcdChr(14+5*Y_POSITION+0*X_POSITION + (MenuPos==4?INVERSE:0), "V Graph");
#else
  LcdChr(14+4*Y_POSITION+0*X_POSITION + (MenuPos==3?INVERSE:0), "Reserved");
  LcdChr(14+5*Y_POSITION+0*X_POSITION + (MenuPos==4?INVERSE:0), "Reserved");
#endif
#if defined(CLOCK_ENABLED)
  LcdChr(8+2*Y_POSITION+6*X_POSITION + (MenuPos==5?INVERSE:0), (IS_ON_CLOCK)? "Timer":"");
#endif
}

#if defined(CLOCK_ENABLED)
static void ApplyWithDelay(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
  }

  if ( (Event& EV_MASK) == EV_KEY_PRESSED  && 
       (Event & KEY_MASK) == KEY_CLOCK )
  {
    RemainTimerValue = TimerValue;
    CurrentFunc(ApplyOut);
  }
}
#endif

void StartFunction(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = 0; /* There are no key ENTER pressed */
    goto redraw;
  }

  if ( (Event& EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_DOWN:
        CurrentFunc(SmallSettings);
        return;
      case KEY_UP:
        CurrentFunc(PredefinedMenu);
        return;
      case KEY_ADC:
        goto redraw;
#if defined(CLOCK_ENABLED)
      case KEY_CLOCK:
      {
        if ( TimerValue != 0 && RemainTimerValue != 0xFFFF )
        {
          DisplayClock(RemainTimerValue, 10+0x8000, Y_POSITION*3+X_POSITION*6);
        }
        else
        {
          LcdChr(Y_POSITION*3+X_POSITION*6+3, "" );
          DisplayClock((RTC->CNTH<<16) + RTC->CNTL, 10, Y_POSITION*3+X_POSITION*9);
        }
        return;
      }
#endif        
      case KEY_ENTER:/* Reserve current */
      {
        MenuPos = 1; /* Key ENTER was pressed */
        goto redraw;
      }
    }
  }

  if ( (Event & EV_MASK) == EV_KEY_LONG ) 
  {
    if ( CurrentSettings.VoltageDAC == DAC->DOR1 &&
         CurrentSettings.CurrentDAC == DAC->DOR2 )
    {
      /* Long key set OFF output */
      DAC_V = 0;
      DAC_I = 0;
      MenuPos = 0; /* It is long press. The event was handled. No need do anything on key realized */
#if defined(CLOCK_ENABLED)
      RemainTimerValue = 0;
#endif
      return;
    }

    /* Apply seved or set voltage and current */
    MenuPos = 0;
#if defined(CLOCK_ENABLED)
    if (TimerValue != 0)
    {
      CurrentFunc(ApplyWithDelay);
    }
    else
#endif
    {
      CurrentFunc(ApplyOut);
    }
    return;   
  }
  

  if ( (Event& EV_MASK) == EV_KEY_REALIZED ) /* realised event current */
  {
    if (MenuPos) /* No long press */
      CurrentFunc(SetNewVoltage);
    return;
  }

  return;
  
redraw:
#if defined(RAW)
  if ( DisplayRaw == 0)
#endif
  {
    OutValue(1, 0, HumanV, CurrentConfig.V.DotPosition, 0xFF);
    OutValue(4, 0, HumanI, CurrentConfig.I.DotPosition, 0xFF);
    OutValueSmall(1, 9, VoltageFromDac(), 
                  CurrentConfig.V.DotPosition, CurrentSettings.VoltageDAC != DAC->DOR1);
    OutValueSmall(5, 9, CurrentFromDac(), 
                  CurrentConfig.I.DotPosition, CurrentSettings.CurrentDAC != DAC->DOR2);
  }
#if defined(RAW)
  else
  {
    OutValue(1, 0, ADCVoltage/(128*16), 4, 0xFF);
    OutValue(4, 0, ADCCurrent/(128*16), 4, 0xFF);
    OutValueSmall(1, 9, CurrentSettings.VoltageDAC, 4, CurrentSettings.VoltageDAC != DAC->DOR1);
    OutValueSmall(5, 9, CurrentSettings.CurrentDAC, 4, CurrentSettings.CurrentDAC != DAC->DOR2);
  }
#endif
}

int16_t VoltageFromAdc(void)
{
  return ADCVoltage*CurrentConfig.V.ADCRamp + CurrentConfig.V.ADCOffset;
}

int16_t CurrentFromAdc(void)
{
  return ADCCurrent*CurrentConfig.I.ADCRamp + CurrentConfig.I.ADCOffset;
}

int16_t CurrentFromDac()
{
  return CurrentSettings.CurrentDAC*CurrentConfig.I.DACRamp + CurrentConfig.I.DACOffset;
}
int16_t VoltageFromDac()
{
  return CurrentSettings.VoltageDAC*CurrentConfig.V.DACRamp + CurrentConfig.V.DACOffset;
}
