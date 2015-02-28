#if !defined(__LAB_ARM_H__)
#define __LAB_ARM_H__
#include "n3310.h"
#include "event.h"

#define Delay(tick) \
{ \
  __IO int i = tick; \
  while ( i != 0 ) \
    i--; \
}

#define ADC_ARRAY_SIZE 256
extern uint32_t ADCVoltage;
extern uint32_t ADCCurrent;

#define AUTO_APPLY_FLAG 0x8000
#define BACKUP_SAVE_FLAG 0x4000
#define IMEDDIATE_APPLY_FLAG 0x2000
extern uint16_t Flags;
extern uint8_t  Contrast;
extern MenuFunction_t  AfterContrast;
extern uint8_t MenuPos;
void ContrastMenu(void);
void LcdBlank(void);


#define DAC_V (DAC->DHR12R1)
#define DAC_I (DAC->DHR12R2)


void CalibrationMenu(void);
void StartFunction(void);
void SaveMenu(void); /* Save settings to flash menu */

#if defined(CLOCK_ENABLED)
int  SwitchOnTheClock(void);
void SetupTheClock(void);

/* 
RTCVal - seconds to display
InvPos - position that has to be inverted. If the first bit is 1 - display seconds also? else - onlu hours and minutes
Position - start coordinated to display the clock. It can have size 5 or 8 characters depend on InPos 1-st bit */
void DisplayClock(uint32_t RTCVal, uint16_t InvPos, uint32_t Position);
#define IS_ON_CLOCK ((RCC->BDCR & RCC_BDCR_RTCSEL) == RCC_BDCR_RTCSEL_0)
void TimerSetup(void);
extern uint16_t TimerValue;
extern uint16_t RemainTimerValue;
#endif

typedef struct
{
  uint16_t VoltageDAC;  // & Contrast
  uint16_t CurrentDAC;  // & Flafs
}Settings_t;

typedef struct
{
  float ADCRamp;
  float ADCOffset;
  float DACRamp;
  float DACOffset;
  int8_t DotPosition;
  int8_t Res[3];
}
SubConfig_t;

typedef struct
{
  uint16_t Value;
  int8_t   DotPosition;
#define MENU_I_FLAG 1
#define MENU_V_FLAG 2
  int8_t   IVFlag;
  char     Text[4];
}
UserMenu_t;

typedef struct
{
  SubConfig_t V;
  SubConfig_t I;
  UserMenu_t Menu[14];
  int32_t Crc;
}Config_t;

#define SAVED_SETTINGS_COUNT (1024 - sizeof(Config_t))/sizeof(Settings_t)
//#define SAVED_SETTINGS_COUNT 4
typedef struct 
{
  Settings_t   Settings[SAVED_SETTINGS_COUNT];
  Config_t     Config;
//  char res[1024-sizeof(Config_t) - SAVED_SETTINGS_COUNT*sizeof(Settings_t)];
} SavedDomain_t;

void OutValue(uint8_t Y, uint8_t X, uint16_t Num, uint8_t DotPosition, uint8_t SelectPos);
void OutValueSmall(uint8_t Y, uint8_t X, uint16_t Num, uint8_t DotPosition, uint8_t InverseFlag);
#if defined(GRAPH)
#undef RAW
#endif
#if defined(RAW)
extern uint8_t DisplayRaw;
#endif

//uint32_t Crc(const void* Pointer, int Len);
void SaveConfig(void);
int RestoreConfig(void); /* Return !0 if failed */
void SaveSettings(uint16_t AndFlash);

extern Config_t CurrentConfig;
extern Settings_t CurrentSettings;


int16_t VoltageFromAdc(void);
int16_t CurrentFromAdc(void);
int16_t VoltageFromDac(void);
int16_t CurrentFromDac(void);
extern int16_t HumanV;
extern int16_t HumanI;

#if defined(GRAPH)
#define GRAPH_SIZE 80
typedef struct
{
  int16_t Min;
  int16_t Max;
}
Graph_t;
extern Graph_t IGraphArray[GRAPH_SIZE];
extern Graph_t VGraphArray[GRAPH_SIZE];
extern uint8_t GraphCurrentPoint;
typedef struct
{
  Graph_t* GraphArray;
  int8_t   DotPosition;
} GraphData_t;
extern GraphData_t GraphData;
extern uint16_t TimeInterval;
void DisplayGraph(void);
void ClearGraph(void);
#endif /*FRAPH*/
#endif /*__LAB_ARM_H__*/

