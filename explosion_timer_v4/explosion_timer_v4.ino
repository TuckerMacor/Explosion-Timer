#include <EEPROM.h>

//output shift register(s)
#define shiftOutDataPin 0
#define shiftOutClockPin 1
#define shiftOutLatchPin 2
#define shiftOutCount 3
#define shiftOutPinCount shiftOutCount * 8
boolean shiftOutStates [shiftOutPinCount];

const boolean segmentNumbers[15][7] = {
  {1, 1, 1, 1, 1, 1, 0}, //0
  {0, 1, 1, 0, 0, 0, 0}, //1
  {1, 1, 0, 1, 1, 0, 1}, //2
  {1, 1, 1, 1, 0, 0, 1}, //3
  {0, 1, 1, 0, 0, 1, 1}, //4
  {1, 0, 1, 1, 0, 1, 1}, //5
  {1, 0, 1, 1, 1, 1, 1}, //6
  {1, 1, 1, 0, 0, 0, 0}, //7
  {1, 1, 1, 1, 1, 1, 1}, //8
  {1, 1, 1, 1, 0, 1, 1}, //9
  {0, 0, 0, 0, 0, 0, 0}, //off
  {1, 1, 1, 0, 1, 1, 1}, //A
  {1, 0, 0, 1, 1, 1, 1}, //E
  {1, 0, 1, 1, 0, 1, 1}, //S
  {1, 0, 0, 0, 1, 1, 1}, //F
};

#define pwrLedPin 8
#define actLedPin 7
#define outLedPin 6
#define outPinA 9
#define outPinB 10
#define buzzerPin 5
#define buttonPin A3
#define buttonCount 4

unsigned long lastButtonRead = 0;
bool buttonState[buttonCount];                    //what state is the button at
bool lastButtonState[buttonCount];                //previous state of the button

#define debounceDelay 10
int setTime = 25;
int outputOnTime = 700;

unsigned long countDownStart = 0;
unsigned long lastBeep = 0;

unsigned long lastBlink = 0;
#define ledBlinkDelayTime 300
bool ledBlinkState = HIGH;

bool blinkDecimalPoint[3] = {false, false, false};

int state = 0;
//0 = ready
//1 = set timer 100's
//2 = set timer 10's
//3 = set timer 1's
//4 = set on time 100's
//5 = set on time 10's
//6 = set on time 1's
//7 = counting down


void setup() {
  // put your setup code here, to run once:
  pinMode(shiftOutDataPin,  OUTPUT);
  pinMode(shiftOutClockPin, OUTPUT);
  pinMode(shiftOutLatchPin, OUTPUT);
  pinMode(pwrLedPin,        OUTPUT);
  pinMode(actLedPin,        OUTPUT);
  pinMode(outLedPin,        OUTPUT);
  pinMode(outPinA,          OUTPUT);
  pinMode(outPinB,          OUTPUT);

  int tmp;
  EEPROM.get(57, tmp);//57 and 55 are from a random number generator used to check if eeprom data is made by this program and is valid
  if (tmp != 55) {
    setTime = 25;
    outputOnTime = 750;
    EEPROM.put(10, setTime);
    EEPROM.put(20, outputOnTime);
    EEPROM.put(57, 55);
  }
  else {
    EEPROM.get(10, setTime);
    EEPROM.get(20, outputOnTime);
  }

  digitalWrite(pwrLedPin, HIGH);
  lightSequence();
  writeShiftSegmentMultiDigit(setTime);
}


void loop() {
  if ((millis() - lastButtonRead) > debounceDelay) {
    readButtons(analogRead(buttonPin));
  }

  if (millis() - lastBlink > ledBlinkDelayTime) {
    ledBlinkState = !ledBlinkState;
    for (int i = 0; i < 3; i++) {
      if (blinkDecimalPoint[i] == HIGH) {
        shiftOutStates[7 + (i) * 8] = !shiftOutStates[7 + (i) * 8];
      }
      else {
        shiftOutStates[7 + (i) * 8] = LOW;
      }
    }
    lastBlink = millis();
  }

  if (state == 7) {
    digitalWrite(actLedPin, HIGH);
    int countdown = ((millis() - countDownStart) / 1000);
    writeShiftSegmentMultiDigit(setTime - countdown);

    if ((millis() - lastBeep) > 1000) {
      tone(buzzerPin, 1200, 250);
      delay(100);
      lastBeep = millis();
    }

    if (((millis() - countDownStart) / 1000) > (setTime)) {
      tone(buzzerPin, 2000);
      digitalWrite(outLedPin, HIGH);
      delay(1000);
      digitalWrite(outPinA, HIGH);
      digitalWrite(outPinB, HIGH);
      delay(outputOnTime);
      digitalWrite(outPinA, LOW);
      digitalWrite(outPinB, LOW);
      digitalWrite(outLedPin, LOW);
      digitalWrite(actLedPin, LOW);
      state = 0;
      writeShiftSegmentMultiDigit(setTime);
    }
    noTone(buzzerPin);
  }

  shiftStatesOut();
}


void readButtons(int buttonLadderValue) {//check if buttons got pressed
  for (int i = 0; i < buttonCount; i++) {
    buttonState[i] = LOW;
  }
  if (buttonLadderValue > 120){
    digitalWrite(actLedPin, HIGH);
  }
  if (buttonLadderValue > 120 && buttonLadderValue < 300) {
    buttonState[3] = HIGH;
  }
  else if (buttonLadderValue > 300 && buttonLadderValue < 450) {
    buttonState[2] = HIGH;
  }
  else if (buttonLadderValue > 450 && buttonLadderValue < 750) {
    buttonState[1] = HIGH;
  }
  else if (buttonLadderValue > 750) {
    buttonState[0] = HIGH;
  }
  for (int i = 0; i < buttonCount; i++) {
    if (buttonState[i] != lastButtonState[i]) {
      lastButtonState[i] = buttonState[i];
      if (buttonState[i] == HIGH) {
        switch (i) {
          case 0:
            state++;
            if (state > 6) {
              state = 0;
            }
            if (state == 0) {
              blinkDecimalPoint[0] = false;
              blinkDecimalPoint[1] = false;
              blinkDecimalPoint[2] = false;
              writeShiftSegmentMultiDigit(setTime);
            }
            if (state == 1) {
              blinkDecimalPoint[0] = true;
              blinkDecimalPoint[1] = false;
              blinkDecimalPoint[2] = false;
              writeShiftSegmentMultiDigit(setTime);
            }
            else if (state == 2) {
              blinkDecimalPoint[0] = false;
              blinkDecimalPoint[1] = true;
              blinkDecimalPoint[2] = false;
              writeShiftSegmentMultiDigit(setTime);
            }
            else if (state == 3) {
              blinkDecimalPoint[0] = false;
              blinkDecimalPoint[1] = false;
              blinkDecimalPoint[2] = true;
              writeShiftSegmentMultiDigit(setTime);
            }
            else if (state == 4) {
              blinkDecimalPoint[0] = true;
              blinkDecimalPoint[1] = false;
              blinkDecimalPoint[2] = false;
              writeShiftSegmentMultiDigit(outputOnTime);
            }
            else if (state == 5) {
              blinkDecimalPoint[0] = false;
              blinkDecimalPoint[1] = true;
              blinkDecimalPoint[2] = false;
              writeShiftSegmentMultiDigit(outputOnTime);
            }
            else if (state == 6) {
              blinkDecimalPoint[0] = false;
              blinkDecimalPoint[1] = false;
              blinkDecimalPoint[2] = true;
              writeShiftSegmentMultiDigit(outputOnTime);
            }
            break;
          case 1:
            if (state > 0 && state < 4) {
              if (state == 1) {
                setTime = incrementOrDecrementDigit(setTime, -1, 2);
              }
              else if (state == 2) {
                setTime = incrementOrDecrementDigit(setTime, -1, 1);
              }
              else if (state == 3) {
                setTime = incrementOrDecrementDigit(setTime, -1, 0);
              }
              if (setTime < 3) {
                setTime = 999;
              }
              writeShiftSegmentMultiDigit(setTime);
            }
            else if (state > 3 && state < 7) {
              if (state == 4) {
                outputOnTime = incrementOrDecrementDigit(outputOnTime, -1, 2);
              }
              else if (state == 5) {
                outputOnTime = incrementOrDecrementDigit(outputOnTime, -1, 1);
              }
              else if (state == 6) {
                outputOnTime = incrementOrDecrementDigit(outputOnTime, -5, 0);
              }
              if (outputOnTime < 100) {
                outputOnTime = 950;
              }
              writeShiftSegmentMultiDigit(outputOnTime);
            }
            break;
          case 2:
            if (state > 0 && state < 4) {
              if (state == 1) {
                setTime = incrementOrDecrementDigit(setTime, 1, 2);
              }
              else if (state == 2) {
                setTime = incrementOrDecrementDigit(setTime, 1, 1);
              }
              else if (state == 3) {
                setTime = incrementOrDecrementDigit(setTime, 1, 0);
              }
              if (setTime > 999) {
                setTime = 3;
              }
              writeShiftSegmentMultiDigit(setTime);
            }
            else if (state > 3 && state < 7) {
              if (state == 4) {
                outputOnTime = incrementOrDecrementDigit(outputOnTime, 1, 2);
              }
              else if (state == 5) {
                outputOnTime = incrementOrDecrementDigit(outputOnTime, 1, 1);
              }
              else if (state == 6) {
                outputOnTime = incrementOrDecrementDigit(outputOnTime, 5, 0);
              }
              if (outputOnTime > 950) {
                outputOnTime = 100;
              }
              writeShiftSegmentMultiDigit(outputOnTime);
            }
            break;
          case 3:
            if (state == 7) {
              state = 0;
              writeShiftSegmentMultiDigit(setTime);
              digitalWrite(actLedPin, LOW);
            }
            else {
              state = 7;
              countDownStart = millis();
              EEPROM.put(10, setTime);
              EEPROM.put(20, outputOnTime);
            }
            break;
        }
        shiftStatesOut();
      }
    }
  }
  digitalWrite(actLedPin, LOW);
  lastButtonRead = millis();
}


int incrementOrDecrementDigit(int number, int amount, int place) {
  int current_digit = (int)(number / pow(10, place)) % 10;
  int new_digit;
  new_digit = current_digit + amount;
  int new_number = number - current_digit * pow(10, place) + new_digit * pow(10, place);
  return new_number;
}


void writeShiftSegmentMultiDigit(int valueToDisplay) {
  if (valueToDisplay > 999 || valueToDisplay < 0) {
    clearshiftOutStates();
    writeShiftSegment(0, 12);
  }
  else {
    writeShiftSegment(0, (valueToDisplay) / 100);
    writeShiftSegment(1, (((valueToDisplay) % 100) / 10));
    writeShiftSegment(2, (((valueToDisplay) % 100) % 10));
  }
  shiftStatesOut();
}


void writeShiftSegment(int whichDisplay, int Value) {
  for (int segment = 0; segment < 7; segment++) {
    shiftOutStates[segment + (whichDisplay) * 8] = segmentNumbers[Value][segment];
  }
}


void clearshiftOutStates() {
  for (int i = shiftOutPinCount - 1; i >=  0; i--) {
    shiftOutStates[i] = LOW;
  }
}


void shiftStatesOut() {
  digitalWrite(shiftOutClockPin, LOW);
  for (int i = shiftOutPinCount - 1; i >=  0; i--) {
    digitalWrite(shiftOutLatchPin, LOW);
    byte valueToShift = shiftOutStates[i];
    digitalWrite(shiftOutDataPin, valueToShift);
    digitalWrite(shiftOutLatchPin, HIGH);
  }
  digitalWrite(shiftOutClockPin, HIGH);
}


void lightSequence() {
  digitalWrite(actLedPin, HIGH);
  digitalWrite(outLedPin, HIGH);
  for (int i = 0; i < 3; i++) {
    noTone(8);
    tone(buzzerPin, 440 * (i + 1), 150);
    delay(150);
  }
  for (int i = 0; i < 10; i++) {
    writeShiftSegment(0, i);
    writeShiftSegment(1, i);
    writeShiftSegment(2, i);
    shiftStatesOut();
    delay(180);
  }
  digitalWrite(actLedPin, LOW);
  digitalWrite(outLedPin, LOW);
}
