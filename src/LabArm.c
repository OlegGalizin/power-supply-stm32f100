#include "event.h"
#include "n3310.h"
#include "LabArm.h"


uint16_t AdcOutArray[ADC_ARRAY_SIZE];
static uint32_t ADCVoltageSum;
static uint32_t ADCCurrentSum;
static uint8_t DMACounter;
uint32_t ADCVoltage;
uint32_t ADCCurrent;

#if defined(__GNUC__)
void DMA1_Channel1_IRQHandler()
#else
void DMAChannel1_IRQHandler(void) /* Read ADC values */
#endif
{
  if (DMA1->ISR & DMA_ISR_HTIF1 && /*   Half Transfer complete */
       (DMA1_Channel1->CCR & DMA_CCR1_HTIE) /* Half traisfer interrupt enabled */
      ) 
  {
    int i;
    /* HTIF is set every time when transfer interrupt > half !!! */
    DMA1_Channel1->CCR &= ~DMA_CCR1_HTIE; /* disable Half traisfer interrupt !!!! */

    for (i = 0; i < ADC_ARRAY_SIZE/2; i = i+2)
    {
      ADCVoltageSum += AdcOutArray[i];
      ADCCurrentSum += AdcOutArray[i+1];
    }
  } else if (DMA1->ISR & DMA_ISR_TCIF1 ) /* transfer complete */
  {
    int i;

    /* transfer complete */
    DMA1->IFCR |= DMA_IFCR_CGIF1; /* Clear all interrupt flags */
    DMA1_Channel1->CCR |= DMA_CCR1_HTIE; /* enable Half traisfer interrupt  */

    for (i=ADC_ARRAY_SIZE/2; i< ADC_ARRAY_SIZE; i=i+2)
    {
      ADCVoltageSum += AdcOutArray[i];
      ADCCurrentSum += AdcOutArray[i+1];
    }

    DMACounter++;
    if (DMACounter == 16) /* 2Mhz/195/ADC_ARRAY_SIZE*2/16 - refresh frenq = 5 Hz */
    {
      DMACounter = 0;
      ADCVoltage = ADCVoltageSum;
      ADCCurrent = ADCCurrentSum;
      NVIC_SetPendingIRQ(EXTI4_IRQn);
      ADCVoltageSum = 0;
      ADCCurrentSum = 0;
      if ( EventQueue == 0 ) /* Lowest priority */
        EventQueue = EV_KEY_PRESSED|KEY_ADC; 
    }
  }
  DMA1->IFCR |= DMA_IFCR_CGIF1; /* Clear all interrupt flags */
}

#if defined(__GNUC__)
void ADC1_IRQHandler(void)
#else
void ADC_IRQHandler(void) /*  Watchdog */
#endif
{
  DAC_V = 0; /* OFF output */
  DAC_I = 0;
  ADC1->CR2 = 0; /* Adc Off */
  GPIOB->BRR = GPIO_BRR_BR0|GPIO_BRR_BR1;
  GPIOB->CRL &= ~(GPIO_CRL_CNF0|GPIO_CRL_CNF1); /* PUSH pull */
  DAC->CR = 0; /* Dac OFF */

  /* Save current settings */
  SaveSettings(0);
  LcdInit(Contrast*3+42);
  LcdChr(Y_POSITION*3+X_POSITION*0+14+INVERSE, "Power fault" );

  NVIC->ICER[(ADC1_IRQn >> 0x05)] =	(u32)0x01 << (ADC1_IRQn & (u8)0x1F); /* Off ADC interrupt */
  NVIC->ICPR[(ADC1_IRQn >> 0x05)] =	(u32)0x01 << (ADC1_IRQn & (u8)0x1F); /* Clear pend ADC interrupt */

  { /* Waiting for power off */
    __IO uint32_t Delay = 5000000;
    while (Delay--) ;
  }

  /* Reboot */
  NVIC_SystemReset();
}

void LcdBlank(void)
{
  LcdInit(42+Contrast*3);
  LcdClear();
}

int16_t HumanV;
int16_t HumanI;


void EXTI4_IRQHandler(void)
{
  HumanV = VoltageFromAdc(); 
  HumanI = CurrentFromAdc();
#if defined(GRAPH)
  {
    static Graph_t I = {32000,-32000};
    static Graph_t V = {32000,-32000};
    static uint16_t Time;

    if (  HumanI < I.Min )
      I.Min = HumanI;
    if ( HumanI > I.Max )
      I.Max = HumanI;
    if (HumanV < V.Min)
      V.Min = HumanV;
    if (HumanV > V.Max)
      V.Max = HumanV;
    Time++;
    if (Time >= TimeInterval)
    {
      Time = 0;
      GraphCurrentPoint++;
      if (GraphCurrentPoint == GRAPH_SIZE)
        GraphCurrentPoint = 0;
      IGraphArray[GraphCurrentPoint] = I;
      VGraphArray[GraphCurrentPoint] = V;
      I.Min = 32000;
      I.Max = -32000;
      V.Min = 32000;
      V.Max = -32000;
    }
  }
#endif
}

#if 1
void SystemInit() 
{};


int main()
{
 /* init hardware */
  SCB->VTOR = FLASH_BASE;
  NVIC_EnableIRQ(DMA1_Channel1_IRQn); /* Enable DMA interrupt */
  NVIC_EnableIRQ(ADC1_IRQn); /* Enable ADC interrupt */
  NVIC_EnableIRQ(EXTI4_IRQn); /*Additional IRQ to calculate V and I */
  NVIC_SetPriority(EXTI4_IRQn, 12); /* Low priority */

  /* CLOCK = 8MHz / 8 = 1 MHz */
  RCC->CFGR |= RCC_CFGR_HPRE_DIV2|RCC_CFGR_PPRE1_DIV4|RCC_CFGR_PPRE2_DIV2; /* APB1 - 1MHz, AHB - 4MHz, APB2 - 2MHz, ADC - 1MHz */
  /* Enable peripery clock */
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |RCC_APB2ENR_ADC1EN|RCC_APB2ENR_TIM1EN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN|RCC_APB1ENR_TIM2EN|RCC_APB1ENR_DACEN|RCC_APB1ENR_PWREN|RCC_APB1ENR_BKPEN; 
  RCC->AHBENR  |= RCC_AHBENR_DMA1EN|RCC_AHBENR_CRCEN; 

  /* Unlock the backup domain */
  PWR->CR |= PWR_CR_DBP;

  /* GPIO configure */
  /* PB12 - PB14 - LCD, PA0 - PA2 - encoder(TIM2), PA4-PA5 - DAC Out, PB0-PB1 - ADC(8-9channel), PA7 - ADC power measure */
  /* PB0 - Voltage IN, PB1 - CurrentIN, PA4 - VoltageOut, PA5 - CurrentOut */
  GPIOB->CRH |= GPIO_CRH_MODE12_1|GPIO_CRH_MODE13_1|GPIO_CRH_MODE14_1|GPIO_CRH_MODE15_1; /* Output 2MHz */
  GPIOB->CRH &= ~(GPIO_CRH_CNF12|GPIO_CRH_CNF13|GPIO_CRH_CNF14|GPIO_CRH_CNF15); /* Output Push-pull */
  GPIOB->CRL |= GPIO_CRL_MODE0_1 |GPIO_CRL_MODE1_1;
  GPIOB->CRL &= ~(GPIO_CRL_CNF0|GPIO_CRL_CNF1); /* PUSH pull */
  
  GPIOA->CRL |= GPIO_CRL_MODE4_1 |GPIO_CRL_MODE5_1|GPIO_CRL_MODE6_1|GPIO_CRL_MODE7_1; /* Out 2 MHZ*/
  GPIOA->CRL &= ~(GPIO_CRL_CNF4|GPIO_CRL_CNF5|GPIO_CRL_CNF6|GPIO_CRL_CNF7); /* PUSH pull */
  GPIOA->BSRR =  GPIO_BSRR_BS6;

  LcdBlank();
  /* Enable the programmable voltage detector */
  PWR->CR |= PWR_CR_PLS_2|PWR_CR_PLS_1; /* 2.8 V */
  PWR->CR |= PWR_CR_PVDE; /* Enable the PVD */

  { /* Waiting for power stabilize */
    __IO uint32_t Delay = 1000000;
    while (Delay--) ;
  }

  /* Check power supply */
  while ( (PWR->CSR & PWR_CSR_PVDO ) != 0 )
    ; /* BLANK */
  /* Power is OK for ADC */

  /* Waiting for stabilize power voltage. It should be more then 1.2 V on the PB5 pin */
  {
    /* Measure PB5 and Vint by injected group */
    GPIOA->CRL &= ~GPIO_CRL_MODE7; /* Input, Analog*/
    ADC1->CR1 |= ADC_CR1_SCAN;
    ADC1->CR2 |= ADC_CR2_TSVREFE|ADC_CR2_JEXTTRIG | ADC_CR2_JEXTSEL; /* injected start by JSWSTART */
    ADC1->SMPR1 |= ADC_SMPR1_SMP17; /* Internal Vref, max sampling time */
    ADC1->SMPR2 |= ADC_SMPR2_SMP7; /* max sampling time */
    ADC1->CR2 |= ADC_CR2_ADON; /* On the ADC */
    
    /* Additional delay to Vref on */
    Delay(10000);

    /* Calibration */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while ( ADC1->CR2 & ADC_CR2_RSTCAL )
      ; /* BLANK */
    ADC1->CR2 |= ADC_CR2_CAL;
    while ( ADC1->CR2 & ADC_CR2_CAL )
      ; /* BLANK */

    Delay(10000);
  }
    
  /* theck the power voltage */
  /* 2 conversions. 7-th channels, 17-th channel  */
  ADC1->JSQR |= ADC_JSQR_JL_0| /* 2 conversion */
                ADC_JSQR_JSQ3_0|ADC_JSQR_JSQ3_1|ADC_JSQR_JSQ3_2| /* CH 7 */
                ADC_JSQR_JSQ4_4|ADC_JSQR_JSQ4_0; /* CH 17 */
  do
  {
    ADC1->SR &= ~ADC_SR_JEOC; /* Reset convertion flag */
    ADC1->CR2 |= ADC_CR2_JSWSTART; /* Start the convertion for two channels */
    while ((ADC1->SR & ADC_SR_JEOC) == 0) /* Waiting end of convertion */
      ; /* BLANK */
#if defined(AWD_ENABLED)
  } while ( ADC1->JDR1/* Vin */ < ADC1->JDR2/* Vref */ );
#else
  } while ( 0 );
#endif
  {
    uint16_t SavedVRef = ADC1->JDR2;

    /* Set up analog watch dog */
    ADC1->CR2 = 0; /* Power off */
    /* Reset the ADC */
    RCC->APB2RSTR |= RCC_APB2RSTR_ADC1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_ADC1RST;

    ADC1->HTR = 0xFFF; /* Max value for HIGHT threshold*/
    ADC1->LTR = SavedVRef/6*5; /* 1 V */
  }

  /* Set up ADC, TIM1, DMA for conversion */

  ADC1->CR1 |= 
#if defined(AWD_ENABLED)
                ADC_CR1_JAWDEN|ADC_CR1_AWDIE| /* AWD on injected channels whith interrupt */
#endif
                ADC_CR1_JAUTO|ADC_CR1_SCAN; /* Scan mode + auto injection */
  ADC1->SMPR2 = ADC_SMPR2_SMP7_0|ADC_SMPR2_SMP8_0|ADC_SMPR2_SMP9_0; /* 7.5 sampling time, 20 - adc time, 5,6,7 ch */
  ADC1->SQR1 =  ADC_SQR1_L_0; /* 2 conversion */
  ADC1->SQR3 =  ADC_SQR3_SQ1_3| /* 8 channel - voltage*/
                ADC_SQR3_SQ2_3|ADC_SQR3_SQ2_0; /* 9- channel - current */
  ADC1->JSQR = ADC_JSQR_JSQ4_2|ADC_JSQR_JSQ4_1|ADC_JSQR_JSQ4_0; /* 1 conversion by 7-th channel in inj */
  ADC1->CR2 |= ADC_CR2_EXTTRIG|ADC_CR2_DMA; /* External trigger by T1 CC1, DMA */

  GPIOA->CRL &= ~(GPIO_CRL_MODE4_1|GPIO_CRL_MODE5_1|GPIO_CRL_MODE7_1); /* Analog IN */
  GPIOB->CRL &= ~(GPIO_CRL_MODE0_1|GPIO_CRL_MODE1_1); /* Analog IN */


  /* TIM1 is configyred to trigger the ADC */
  TIM1->ARR = 194; /* 2MHz / 200 = 10KHz */
  TIM1->CCMR1 |= TIM_CCMR1_OC1M; /* PWM 2 mode */
  TIM1->CCR1 = 99;
  TIM1->BDTR |= TIM_BDTR_MOE; /* Only for TIM1 - MAIN OUTPUT ENABLE!!! */
  TIM1->CCER |= TIM_CCER_CC1E; /* Output enable */
  
  /* DMA configuring */
  DMA1_Channel1->CCR |= DMA_CCR1_PL|DMA_CCR1_MSIZE_0|DMA_CCR1_PSIZE_0| /*Hight pry, 16 byte mem, 16 byte pereph */
          DMA_CCR1_MINC|DMA_CCR1_CIRC|DMA_CCR1_HTIE|DMA_CCR1_TCIE; /*  mem inc, circular, enterrupts by Half and End of conv */
  DMA1_Channel1->CNDTR = ADC_ARRAY_SIZE;
  DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
  DMA1_Channel1->CMAR = (uint32_t)&AdcOutArray[0];

  /* On converting */
  ADC1->CR2 |= ADC_CR2_ADON; /* Adc ON */
  /* Additional delay to Vref on */
  Delay(10000);

  /* Calibration */
  ADC1->CR2 |= ADC_CR2_RSTCAL;
  while ( ADC1->CR2 & ADC_CR2_RSTCAL )
    ; /* BLANK */
  ADC1->CR2 |= ADC_CR2_CAL;
  while ( ADC1->CR2 & ADC_CR2_CAL )
    ; /* BLANK */

  Delay(10000);
  DMA1_Channel1->CCR |= DMA_CCR1_EN; /* Enable DMA */
  TIM1->CR1 |= TIM_CR1_CEN;    /* start TIM1 */

  /* DAC Init */
  DAC_V = 0;
  DAC_I = 0;
  
  DAC->CR |= DAC_CR_BOFF2|DAC_CR_BOFF1; /* Disable buffers */
  DAC->CR |= DAC_CR_EN1|DAC_CR_EN2; /* On DAC */

  LcdBlank();
  EventInit();
#if defined(GRAPH)
  ClearGraph();
#endif

  if ( RestoreConfig() != 0 || (GPIOA->IDR & KEY_ENTER) == 0 )
  {
      AfterContrast = CalibrationMenu;
      Contrast = 7;
      CurrentFunc(ContrastMenu);
//    CurrentFunc(StartFunction);
  }
  else
  {
    CurrentFunc(StartFunction);
  }
#if defined(CLOCK_ENABLED)
  if ( IS_ON_CLOCK  ) /* RTC clock is on */
  {
    SwitchOnTheClock();
  }
#endif
  
  do
  	EventCheck();
  while(1);
}

#endif
