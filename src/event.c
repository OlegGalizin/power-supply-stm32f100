#include <string.h>
#include "event.h"

uint16_t Event;     // Generated event
volatile uint8_t EvCounter; // Press delay counter

MenuFunction_t CurrentFunction = NULL; // Current function
MenuFunction_t PrevFunc = NULL;

volatile uint16_t EventQueue; // Generated event
static uint16_t PrevKey; // Previous keys pressed
static uint8_t  RealizeCounter; // Realize delay counter

static uint16_t PrevCounter;
void EventInit(void)
{
  NVIC->ISER[(TIM3_IRQn >> 0x05)] =	(u32)0x01 << (TIM3_IRQn & (u8)0x1F); /* Enable Timer interrupt */
  NVIC->ISER[(TIM2_IRQn >> 0x05)] =	(u32)0x01 << (TIM2_IRQn & (u8)0x1F); /* Enable Timer interrupt */

  TIM3->DIER |= TIM_DIER_UIE; /* enable update IRQ */
  TIM3->ARR = 10000; /* 2MHz / 10000 = 200Hz  - 5mSec */
  TIM3->CR1 |= TIM_CR1_CEN;    /* start TIM7 */
  
//  TIM2->CNT = 0x8000;
  TIM2->ARR = 0xFFFF;
  TIM2->SMCR |= (TIM_SMCR_SMS_0|TIM_SMCR_SMS_1); /* encoder mode 2 */
//  TIM2->CCMR1 |= TIM_CCMR1_CC1S_0|TIM_CCMR1_IC1F|TIM_CCMR1_CC2S_0|TIM_CCMR1_IC2F;  /* TI1, TI2 - inputs with max filter */
//  TIM2->CCER |= TIM_CCER_CC1E|TIM_CCER_CC2E; /* Enable inputs */
  TIM2->SR = 0;
  TIM2->DIER |= TIM_DIER_CC2IE|TIM_DIER_CC1IE; /* Interrupt by trigger */
  TIM2->CCR1 = (uint16_t)- 4;
  TIM2->CCR2 = 0 + 4;
  PrevCounter = 0;
  TIM2->CR1 |= TIM_CR1_CEN;
}

void EventKeys(void)
{
  uint16_t Key;
  
  if ( (EventQueue & KEY_MASK_SYS) != 0 ) /* Previous event hasn't been handled. The key hase highiest  priority */
    return;
  if (CurrentFunction != PrevFunc ) /* The function was changed */
    return;
  Key = (~KEYPORT) & KEY_MASK_SYS; /* Read the port */

  if ( Key == 0 ) // no any key pressed
  {
    if ( PrevKey == 0 ) // no any key was pressed before
    {
      EvCounter = 0;
      return;
    }
    if ( EvCounter > KEY_PRESSED_VALUE )
    {
      RealizeCounter++; // increase timer counter
      if ( RealizeCounter > KEY_REALIZE_VALUE ) // expired realise timeout
      {
        if ( EvCounter != 0xFF ) /* There is no switch to new function. New function should not get previos function event */
        {
          EventQueue = EV_KEY_REALIZED | PrevKey; /* Realized event - the last event */
        }
        EvCounter = 0; // Reset interval timer value
        PrevKey = 0;   // Reset key pressed value
        RealizeCounter = 0;  // Reset realise counter
      }
    }
    else
    {
      EvCounter = 0; // Reset interval timer value
      PrevKey = 0;   // Reset key pressed value
      RealizeCounter = 0;  // Reset realise counter
    }
  }
  else // Some keys are pressed
  {
    RealizeCounter = 0; //reset realise delay
   
    if ( EvCounter == 0xFF ) /* Locked - new function has been set */
      return; 
    
    if ( Key & (~PrevKey) ) //there are some new keys
    {
      PrevKey |= Key;       // adding the new keys
      if ( EvCounter == 0 )
      {
      /* Generate KEY TOUCH event */
        EventQueue = EV_KEY_TOUCH + PrevKey;
      }
      else if ( EvCounter > KEY_LONG_VALUE ) // Delay after first press is not long
        EventQueue = EV_KEY_LONG | PrevKey; //generate key press event
    }
    else // the same keys are pressed
    {
      if ( EvCounter == KEY_PRESSED_VALUE ) // Delay after first press is not long
        EventQueue = EV_KEY_PRESSED | PrevKey; //generate key press event
      else if ( EvCounter == KEY_LONG_VALUE )  // Long press timeout has expired
      {
        EventQueue = EV_KEY_LONG | PrevKey; // Generate Long press event
      }
      else if ( EvCounter == KEY_REPEATE_VALUE ) // After long press the key is stil pressed
      {
        EventQueue = EV_KEY_REPEATE | PrevKey; // Generate repeate press event
        EvCounter = KEY_LONG_VALUE; // Reset time counter for next delay
      }
      EvCounter++; // Delay counter increasing
    }
  }
}

void EventCheck(void)
{
      if ( CurrentFunction != PrevFunc ) // Function was changed
      {
        Event = EV_FUNC_FIRST;       // Generate FUNC_FIRST event
        PrevFunc = CurrentFunction;      // Save the function
    		__disable_irq();
        if ( EvCounter )             /// Some keys are stil pressed
          EvCounter = 0xFF;            // Lock any key events until all keys are not realized
    		__enable_irq();
      }
      else
      {
    		__disable_irq();
        Event = EventQueue;          // Read event thar was generated in interrupt handlers
        EventQueue = 0;              // The interrupt handlers can write new value
    		__enable_irq();
      }

      CurrentFunction();      // Run the current menu function
}

static uint16_t TickCounter;
static uint16_t PrevTickCounter;
uint16_t EncCounter;

void TIM3_IRQHandler(void)
{
  TIM3->SR = 0; /* Clear pending flag */
  EventKeys();
  TickCounter++;
  if ( TickCounter - PrevTickCounter > 1024 ) /* 10 cek */
    PrevTickCounter = TickCounter - 512; /* Once in 2.5 second */
  if ( TickCounter - PrevTickCounter == 2 )
  {
	  TIM2->SR = 0; /* Clear pending flag */
  	TIM2->DIER |= TIM_DIER_CC2IE; /* Interrupt by trigger */
  }   
}

uint16_t EncStep;
uint16_t EncStepMax = ENC_STEP_MAX_DEFAULT;
uint16_t EncMax = ENC_MAX_DEFAULT;

void TIM2_IRQHandler(void)
{
  static uint16_t Delta;

  PrevCounter = TIM2->CNT;
  TIM2->CCR1 = PrevCounter - 4;
  TIM2->CCR2 = PrevCounter + 4;
  TIM2->SR = 0; /* Clear pending flag - only trigger interrupt*/

  Delta = TickCounter - PrevTickCounter;
  PrevTickCounter = TickCounter;
  
  Delta = Delta / 2;
  if (EncStep == 0)
    EncStep = 1;
  if ( Delta < 8 && EncStep < EncStepMax) /* The step has to be inreased */
  {
    if ( EncStep < 4 )
      EncStep = EncStep*2;
    else if (EncStep < 64)
      EncStep = EncStep + EncStep/4;
    else
      EncStep = EncStep + EncStep/8;
  }
  if ( Delta > 20 )
  {
    if (Delta > 120) /* 0.6 sec */
      EncStep = 1;
    EncStep = EncStep - EncStep/2;
  }

  if (TIM2->CR1 & TIM_CR1_DIR)
  {
    if ((int32_t)EncCounter - EncStep < 0 )
      EncCounter = 0;
    else
      EncCounter -= EncStep;
    if ( (EventQueue & KEY_MASK_SYS) == 0) /* Medium priority */
      EventQueue = EV_KEY_PRESSED|KEY2; 
  }
  else
  {
    if ((uint32_t)EncCounter + EncStep > EncMax )
      EncCounter = EncMax;
    else
      EncCounter += EncStep;
    if ( (EventQueue & KEY_MASK_SYS) == 0) /* Medium priority */
      EventQueue = EV_KEY_PRESSED|KEY3; 
  }
}


#if defined(EVENT_DEMO)

#include "n3310.h"

void SystemInit()
{
}

#if defined(ENC_DEMO)
uint16_t Value;

void Contrast(void)
{
}

void StartFunction(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    Value = 0;
    goto redraw;
  }
  if ( (Event & KEY2) || (Event & KEY3) )
  {
    Value = EncCounter;
    LcdChr ( Y_POSITION*3+X_POSITION*1+9, "Encoder" );
    goto redraw;
  }

  if ( (Event & KEY_MASK) == 0)
    return;

  if ( ((Event & EV_MASK) == EV_KEY_LONG) )
  { 
    LcdChr ( Y_POSITION*3+X_POSITION*1+9, "Long key" );
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_TOUCH )
  { 
    LcdChr ( Y_POSITION*3+X_POSITION*1+9, "Touch key" );
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_REALIZED)
  { 
    LcdChr ( Y_POSITION*3+X_POSITION*1+9, "Realize key" );
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_DOUBLE)
  { 
    LcdChr ( Y_POSITION*5+X_POSITION*1+9, "Double key" );
    goto redraw;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  { 
    LcdChr ( Y_POSITION*5+X_POSITION*1+9, "" );
    LcdChr ( Y_POSITION*3+X_POSITION*1+9, "Key" );
	Value = 1;
    goto redraw;
  }
  
  if ( (Event & EV_MASK) == EV_KEY_REPEATE )
  {
    LcdChr ( Y_POSITION*3+X_POSITION*1+9, "Repeate key" );
    if ( Value < 9000 )
   	  Value++;
	goto redraw;
  }

  return;

redraw:
  {
#define DIGIT_COUNT 4
#define FIRST_DIV 1000
    char Out[DIGIT_COUNT];
  	int Div = FIRST_DIV;
  	char* Pointer = Out;
  	char i = DIGIT_COUNT;
  	char OnFlag = 0;
	int Data = Value;

  	while (i)
  	{
     char Dig = Data/Div;

     if ( Dig == 0 && OnFlag == 0)
     {
       *Pointer = ' '; //First zero digits are not displaed
     }
     else
     {
       *Pointer = Dig + '0';
       OnFlag = 1;
     }
     Pointer++;
     i--;
	 if ( i == 1 )
	   OnFlag = 1; /* Zero also should be displayed */
     Data = Data%Div;
     Div = Div/10;
  	} //while
    LcdChr ( Y_POSITION*1+X_POSITION*1+DIGIT_COUNT+BIG_UP, Out );
    LcdChr ( Y_POSITION*2+X_POSITION*1+DIGIT_COUNT+BIG_DOWN, Out );
    LcdChr ( Y_POSITION*4+X_POSITION*1+9, "Menu Func" );
  }
}
#endif /* ENC_DEMO */

#if defined(MENU_DEMO)
uint8_t MenuCounter;

void MenuSelected(void);
void Contrast(void);
#if defined(FLOAT_DEMO)
void FloatChange(void);
#include "ftos.h"
#endif

void MainMenu()
{
  if ( Event == 0 )
    return;

  if ( Event == EV_FUNC_FIRST )
  {
    MenuCounter = 0;
    goto RedrawMenu;
  }

  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        if ( MenuCounter == 0 )
          MenuCounter = 4;
        else
          MenuCounter = MenuCounter - 1;
        break;
      case KEY_DOWN:
        if ( MenuCounter == 4 )
          MenuCounter = 0;
        else
          MenuCounter = MenuCounter + 1;
        break;
      case KEY_ENTER:
        switch(MenuCounter)
        {
          case 4:
            CurrentFunc = Contrast;
            break;
#if defined(FLOAT_DEMO)
          case 2:
            CurrentFunc = FloatChange;
            break;
#endif
          default:
            CurrentFunc = MenuSelected;
            return;
        }
    }
  }
  else
    return;
RedrawMenu:
  LcdChr(X_POSITION*0+Y_POSITION*1+14 + (0==MenuCounter)*INVERSE, "Menu1");
  LcdChr(X_POSITION*0+Y_POSITION*2+14 + (1==MenuCounter)*INVERSE, "Menu2");
#if defined(FLOAT_DEMO)
  LcdChr(X_POSITION*0+Y_POSITION*3+14 + (2==MenuCounter)*INVERSE, "Float demo");
#else
  LcdChr(X_POSITION*0+Y_POSITION*3+14 + (2==MenuCounter)*INVERSE, "Menu3");
#endif  
  LcdChr(X_POSITION*0+Y_POSITION*4+14 + (3==MenuCounter)*INVERSE, "Menu4");
  LcdChr(X_POSITION*0+Y_POSITION*5+14 + (4==MenuCounter)*INVERSE, "Contrast");
}

void MenuSelected(void)
{
  if ( (Event&EV_MASK) == EV_FUNC_FIRST)
  {
    LcdChr(X_POSITION*0+Y_POSITION*1+14,  "Press key     ");
    LcdChr(X_POSITION*0+Y_POSITION*2+14,  "ENTER for a   ");
    LcdChr(X_POSITION*0+Y_POSITION*3+14,  "long time to  ");
    LcdChr(X_POSITION*0+Y_POSITION*4+14,  "return main   ");
    LcdChr(X_POSITION*0+Y_POSITION*5+14,  "menu");
    return;
  }
  
  if ( Event == (EV_KEY_LONG + KEY_ENTER) )
  { /* Return back */
    CurrentFunc = MainMenu;
  } 
}

#if defined(FLOAT_DEMO)
void FloatChange(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    EncCounter = 2000;
    LcdClear(); /* Clear screen */
    goto redraw;
  }
  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_ENTER:
         CurrentFunc = MainMenu;
         return;
    }
  }
redraw:
  {
    char Buf[10];
    float Out = EncCounter *0.00756 + 0.015;
    
    FToS(Out, Buf);
      
    LcdChr(Y_POSITION*2 + X_POSITION * 2 + 10, Buf);
  }
}
#endif /* FLAT_DEMO */

void Contrast(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    MenuCounter = 70;
    LcdClear(); /* Clear screen */
    goto redraw;
  }
  if ( (Event & EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        if ( MenuCounter < 90 )
          MenuCounter++;
        break;
      case KEY_DOWN:
        if ( MenuCounter > 0 )
          MenuCounter--;
        break;
      case KEY_ENTER:
         CurrentFunc = MainMenu;
         return;
    }
redraw:
    LcdContrast ( MenuCounter );
    {
      char Buf[2];
      uint8_t Counter = MenuCounter;
      
      Buf[0] = Counter/10 + '0';
      Counter = Counter%10;
      Buf[1] = Counter + '0';
      LcdChr(BIG_UP+Y_POSITION*2 + X_POSITION * 2 + 2, Buf);
      LcdChr(BIG_DOWN+Y_POSITION*3 + X_POSITION * 2 + 2, Buf);
    }
  }
}

int LongCounter;

void StartFunction(void)
{
  char* OutString = "";
  char  KeyArray[3];
  
  if (Event == 0)
    return;
  
  switch ( Event & EV_MASK )
  {
    case EV_KEY_TOUCH:
      OutString = "Touch";
//      return;
      break;
    case EV_KEY_PRESSED:
      OutString = "Press";
      break;
    case EV_KEY_LONG:
      OutString = "Long";
      if ( Event & KEY_ENTER )
	  {
	    if (LongCounter > 3 )
         CurrentFunc = MainMenu;
        LongCounter++;
	  }
      break;
    case EV_KEY_REPEATE:
      OutString = "Repeate";
      break;
    case EV_KEY_REALIZED:
      OutString = "Realize";
      break;
    case EV_KEY_DOUBLE:
      OutString = "Double";
      break;
    case EV_FUNC_FIRST:
      OutString = "First";
  }
    
  LcdChr(Y_POSITION*(MenuCounter%5+1) + 14, OutString);
  Event &= KEY_MASK;
  KeyArray[0] = Event & KEY1 ? '*':'-';
  KeyArray[1] = Event & KEY2 ? '*':'-';
  KeyArray[2] = Event & KEY3 ? '*':'-';
  LcdChr(Y_POSITION*(MenuCounter%5+1) + X_POSITION * 7 + 3, KeyArray);

  {
    uint8_t Counter = MenuCounter;
    KeyArray[0] = Counter/100 + '0';
    Counter = Counter%100;
    KeyArray[1] = Counter/10 + '0';
    Counter = Counter%10;
    KeyArray[2] = Counter + '0';
    LcdChr(Y_POSITION*(MenuCounter%5+1) + X_POSITION * 10 + 3, KeyArray);
  }
  MenuCounter++;
}

#endif /*MENU_DEMO*/



int main()
{
  CurrentFunc(StartFunction);
  /* init hardware */
  SCB->VTOR = FLASH_BASE;
//  NVIC->ISER[(TIM3_IRQn >> 0x05)] |=	(u32)0x01 << (TIM3_IRQn & (u8)0x1F); /* Enable Timer interrupt */
//  NVIC->ISER[(TIM2_IRQn >> 0x05)] |=	(u32)0x01 << (TIM2_IRQn & (u8)0x1F); /* Enable Timer interrupt */
  LcdInit();
  LcdClear();

  /* CLOCK = 8MHz / 8 = 1 MHz */
  RCC->CFGR |= RCC_CFGR_HPRE_DIV2|RCC_CFGR_PPRE1_DIV4;
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |RCC_APB2ENR_AFIOEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN|RCC_APB1ENR_TIM2EN; 

  GPIOB->CRH |= GPIO_CRH_MODE12_1|GPIO_CRH_MODE13_1|GPIO_CRH_MODE14_1|GPIO_CRH_MODE15_1; /* Output 2MHz */
  GPIOB->CRH &= ~(GPIO_CRH_CNF12|GPIO_CRH_CNF13|GPIO_CRH_CNF14|GPIO_CRH_CNF15); /* Output Push-pull */

#if 0   
  TIM3->DIER |= TIM_DIER_UIE; /* enable update IRQ */
  TIM3->ARR = 10000; /* 1MHz / 10000 = 100Hz  - 10mSec */
  TIM3->CR1 |= TIM_CR1_CEN;    /* start TIM7 */
  
  TIM2->CNT = 0x8000;
  TIM2->ARR = 0xFFFF;
  TIM2->SMCR |= TIM_SMCR_SMS_0; /* encoder mode 1 */
  TIM2->CCMR1 |= TIM_CCMR1_CC1S_0|TIM_CCMR1_IC1F|TIM_CCMR1_CC2S_0|TIM_CCMR1_IC2F;  /* TI1, TI2 - inputs with max filter */
  TIM2->CCER |= TIM_CCER_CC1E|TIM_CCER_CC2E; /* Enable inputs */
  TIM2->SR = 0;
  TIM2->DIER |= TIM_DIER_CC2IE/*|TIM_DIER_CC1IE*/; /* Interrupt by trigger */
  TIM2->CR1 |= TIM_CR1_CEN;
#endif
  EventInit();
   
  do
  	EventCheck();
  while(1);
}
#endif  /* EVENT_DEMO*/

