#include <string.h>
#include "LabArm.h"

#if defined(REAL_FLASH)
#if !defined(__GNUC__)
const SavedDomain_t SavedDomain[2] __attribute__ ((at(FLASH_BASE+1024*13)));
#else
const SavedDomain_t SavedDomain[2] __attribute__ ((section(".eb0rodata")));
//const SavedDomain_t SavedDomain[2] __attribute__ ((aligned(1024)))={{{{0}}}};
#endif
const Settings_t*  NextSettings;

#define RDP_Key                  ((uint16_t)0x00A5)
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)

static uint32_t Crc(const void* Pointer, int Len)
{
  int i;
  const uint8_t* Data = (const uint8_t*)Pointer;
  uint32_t Ret = 0x55;

#if 1
  for (i = 0; i<Len; i++)
  {
    Ret = Ret ^ (uint32_t)(Data[i]);
  }
#else
  CRC->CR |= CRC_CR_RESET;
  CRC->IDR = Ret;
  for (i = 0; i<Len; i++)
  {
    CRC->IDR = Data[i];
    Ret = CRC->IDR;
  }
#endif

  return Ret;
}


void WriteFlash(void* Src, void* Dst, int Len)
{
  uint16_t* SrcW = (uint16_t*)Src;
  volatile uint16_t* DstW = (uint16_t*)Dst;

  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;

  FLASH->CR |= FLASH_CR_PG; /* Programm the flash */
  while (Len)
  {
    *DstW = *SrcW;
    while ((FLASH->SR & FLASH_SR_BSY) != 0 )
      ;
    if (*DstW != *SrcW )
    {
      break;
    }
    DstW++;
    SrcW++;
    Len = Len - sizeof(uint16_t);
  }

  FLASH->CR &= ~FLASH_CR_PG; /* Reset the flag back !!!! */
  FLASH->CR |= FLASH_CR_LOCK; /* Lock the flash back */
}
void ErasePage(void* Addr)
{
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;

  FLASH->CR |= FLASH_CR_PER; /* Page erase */
  FLASH->AR = (uint32_t)Addr; 
  FLASH->CR|= FLASH_CR_STRT; /* Start erase */
  while ((FLASH->SR & FLASH_SR_BSY) != 0 ) /* Wait end of eraze */
    ;
  FLASH->CR &= ~FLASH_CR_PER; /* Page erase end */
}
void SaveConfig(void)
{
  const SavedDomain_t* ValidDomain;

  CurrentConfig.Crc = Crc(&CurrentConfig, sizeof(Config_t) - sizeof(uint32_t));

  ValidDomain = SavedDomain;
  /* Clear 2 page */
  ErasePage((void*)SavedDomain); 
  ErasePage((void*)(SavedDomain+1)); 

  /* Save header and first settings */
  WriteFlash(&CurrentConfig, (void*)&ValidDomain->Config, sizeof(Config_t));

  NextSettings = &ValidDomain->Settings[0];
  SaveSettings(1);
  NextSettings = &ValidDomain->Settings[1]; /* Prepare pointer to save at power fault */
}

void SaveSettings(uint16_t AndFlash)
{
  if ( (DAC->DOR1 != 0 &&  DAC->DOR2 != 0) ) /* Is the out ON */
  {
    CurrentSettings.VoltageDAC = DAC->DOR1; /* Restore real current value */
    CurrentSettings.CurrentDAC = DAC->DOR2;
  }
  /* Else - save preset values */

  CurrentSettings.CurrentDAC |= Flags; /* Adding flags to save */
  CurrentSettings.VoltageDAC |= (Contrast<<12);
  if (Flags & BACKUP_SAVE_FLAG)
  {
    BKP->DR10 = CurrentSettings.VoltageDAC;
    BKP->DR9  = CurrentSettings.CurrentDAC;
    if (AndFlash == 0 )
      goto Restore;
  }
  if ( NextSettings  == NULL )
    return;
  WriteFlash(&CurrentSettings, (void*)NextSettings, sizeof(CurrentSettings));

Restore:
  CurrentSettings.CurrentDAC &= 0x0FFF; /* Remove flags back */
  CurrentSettings.VoltageDAC &= 0x0FFF;
}


int RestoreConfig(void) /* Return !0 if failed */
{
  const Settings_t* CurSettings;
  const Settings_t* PrevSettings;
  const SavedDomain_t* ValidDomain;
 
  ValidDomain = SavedDomain;
  if ( Crc( &ValidDomain->Config, sizeof(Config_t) - sizeof(uint32_t) ) != ValidDomain->Config.Crc )
  {
    ValidDomain = SavedDomain + 1;
    if ( Crc( &ValidDomain->Config, sizeof(Config_t) - sizeof(uint32_t) ) != ValidDomain->Config.Crc )
    {
      return -1; /* Both blocks are invalid */
    }
  }
  memcpy((char*)&CurrentConfig, (char*)&ValidDomain->Config, sizeof(CurrentConfig));

  PrevSettings = ValidDomain->Settings;

  for ( CurSettings = PrevSettings; CurSettings < ValidDomain->Settings + SAVED_SETTINGS_COUNT; CurSettings++)
  {
    if ( CurSettings->VoltageDAC == 0xFFFF && CurSettings->CurrentDAC == 0xFFFF )
	  break;
    PrevSettings = CurSettings;
  }

  memcpy(&CurrentSettings, PrevSettings, sizeof(CurrentSettings));
#if 0
  Flags = CurrentSettings.CurrentDAC & 0xF000;
  CurrentSettings.CurrentDAC &= 0x0FFF;
  Contrast = CurrentSettings.VoltageDAC>>12;
  CurrentSettings.VoltageDAC &= 0x0FFF;
#endif

  NextSettings = CurSettings;
  if ( CurSettings == ValidDomain->Settings + SAVED_SETTINGS_COUNT ) // All space is full
  {
    const SavedDomain_t* NextDomain = SavedDomain;

    if ( ValidDomain == SavedDomain )
      NextDomain = SavedDomain + 1;

    WriteFlash((void*)&ValidDomain->Config, (void*)&NextDomain->Config, sizeof(Config_t));
    WriteFlash((void*)CurSettings, (void*)&NextDomain->Settings[0], sizeof(Settings_t));
    NextSettings = &NextDomain->Settings[1];
    ErasePage( (void*) ValidDomain );
    ValidDomain = NextDomain;
  }
  
  if ( BKP->DR9 & BACKUP_SAVE_FLAG )
  {
    /* restore from backup domain */
    CurrentSettings.VoltageDAC = BKP->DR10;
    CurrentSettings.CurrentDAC = BKP->DR9;
  }

  Flags = CurrentSettings.CurrentDAC&0xF000;
  CurrentSettings.CurrentDAC &= 0x0FFF;
  Contrast = CurrentSettings.VoltageDAC>>12;
  CurrentSettings.VoltageDAC &= 0x0FFF;
  LcdContrast ( Contrast*3+42 );

  return 0;
}

#else  /* REAL_FLASH */
void SaveConfig(void)
{
}

int RestoreConfig(void)
{
  return -1;
}

void SaveSettings(void)
{
}

#endif /* RAL_FLASH */

