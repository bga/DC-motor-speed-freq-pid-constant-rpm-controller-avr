#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
//#include <avr/wdt.h>
//#include <avr/sleep.h> 
//#include <util/delay.h> 

#include <!avr/timer.h> 
#include <!avr/eeprom.h> 

#include <!cpp/newKeywords.h> 
#include <!cpp/bitManipulations.h> 
#include <!cpp/RunningAvg.h> 
#include <!cpp/AddlerCrc.h> 

#include <!cpp/common.h> 

#include <!mcu/EepromManager.h> 

#include "config.h"

template<typename IntMinArg, typename IntVArg, typename IntMaxArg> 
IntVArg clampInt(IntMinArg min, IntVArg v, IntMaxArg max) {
  //return (max <= v) ? max : (v <= min) ? min : v;
  //return v;
  
  if(max <= v) {
    v = max; 
  }
  
  if(v <= min) {
    v = min; 
  }
  
  return v;
}


inline void Adc_init() {
  //# enable ADC, minimal prescaler
  ADCSRA = _BV(ADEN);
}

#define ADC_VREF_TYPE_1V1 _BV(REFS0)
#define ADC_VREF_TYPE_VCC 0
#define ADC_VREF_TYPE ADC_VREF_TYPE_VCC

//# read ADC low precision
U8 Adc_readU8(U8 adcChannel) {
  ADMUX = adcChannel | _BV(ADLAR) | ADC_VREF_TYPE;
  //_delay_us(10);
  //# start conversation
  ADCSRA |= _BV(ADSC);
  //# wait conversation end
  wait((ADCSRA & _BV(ADIF)) == 0);
  
  return ADCH;
}

void blinkHalf() {
  PINB = _BV(ledPort);
}

volatile HallSensorTriggersCount hallSensorTriggersCount = 1; 
ISR(PCINT0_vect) {
  hallSensorTriggersCount += 1;
}

typedef U16 TicksCount;
volatile TicksCount ticksCount = 0;

inline void delayMs(int msCount) {
  val startTime = ticksCount; wait((ticksCount - startTime) < msToTicksCount(msCount));
}

volatile U8 statusLedBlinksCount = 0;

#if 0
void StatusLed_blinkRaw(const unsigned int blinksCount) {
  statusLedBlinksCount += blinksCount;
}
inline void StatusLed_blink(const unsigned int blinksCount) {
  StatusLed_blinkRaw(2 * blinksCount);
}
#else 
void StatusLed_blink(const unsigned int blinksCount) {
  statusLedBlinksCount += 2 * blinksCount;
}
#endif

volatile HallSensorTriggersCount hallSensorTriggersCountTrail[8];
volatile U8 hallSensorTriggersCountTrailIndex = 0;
ISR(TIM0_OVF_vect){
  val ticksCountLocal = (ticksCount += 1);
  
  if(1) {
    var statusLedBlinksCountLocal = statusLedBlinksCount;
    if(statusLedBlinksCountLocal != 0 && (ticksCountLocal & (statusLedBlinksInterval - 1)) == 0) {
      blinkHalf();
      statusLedBlinksCountLocal -= 1;
      statusLedBlinksCount = statusLedBlinksCountLocal;
    }
    else {
      
    }
  }
  
  block {
    if((ticksCountLocal & (hallSensorFetchInterval - 1)) == 0) {
      //blinkHalf();
      var hallSensorTriggersCountTrailIndexLocal = hallSensorTriggersCountTrailIndex;
      hallSensorTriggersCountTrail[hallSensorTriggersCountTrailIndexLocal] = hallSensorTriggersCount;
      cycleInc(hallSensorTriggersCountTrailIndexLocal, arraySize(hallSensorTriggersCountTrail));
      hallSensorTriggersCountTrailIndex = hallSensorTriggersCountTrailIndexLocal;
    }
    else {
      
    }
  }
}

HallSensorTriggersCount getCurrentRpm() {
  var nextIndex = hallSensorTriggersCountTrailIndex; cycleInc(nextIndex, arraySize(hallSensorTriggersCountTrail));
  return hallSensorTriggersCountTrail[hallSensorTriggersCountTrailIndex] - hallSensorTriggersCountTrail[nextIndex];
}

#define timerPrescaler 256UL
#define pwmDutyCycleMax (F_CPU / (timerPrescaler * pwmFreq))  

void setPwmDutyCycle(U8 dutyCycle) {
  //assert(dutyCycle < pwmDutyCycleMax, "dutyCycle < pwmDutyCycleMax");
  OCR0B = dutyCycle;
}
U8 getPwmDutyCycle() {
  return OCR0B;
}

inline void setup() {
  //# set clock prescaler to 1
  CLKPR = 0x80; 
  CLKPR = 0x00;
  
  cli();
  
  //# pull up
  PORTB = _BV(hallSensorPort);
  DDRB = _BV(pwmPort) | _BV(ledPort);
  
  //# enable PCINT0 interrupt for { hallSensorPort }
  GIMSK = _BV(PCIE);
  PCMSK = _BV(hallSensorPort); 
  
  TCCR0A = TCCR0B = 0;
  
  const int cs = AVR_timerPrescalerToClockPrescalerBits(timerPrescaler);
  static_assert(cs != 0, "wrong timer clock prescaler");
  
  //prescale timer to 256   
  TCCR0B = cs | _BV(WGM02);    
  
  static_assert(pwmDutyCycleMax < 256, "timer counter should be less 256");
  static_assert(1 < pwmDutyCycleMax, "timer counter should be more than 1");
  OCR0A = pwmDutyCycleMax - 1;
  
  // turn on fast pwm mode
  TCCR0A = _BV(WGM01) | _BV(WGM00) | _BV(COM0B0) | _BV(COM0B1);

  //enable timer overflow interrupt   
  TIMSK0 = _BV(TOIE0);
  
  ADCSRA = _BV(ADEN);
  //ADCSRB &= 0xF8;
  
  sei();
}

struct EepromConfig {
  typedef AddlerCrc Crc; 
  typedef EEPROM_SizeT EepromSizeT;
  
  enum { copiesCount = eepromSaveCopiesCount };
  
  static void EepromApi_lockWriteAccess() {
    
  }
  static void EepromApi_unlockWriteAccess() {
    
  }
  static U8 EepromApi_readByte(EepromSizeT offset) {
    return EEPROM_read(eepromSaveOffset + offset);
  }
  static void EepromApi_writeByte(EepromSizeT offset, U8 v) {
    EEPROM_write(eepromSaveOffset + offset, v);
  }
  static void EepromApi_readBlock(U8 * data, EepromSizeT offset, EepromSizeT dataSize) {
    //for(U8 i = dataSize; i--;) {
    forInc(U8, i, 0, dataSize) {
      data[i] = EEPROM_read(eepromSaveOffset + offset + i);
    }
  }
  static void EepromApi_writeBlock(U8 const * data, EepromSizeT offset, EepromSizeT dataSize) {
    //for(U8 i = dataSize; i--;) {
    forInc(U8, i, 0, dataSize) {
      EEPROM_write(eepromSaveOffset + offset + i, data[i]);
    }
  }
};

typedef EepromManager<Settings, EepromConfig> AppEepromManager;

int __attribute__((noreturn)) main(); 
int main() {
  setup();
  
  //# fill eeprom sequentially
  if(0) {
    forInc(U8, i, 0, MCU_EEPROM_SIZE) {
      EEPROM_write(i, i + 1 - MCU_EEPROM_SIZE / 2);
    }
    
    wait(1);
  }
  
  //# read eeprom, flash N times
  if(0) {
    const U8 flashCount = EEPROM_read(0);

    #if 0
    U8 i = 2 * flashCount; while(i--) {
      blinkHalf();
      delayMs(1000);
    }
    #else
    forInc(U8, i, 0, 2 * flashCount) {
      blinkHalf();
      delayMs(1000);
    }
    #endif
    
    wait(1);
  }
  
  //# read adc, set pwm
  if(0) {
    RunningAvg<U8[8], U16> turboAdcRunningAvg;
    loop {
      turboAdcRunningAvg.add(Adc_readU8(turboAdcAndTurboButtonAdcChannel));
      setPwmDutyCycle((turboAdcRunningAvg.computeAvg() * pwmDutyCycleMax) >> 8);
      delayMs(50);
    }
  }
  
  AppEepromManager eepromManager;
  

  
  val loadSettings = [&]() {
    //EepromConfig::EepromApi_readBlock((U8 *)&settings, eepromSaveOffset, sizeof(settings));
  };
  val saveSettings = [&]() {
    eepromManager.writeData();
    //EepromConfig::EepromApi_writeBlock((U8 *)&settings, eepromSaveOffset, sizeof(settings));
  };
  
  block {
    val blockNo = eepromManager.searchLastRev();
    #if 0
    forInc(U8, i, 0, blockNo * 2) {
      blinkHalf();
      delayMs(255);
    }
    #else
    StatusLed_blink(blockNo * 2);
    #endif
    if(blockNo == AppEepromManager::faultOffset) {
      wait(1);
    }
  }
  
  Settings& settings = eepromManager.getData();
  
  //loadSettings();
  
  //# calibration
  block {
    val isCalibrationButtonPresses = [=]() {
      return PINB & _BV(calibrationButtonPort);
    };
    
    //# start with pressed turbo button. Do calibration
    if(isCalibrationButtonPresses()) {
      
      //# set pwm to 0 and increase until motor will start rotating
      setPwmDutyCycle(1);
      val lastHallSensorTriggersCount = hallSensorTriggersCount;
      loop {
        if(getPwmDutyCycle() == pwmDutyCycleMax) {
          break;
        }
        else if(lastHallSensorTriggersCount != hallSensorTriggersCount) {
          break;
        }
        else {
          delayMs(125);
          setPwmDutyCycle(getPwmDutyCycle() + 1);
          blinkHalf();
          //# wait 64 ticks
          //wait((ticksCount & 63) != 0); wait((ticksCount & 63) == 0);
        }
      }
      
      settings.minPwm = getPwmDutyCycle();
      
      //# set max rpm
      setPwmDutyCycle(pwmDutyCycleMax);
      loop {
        if(!isCalibrationButtonPresses()) {
          //# debounce
          //? wait 125 ms to allow compiler merge this call with prev { delayMs } call
          delayMs(125);
          if(!isCalibrationButtonPresses()) {
            break;
          }
          else {
            
          }
        }
        else {
          
        }
      }
      //# and measure it
      settings.maxRpm = getCurrentRpm();
      saveSettings();
    }
    else {
       
    }
  }
  
  //# main loop
  
  setBit(PORTB, ledPort);
  
  
  I8 pidDiffIntegral = 0;
  
  //# run motor
  setPwmDutyCycle(pwmDutyCycleMax / 2);
  //# wait for { hallSensorTriggersCountTrail } prefill
  wait(hallSensorTriggersCountTrailIndex != 0);
  
  U8 normalButtonAdc;
  U8 lastRealNormalButtonAdc = 0x7F;
  RunningAvg<U8[1], U16>  normalButtonAdcRunningAvg;
  RunningAvg<U8[1], U16>  turboButtonAdcRunningAvg;
  
  loop {
    //# wait next { hallSensorTriggersCountTrail } data
    // TODO merge with prev wait loop
    block {
      val lastHallSensorTriggersCountTrailIndex = hallSensorTriggersCountTrailIndex; wait(lastHallSensorTriggersCountTrailIndex == hallSensorTriggersCountTrailIndex);
    }
    
    val rpmToPwm = [&](U8 rpm) {
      return settings.minPwm + (((pwmDutyCycleMax - settings.minPwm) * rpm) >> 8);
    };
    
    val setPwmDutyCycleFromAdcValue = [&](U8 adcValue) {
      val currentRpm = getCurrentRpm();
      //val expectedRpm = (bm.adcRunningAvg.computeAvg() * settings.maxRpm) >> 8;
      val expectedRpm = (adcValue * settings.maxRpm) >> 8;
      
      val rpmDiff = expectedRpm - currentRpm;
      pidDiffIntegral += rpmDiff;
      pidDiffIntegral = clampInt(pidDiffIntegralMin, pidDiffIntegral, pidDiffIntegralMax);
      val pidRpmDiff = ((rpmDiff * settings.pidSettings.kp) >> kpShift) + ((pidDiffIntegral * settings.pidSettings.ki) >> kiShift); 
      setPwmDutyCycle(rpmToPwm(currentRpm + pidRpmDiff));
    };
    
    val normalButtonAdc = Adc_readU8(normalAdcAndNormalButtonAdcChannel);
    val turboButtonAdc = Adc_readU8(turboAdcAndTurboButtonAdcChannel);

    check(buttonPressThreshold <= normalButtonAdc) {
      normalButtonAdcRunningAvg.add(normalButtonAdc);
    }
    //check(buttonPressThreshold <= turboButtonAdc) {
      turboButtonAdcRunningAvg.add(turboButtonAdc);
    //}
    
    #if 0
    U8 adcValue;
    if(buttonPressThreshold <= normalButtonAdc) {
      adcValue = lastRealNormalButtonAdc = normalButtonAdc;
    }
    else {
      adcValue = Adc_readU8(turboAdcAndTurboButtonAdcChannel);
    }
    #else
    U8 adcValue;
    if(buttonPressThreshold <= normalButtonAdc) {
      adcValue = normalButtonAdcRunningAvg.computeAvg();
    }
    else {
      adcValue = turboButtonAdcRunningAvg.computeAvg();
    }
    #endif
    
    setPwmDutyCycleFromAdcValue(adcValue);

    
    //set_sleep_mode(SLEEP_MODE_IDLE); 
    //sleep_mode(); 
  }
}
