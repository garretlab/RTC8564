#include "RTC8564.h"
#include <Wire.h>

void RTC8564Class::begin() {
  struct dateTime dt = {50, 0, 0, 1, 1, 13, 2};  // 2013 January 1st, Tue 00:00:50

  begin(&dt);
};

void RTC8564Class::begin(struct dateTime *dt) {
  uint8_t control[2];
  struct alarmTime at = {0, 0, 0, 0};

  slaveAddress = 0x51;  // = 0xa2 >> 1
  
  delay(1000);     // Wait for 1 second to eusure the crystal to be stable.
  Wire.begin();
  
  control[0] = RTC8564_STOP_BIT, control[1] = 0x00;
  setRegisters(RTC8564_CONTROL1, 2, control);
    
  setDateTime(dt);                                      // Setup date and time.  setDateTime() sets STOP to 0
  setAlarmInterrupt(RTC8564_AE_NONE, &at, false);       // Setup alarm interrupt.
  setClkoutFrequency(false, RTC8564_CLKOUT_32768HZ);    // Setup CLKOUT frequency.
  setTimerInterrupt(false, false, RTC8564_CLK_244US, 0, false);                 // Setup timer interrupt.
}

void RTC8564Class::setDateTime(struct dateTime *dt) {
  uint8_t data[7];
  
  data[0] = RTC8564_STOP_BIT;
  setRegisters(RTC8564_CONTROL1, 1, data);
  
  data[0] = decimalToBCD(dt->seconds);
  data[1] = decimalToBCD(dt->minutes);
  data[2] = decimalToBCD(dt->hours);
  data[3] = decimalToBCD(dt->days);
  data[4] = decimalToBCD(dt->weekdays);
  data[5] = decimalToBCD(dt->months);
  data[6] = decimalToBCD(dt->years);
  setRegisters(RTC8564_SECONDS, 7, data);

  data[0] = 0x00;
  setRegisters(RTC8564_CONTROL1, 1, data);
}

int RTC8564Class::getDateTime(struct dateTime *dt) {
  uint8_t data[7];
  
  getRegisters(RTC8564_SECONDS, 7, data);
  
  if (data[0] & 0x80) {
    return -1;
  }
  
  dt->seconds  = BCDToDecimal(data[0] & 0x7f);
  dt->minutes  = BCDToDecimal(data[1] & 0x7f);
  dt->hours    = BCDToDecimal(data[2] & 0x3f);
  dt->days     = BCDToDecimal(data[3] & 0x3f);
  dt->weekdays = BCDToDecimal(data[4] & 0x07);
  dt->months   = BCDToDecimal(data[5] & 0x1f);
  dt->years    = BCDToDecimal(data[6]);
  
  if (data[5] & 0x80) {
    dt->years += 100;
  }
  
  return 0;
}

void RTC8564Class::setAlarmInterrupt(uint8_t enableFlags, struct alarmTime *dt, uint8_t interruptEnable) {
  uint8_t control2;
  uint8_t data[4];
  
  // Read Control2 register
  getRegisters(RTC8564_CONTROL2, 1, &control2);

  control2 &= (~RTC8564_AIE_BIT);
  setRegisters(RTC8564_CONTROL2, 1, &control2);

  data[0] = (enableFlags & RTC8564_AE_MINUTE)  ? decimalToBCD(dt->minutes)  : RTC8564_AE_BIT;
  data[1] = (enableFlags & RTC8564_AE_HOUR)    ? decimalToBCD(dt->hours)    : RTC8564_AE_BIT;
  data[2] = (enableFlags & RTC8564_AE_DAY)     ? decimalToBCD(dt->days)     : RTC8564_AE_BIT;
  data[3] = (enableFlags & RTC8564_AE_WEEKDAY) ? decimalToBCD(dt->weekdays) : RTC8564_AE_BIT;
  setRegisters(RTC8564_MINUTE_ALARM, 4, data);

  if (interruptEnable) {
    control2 |= RTC8564_AIE_BIT;
  } else {
    control2 &= ~RTC8564_AIE_BIT;
  }
  setRegisters(RTC8564_CONTROL2, 1, &control2);
}

int RTC8564Class::getAlarmFlag() {
  uint8_t control2;

  getRegisters(RTC8564_CONTROL2, 1, &control2);

  return (control2 & RTC8564_AF_BIT) ? true : false;
}  

void RTC8564Class::clearAlarmFlag() {
  uint8_t control2;

  getRegisters(RTC8564_CONTROL2, 1, &control2);
  control2 &= ~RTC8564_AF_BIT;
  setRegisters(RTC8564_CONTROL2, 1, &control2);
}  

void RTC8564Class::setTimerInterrupt(uint8_t enableFlag, uint8_t repeatMode, uint8_t clockMode, uint8_t counter, uint8_t interruptEnable) {
  uint8_t control2;
  uint8_t data[4];
  
  getRegisters(RTC8564_CONTROL2, 1, &control2);
  
  data[0] = 0;
  setRegisters(RTC8564_TIMER_CONTROL, 1, data);
  
  control2 &= ~(RTC8564_TITP_BIT | RTC8564_TF_BIT | RTC8564_TIE_BIT);
  setRegisters(RTC8564_CONTROL2, 1, &control2);
  
  if (repeatMode) {
    control2 |= RTC8564_TITP_BIT;
  }
  
  if (interruptEnable) {
    control2 |= RTC8564_TIE_BIT;
  }
  setRegisters(RTC8564_CONTROL2, 1, &control2);
  setRegisters(RTC8564_TIMER, 1, &counter);
  
  if (enableFlag) {
    clockMode |= RTC8564_TE_BIT;
    setRegisters(RTC8564_TIMER_CONTROL, 1, &clockMode);
  }
}

int RTC8564Class::getTimerFlag() {
  uint8_t control2;

  getRegisters(RTC8564_CONTROL2, 1, &control2);

  return (control2 & RTC8564_TF_BIT) ? true : false;
}  

void RTC8564Class::clearTimerFlag() {
  uint8_t control2;

  getRegisters(RTC8564_CONTROL2, 1, &control2);
  control2 &= ~RTC8564_TF_BIT;
  setRegisters(RTC8564_CONTROL2, 1, &control2);
}

void RTC8564Class::setClkoutFrequency(uint8_t enableFlag, uint8_t frequency) {
  if (enableFlag) {
    frequency |= RTCS8564_FE_BIT;
  }
  
  setRegisters(RTC8564_CLKOUT_FREQUENCY, 1, &frequency);
}

inline int RTC8564Class::decimalToBCD(int decimal) {
  return (((decimal / 10) << 4) | (decimal % 10));
}

inline int RTC8564Class::BCDToDecimal(int bcd) {
  return ((bcd >> 4) * 10 + (bcd & 0x0f));
}

inline int RTC8564Class::waitForData() {
  while(Wire.available() < 1) {
    ;
  }
}

void RTC8564Class::setRegisters(uint8_t address, int numData, uint8_t *data) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(address);
  Wire.write(data, numData);
  Wire.endTransmission();
}

void RTC8564Class::getRegisters(uint8_t address, int numData, uint8_t *data) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(slaveAddress, numData);
  
  for (int i = 0; i < numData; i++) {
    waitForData();
    data[i] = Wire.read();
  }
}
