#if defined(CLOCK_ENABLED)
#include "LabArm.h"

#define CLOCK_DIV (0x8000 - 2)

void DisplayClock(uint32_t RTCVal, uint16_t InvPos, uint32_t Position)
{
  int Sec = RTCVal % (24*60*60); /* Seconds in the day */
  int Value;
  char Ch;
  
  if (Sec % 2 )
    Ch = ' ';
  else
    Ch = ':';
  LcdChr(Position + X_POSITION*2 + 1, &Ch );

  if ( ((int16_t)InvPos) < 0 )
  {
    LcdChr(Position + X_POSITION*5 + 1, &Ch );
    Value = Sec % 60; /* Seconds */
    Ch = Value/10 + '0';
    LcdChr( Position + X_POSITION*6 + 1, &Ch );
    Ch = Value%10 + '0';
    LcdChr( Position + X_POSITION*7 + 1, &Ch );
  }
  InvPos &= ~0x8000;
  Value = Sec/(60*60); /* Hour */
  Ch = Value/10 + '0';
  LcdChr( Position + 1 + ((InvPos==0)?INVERSE:0), &Ch );
  Ch = Value%10 + '0';
  LcdChr( Position + X_POSITION*1 + 1 +((InvPos==1)?INVERSE:0), &Ch );
  Sec = Sec%(60*60); /* Sec in the hour*/
  Value = Sec/60; /* Minutes */
  Ch = Value/10 + '0';
  LcdChr( Position + X_POSITION*3 + 1 + ((InvPos==2)?INVERSE:0), &Ch );
  Ch = Value%10 + '0';
  LcdChr( Position + X_POSITION*4 + 1 + ((InvPos==3)?INVERSE:0), &Ch );
  return;
}


int SwitchOnTheClock(void)
{
  if ( (RCC->BDCR & RCC_BDCR_RTCSEL) != RCC_BDCR_RTCSEL_0  ) /* RTC clock is off */
  {
    GPIOC->CRH |= GPIO_CRH_MODE14_1 |GPIO_CRH_MODE15_1; /* Out 2 MHZ*/
    GPIOC->CRH &= ~(GPIO_CRH_CNF14|GPIO_CRH_CNF15); /* PUSH pull */
    Delay(30);
    GPIOA->BSRR =  GPIO_BSRR_BS14; /* Boot the cristal */
    Delay(30);
    GPIOC->CRH &= ~(GPIO_CRH_MODE14_1|GPIO_CRH_MODE15_1); /* Analog IN */
    RCC->BDCR |= RCC_BDCR_LSEON;
    {
      __IO int i=1000000;
      while ( i != 0 && (RCC->BDCR & RCC_BDCR_LSERDY) == 0)
        i--;
    }
    if ( (RCC->BDCR & RCC_BDCR_LSERDY) == 0 )
      return -1;
  
    RCC->BDCR |= RCC_BDCR_RTCSEL_0; /* RTC source is LSE */
    RCC->BDCR |= RCC_BDCR_RTCEN; /* Enable RTC */

    BKP->RTCCR = 63; 

    RTC->CRL |= RTC_CRL_CNF; /* Enter config mode */
    RTC->PRLL = CLOCK_DIV; /* Freq is some higher that 1 hz */
    BKP->DR7  = CLOCK_DIV;
    RTC->CRL &= ~RTC_CRL_CNF; /* Write value  */
    while ( (RTC->CRL & RTC_CRL_RTOFF) == 0 ) /* Wait to ready for writing */
      ;
  }

  RTC->CRL &= ~RTC_CRL_RSF; /* This bit will be set at syncronisation time */
  while ( (RTC->CRL & RTC_CRL_RSF) == 0)
    ; /* Wait syncronisation */
  while ( (RTC->CRL & RTC_CRL_RTOFF) == 0 ) /* Wait to ready for writing */
    ;

  RTC->CRH |= RTC_CRH_SECIE; /* Enable second interrupt */
  while ( (RTC->CRL & RTC_CRL_RTOFF) == 0 ) /* Wait to ready for writing */
    ;
  NVIC->ISER[(RTC_IRQn >> 0x05)] =	(u32)0x01 << (RTC_IRQn & (u8)0x1F); /* Enable RTC second interrupt */

  return 0;
}

void RTC_IRQHandler(void)
{
  if ((EventQueue & ~KEY_ADC) == 0 ) /* ADC has lower priority */
    EventQueue = EV_KEY_PRESSED | KEY_CLOCK;
  RTC->CRL &= ~RTC_CRL_SECF;
  if (TimerValue != 0)
  {
    if (RemainTimerValue > 1)
    {
      if ( RemainTimerValue != 0xFFFF )
        RemainTimerValue--;     
    }
    else
    {
      RemainTimerValue = 0xFFFF;
      DAC_V = 0; /* Off the output */
      DAC_I = 0;
    }
  }
}

static int32_t NewTime;
static uint8_t Change;

static int Inc;
void SetupTheClock(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = 0;
    Change = 0;
    Inc = 1;
    NewTime = (((RTC->CNTH<<16) + RTC->CNTL)%(24*60*60) + (24*60*60));
    NewTime = NewTime / 60*60; /* Round to 1 minutes */
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
       case KEY_UP:
        if ( Change == 0 )
        {
          if ( MenuPos == 6 )
            MenuPos = 0;
          else
            MenuPos = MenuPos + 1;
          goto redraw;
        }
        else
          Inc = 1;
      goto ChangeTime;
      case KEY_DOWN:
        if ( Change == 0 )
        {
          if ( MenuPos == 0 )
            MenuPos = 6;
          else
            MenuPos = MenuPos - 1;
          goto redraw;
        }
        else
          Inc = -1;
        goto ChangeTime;
      case KEY_ENTER:
        switch(MenuPos)
        {
          case 0:
          case 1:
          case 2:
          case 3:
            Change = !Change;
            return;
          case 4:
            BKP->DR5 = (NewTime >> 16); 	/* 05.10.11 Clear calibration time to prevent bad next calibration. */
    		    BKP->DR6 = NewTime; 
            
            /* Restore default clock speed 18.10.11 */
            BKP->RTCCR = 63; 
            BKP->DR7  = CLOCK_DIV;

            RTC->CRL |= RTC_CRL_CNF; /* Enter config mode */
            RTC->PRLL = CLOCK_DIV; /* Freq is some higher that 1 hz */
            while ( (RTC->CRL & RTC_CRL_RTOFF) == 0 ) /* Wait to ready for writing */
              ;
            goto SaveTime;
          case 5:
            CurrentFunc(StartFunction);
            return;
          case 6: /* Calibration */
            if (BKP->DR5 != 0 || BKP->DR6 != 0 ) /* Trim the clock */
            {  /* Trim the clock speed */
              int Delta;
              int Time;
              Delta = NewTime%(24*60*60) - (((RTC->CNTH<<16) + RTC->CNTL)%(24*60*60)); /* New time - old time. Positive is slow */
              Time  = ((RTC->CNTH<<16) + RTC->CNTL) - ((BKP->DR5<<16) +  BKP->DR6); /* Old time - last trim date */
              if ( Delta > (12*60*60) )
                Delta = Delta - (24*60*60);
              if ( Delta < -(12*60*60) )
                Delta = Delta + (24*60*60);
              if (Time > (24*60*60 - 60*10) ) /* Time eplased is More then 1 day without 10 minutes */
              {
#if !defined(__GNUC__)
                Delta = (Delta * ((int64_t)1024*1024))/Time; /* Delta in PPM */
#else
                /* Int64 operationd adds about 9k of code!!! */		
                while ( Delta > 2047 || Delta < -2047 )
                {
                  Delta = Delta/2;
                  Time  = Time/2;
                }
                Delta = (Delta * (1024*1024))/Time;
#endif
                Delta = (BKP->RTCCR & BKP_RTCCR_CAL) - Delta;
                if (Delta < 0 || Delta > 127 )
                {
                  BKP->DR7  = BKP->DR7 + Delta/32 - 2; /* Save new prescaler value */
                  RTC->CRL |= RTC_CRL_CNF; /* Enter config mode */
                  RTC->PRLL = BKP->DR7; /* Set new prescaler value. PRLL is read only */
                    RTC->CRL &= ~RTC_CRL_CNF; /* Write value  */
                    while ( (RTC->CRL & RTC_CRL_RTOFF) == 0 ) /* Wait to ready for writing */
                        ;
                  Delta = 64 + (Delta - Delta/32*32);
                }
                BKP->RTCCR = Delta; /* Write correction */
              }
            }
            BKP->DR5 = (NewTime >> 16); 
            BKP->DR6 = NewTime;
            goto SaveTime;
        }
    }
  }
  return;

SaveTime:
    RTC->CRL |= RTC_CRL_CNF; /* Enter config mode */
    RTC->CNTH = NewTime>>16;
    RTC->CRL &= ~RTC_CRL_CNF; /* Write value  */
    while ( (RTC->CRL & RTC_CRL_RTOFF) == 0 ) /* Wait to ready for writing */
      ;
    RTC->CRL |= RTC_CRL_CNF; /* Enter config mode */
    RTC->CNTL = NewTime&0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF; /* Write value  */
    while ( (RTC->CRL & RTC_CRL_RTOFF) == 0 ) /* Wait to ready for writing */
      ;
    CurrentFunc(StartFunction);
    return;

ChangeTime:
  switch (MenuPos)
  {
    case 0: 
        Inc = Inc * 10 * 60 * 60;  /* +-10 hours */
        break;
    case 1: 
        Inc = Inc * 60 * 60;  /* +-1 hours */
        break;
    case 2:
        Inc = Inc * 10 * 60;  /* +-10 min */
        break;
    case 3:
        Inc = Inc * 60;  /* +-1 min */
        break;
  }
  if ( NewTime + Inc < 24*60*60 )
    return;
  if ( NewTime + Inc >= ( 24*60*60*2 - 1 ) ) /* 2 day without 1 sec */
    return;
  NewTime += Inc;

redraw:
  DisplayClock(NewTime, MenuPos, Y_POSITION*3+X_POSITION*9);
  LcdChr (Y_POSITION*4+X_POSITION*0+14+(4==MenuPos)*INVERSE, "Set time" );
  LcdChr (Y_POSITION*5+X_POSITION*0+14+(5==MenuPos)*INVERSE, "Return" );
  LcdChr (Y_POSITION*1+X_POSITION*0+14+(6==MenuPos)*INVERSE, "Calibration" );
}


uint16_t TimerValue = 0;
uint16_t RemainTimerValue = 0;
void TimerSetup(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    EncCounter = TimerValue;
    EncStepMax = 1800;
    EncMax = 18 * 3600 + 1;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    if ( (Event & KEY_MASK) == KEY_ENTER )
    {
      TimerValue = EncCounter;
      EncStepMax = ENC_STEP_MAX_DEFAULT;
      EncMax = ENC_MAX_DEFAULT;
      CurrentFunc(StartFunction);
      return;
    }
  }

  LcdChr(Y_POSITION*1+X_POSITION*0+14, "Timer value" );
  DisplayClock(EncCounter, 10+0x8000, Y_POSITION*3+X_POSITION*6);
}


#endif
