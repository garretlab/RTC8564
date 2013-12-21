#include "RTC8564.h"
#include <Wire.h>

void RTC8564Class::begin() {
  struct dateTime dt = {0, 0, 0, 1, 1, 13, 2};  // 2013 January 1st, Tue 00:00:50

  begin(&dt);
};

void RTC8564Class::begin(struct dateTime *dt) {
  uint8_t control[2];
  struct alarmTime at = {0, 0, 0, 0};

  slaveAddress = RTCS8564_I2C_ADDRESS;
  
  delay(1000);     // Wait for 1 second to eusure the crystal to be stable.
  Wire.begin();
  
  control[0] = RTC8564_STOP_BIT, control[1] = 0x00;
  setRegisters(RTC8564_CONTROL1, 2, control);
    
  setDateTime(dt);                                      // Setup date and time.  setDateTime() sets STOP to 0
  setAlarm(RTC8564_AE_NONE, &at, false);       // Setup alarm interrupt.
  setClkoutFrequency(false, RTC8564_CLKOUT_32768HZ);    // Setup CLKOUT frequency.
  setTimer(false, false, RTC8564_CLK_244US, 0, false);                 // Setup timer interrupt.
}

void RTC8564Class::setDateTime(struct dateTime *dt) {
  uint8_t data[7];
  
  data[0] = RTC8564_STOP_BIT;
  setRegisters(RTC8564_CONTROL1, 1, data);
  
  data[0] = decimalToBCD(dt->second);
  data[1] = decimalToBCD(dt->minute);
  data[2] = decimalToBCD(dt->hour);
  data[3] = decimalToBCD(dt->day);
  data[4] = decimalToBCD(dt->weekday);
  data[5] = decimalToBCD(dt->month);
  if (dt->year > 100) {
    data[6] = decimalToBCD(dt->year - 100);
    data[5] |= RTCS8564_CAL_CENTURY;
  } else {
    data[6] = decimalToBCD(dt->year);
  }
  setRegisters(RTC8564_SECONDS, 7, data);

  data[0] = 0x00;
  setRegisters(RTC8564_CONTROL1, 1, data);
}

int RTC8564Class::getDateTime(struct dateTime *dt) {
  uint8_t data[7];
  
  getRegisters(RTC8564_SECONDS, 7, data);
  
  if (data[0] & RTCS8564_CAL_VL) {
    return -1;
  }
  
  dt->second  = BCDToDecimal(data[0] & 0x7f);
  dt->minute  = BCDToDecimal(data[1] & 0x7f);
  dt->hour    = BCDToDecimal(data[2] & 0x3f);
  dt->day     = BCDToDecimal(data[3] & 0x3f);
  dt->weekday = BCDToDecimal(data[4] & 0x07);
  dt->month   = BCDToDecimal(data[5] & 0x1f);
  dt->year    = BCDToDecimal(data[6]);
  
  if (data[5] & RTCS8564_CAL_CENTURY) {
    dt->year += 100;
  }
  
  return 0;
}

void RTC8564Class::setAlarm(uint8_t enableFlags, struct alarmTime *at, uint8_t interruptEnable) {
  uint8_t control2;
  uint8_t data[4];
  
  // Read Control2 register
  getRegisters(RTC8564_CONTROL2, 1, &control2);

  control2 &= (~RTC8564_AIE_BIT);
  setRegisters(RTC8564_CONTROL2, 1, &control2);

  data[0] = (enableFlags & RTC8564_AE_MINUTE)  ? decimalToBCD(at->minute)  : RTC8564_AE_BIT;
  data[1] = (enableFlags & RTC8564_AE_HOUR)    ? decimalToBCD(at->hour)    : RTC8564_AE_BIT;
  data[2] = (enableFlags & RTC8564_AE_DAY)     ? decimalToBCD(at->day)     : RTC8564_AE_BIT;
  data[3] = (enableFlags & RTC8564_AE_WEEKDAY) ? decimalToBCD(at->weekday) : RTC8564_AE_BIT;
  setRegisters(RTC8564_MINUTE_ALARM, 4, data);

  if (interruptEnable) {
    control2 |= RTC8564_AIE_BIT;
  } else {
    control2 &= ~RTC8564_AIE_BIT;
  }
  setRegisters(RTC8564_CONTROL2, 1, &control2);
}

void RTC8564Class::getAlarm(uint8_t *enableFlags, struct alarmTime *at) {
  uint8_t data[4];
  
  getRegisters(RTC8564_MINUTE_ALARM, 4, data);
  
  at->minute  = BCDToDecimal(data[0] & 0x7f);
  at->hour    = BCDToDecimal(data[1] & 0x3f);
  at->day     = BCDToDecimal(data[2] & 0x3f);
  at->weekday = BCDToDecimal(data[3] & 0x07);
  
  *enableFlags = 0;
  for (int i = 0; i < 4; i++) {
    if (!(data[i] & RTC8564_AE_BIT)) {
      *enableFlags |= (1 << i);
    }
  }
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

void RTC8564Class::setTimer(uint8_t enableFlag, uint8_t repeatMode, uint8_t clockMode, uint8_t counter, uint8_t interruptEnable) {
  uint8_t control2;
  uint8_t data[4];
    
  data[0] = 0;
  setRegisters(RTC8564_TIMER_CONTROL, 1, data);
  
  if (enableFlag) {
    getRegisters(RTC8564_CONTROL2, 1, &control2);
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
  } else {
    frequency &= ~RTCS8564_FE_BIT;
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

RTC8564Class RTC8564;
