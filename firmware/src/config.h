#pragma once

enum { 
  ledPort = 2, 
  hallSensorPort = 0, 
  pwmPort = 1, 
  turboAdcAndTurboButtonPort = 4, 
  turboAdcAndTurboButtonAdcChannel = 2, 
  normalAdcAndNormalButtonPort = 3, 
  normalAdcAndNormalButtonAdcChannel = 3, 
};

enum { pwmFreq = 500 };
#define msToTicksCount(msArg) (((U32)(msArg)) * pwmFreq / 1000)


enum { hallSensorFetchInterval = 64 }; 
static_assert((hallSensorFetchInterval & (hallSensorFetchInterval - 1)) == 0, "hallSensorFetchInterval should be power of 2");

enum { statusLedBlinksInterval = 32 }; 
static_assert((statusLedBlinksInterval & (statusLedBlinksInterval - 1)) == 0, "statusLedBlinksInterval should be power of 2");

//# low adc value that means short circuit to ground
enum {
  buttonPressThreshold = 10,  
  holdButtonPressTimeInterval = msToTicksCount(2000)
};   

//# hold it when powering device to start calibration
enum { calibrationButtonPort = turboAdcAndTurboButtonPort }; 


typedef U8 HallSensorTriggersCount;


enum { 
  kpShift = 7, 
  kiShift = 7
};

static_assert(kiShift < 8, "kiShift < 8");
static_assert(kpShift < 8, "kpShift < 8");

struct PidSettings {
  U8 kp;
  U8 ki;
};

enum { 
  pidDiffIntegralMin = -50, 
  pidDiffIntegralMax = 50 
};

struct Settings {
  U16 maxRpm;
  U8 minPwm;
  PidSettings pidSettings;
};

const Settings defaultSettings EEMEM = {
  .maxRpm = 1000, 
  .minPwm = 0, 
  .pidSettings = {
    .kp = U8(0.6 * (1 << kpShift)), 
    .ki = U8(0.3 * (1 << kiShift)), 
  }
};

enum {
  eepromSaveOffset = 0, 
  eepromSaveCopiesCount = 3
};

#ifdef __AVR_ATtiny13A__
  #define MCU_EEPROM_SIZE 64
#endif

static_assert(eepromSaveOffset + eepromSaveCopiesCount * sizeof(Settings) <= MCU_EEPROM_SIZE, "");
