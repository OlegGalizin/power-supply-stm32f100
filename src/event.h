#if !defined(_EVENT_H_)
#define _EVENT_H_

#include <stm32f10x.h>

#define EV_KEY_TOUCH    0x0100
#define EV_KEY_PRESSED  0x0200 
#define EV_KEY_LONG     0x0300
#define EV_KEY_REPEATE  0x0400
#define EV_KEY_REALIZED 0x0500
#define EV_KEY_DOUBLE   0x0600
#define EV_FUNC_FIRST   0x0700
#define EV_MASK         0x0700

extern uint16_t Event;  /* 0xKKKKKAAA */
/* AAA - it is event key. Event can be defined by & operation with EV_MASK*/
/* KKKKKK - keys - one bit fpr one key. Up to 5 keys */
volatile extern uint16_t EventQueue; /* for setting user event */
extern uint16_t EncCounter; /* 0 - 4096 */
extern uint16_t EncStep;
extern uint16_t EncStepMax;
extern uint16_t EncMax;

#define ENC_MAX_DEFAULT 4095
#define ENC_STEP_MAX_DEFAULT 200;

extern volatile uint8_t EvCounter; // Press delay counter
typedef void (*MenuFunction_t)(void);
extern MenuFunction_t CurrentFunction;  // Current function
#define CurrentFunc(MenuFunc) CurrentFunction = MenuFunc

extern void EventInit(void);
extern void EventKeys(void); /* This function is periodicaly called (e.g. from ISR) */
extern void EventCheck(void); /* This function should be called to handele the event */

/* Event detect delays */
#define KEY_PRESSED_VALUE 4 // Press event delay
#define KEY_LONG_VALUE    140  // Long press event delay
#define KEY_REPEATE_VALUE 160 // Repeate event delay
#define KEY_REALIZE_VALUE 8 //  Realize event detect delay
//#define KEY_DOUBLE_VALUE  40 //  Double event detect delay. It should be more then REALIZE_KEY_VALUE and KEY_PRESSED_VALUE

#define KEYPORT GPIOA->IDR

#define KEY1  GPIO_IDR_IDR2
#define KEY2  GPIO_IDR_IDR1
#define KEY3  GPIO_IDR_IDR0
#define KEY4  GPIO_IDR_IDR3
#define KEY5  GPIO_IDR_IDR5
#define KEY6  GPIO_IDR_IDR6
#define KEY_MASK_SYS (KEY1)
#define KEY_MASK (KEY1|KEY2|KEY3|KEY4|KEY5)

#define KEY_ENTER 	KEY1    /* Encoder button */
#define KEY_DOWN 		KEY2    /* Encoder down */
#define KEY_UP 	    KEY3    /* Encored event up */
#define KEY_ADC     KEY4    /* Adc ready */
#define KEY_CLOCK   KEY5    /* Second event */
//#define KEY_GRAPH   KEY6    /* Gpath update */

#endif /* _EVENT_H_ */
