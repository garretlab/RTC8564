#include <Wire.h>
#include "RTC8564.h"

int interruptPin = 12;

void setup() {
  struct dateTime dt = {0, 0, 0, 1, 1, 13, 2};
  struct alarmTime at = {1, 0, 1, 2};
  char s[20];
  uint8_t flags;

  pinMode(interruptPin, INPUT_PULLUP);
  Serial.begin(9600);
  RTC8564.begin(&dt);
  RTC8564.setAlarm(RTC8564_AE_MINUTE, &at, 1);
  RTC8564.setTimer(true, false, RTC8564_CLK_1SEC, 5, true);
}

void loop() {
  struct dateTime dt;
  char s[20];
  uint8_t data;
  
  if (RTC8564.getDateTime(&dt) == 0) {
    sprintf(s, "%4d/%02d/%02d %02d:%02d:%02d ", 
      dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    Serial.print(s);
  } else {
    RTC8564.begin();
    Serial.println("Detected voltage low");
  }
  
  if (digitalRead(interruptPin)) {
    Serial.print("HIGH ");
  } else {
    Serial.print("LOW  ");
  }

  if (RTC8564.getAlarmFlag()) {
    Serial.print("AF=ON,  ");
    RTC8564.clearAlarmFlag();
  } else {
    Serial.print("AF=OFF, ");
  }
  
  if (RTC8564.getTimerFlag()) {
    Serial.print("TF=ON,  ");
    RTC8564.clearTimerFlag();
  } else {
    Serial.print("TF=OFF, ");
  }

  Serial.println();
  delay(1000);
}

