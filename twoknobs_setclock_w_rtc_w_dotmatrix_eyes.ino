/* dotMatix Clock
   by Jake Sherwood
   jakesherwood.com
   created January, 30, 2020
   last modified February 11, 2020
*/

/*
  ############
  librarys
  ############
    Uses RTCzero lib for builtin Nano RTC
    Encoder handling based off of Encoder Library - TwoKnobs Example
    http://www.pjrc.com/teensy/td_libs_Encoder.html
    uses the MD_Parola and MD_MAX72xx library for dot matrix display
    MD_MAX72XX library can be found at https://github.com/MajicDesigns/MD_MAX72XX
    uses MD_EyePair.h for googly eyes - taken from MAX72 example code

*/



#include <RTCZero.h>
#include <Encoder.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#include <SPI.h>
#include "MD_EyePair.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW

#define MAX_DEVICES 4

#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10

// Hardware SPI connection
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// connection for googly eyes
MD_MAX72XX M = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// pin numbers to the pins connected to your encoder(s).
Encoder modeKnob(A5, A7);  // left rotary
Encoder setKnob(2, 3);  //right rotary

//create rtc obj
RTCZero rtc;

//consts for rotary buttons
const int modeButton = A2;  // left rotary button attached to pin A2
const int setButton = A4;  // right rotary button attached to pin A4

// vars for rotary button change states
int modeButtonState;
int lastModeButtonState = LOW;
int modeReading;
int modeButtonPushCounter = 0; // counter for number of btn presses
int setButtonState;
int lastSetButtonState = LOW;
int setReading;
long newMode, newSet;

int feature = 1;  // default to set - selectFeature() func -
// feature select (set,displayTime, displayDate, displayAnimation)

int settingMode;  // used to pass the setting Mode (h,m,s,m,d,y) to dateTimeSelect() func


// debounce vars
unsigned long lastModeDebounceTime = 0; // last time modeButton pin was toggled
unsigned long lastSetDebounceTime = 0; // last time setButton pin was toggled
unsigned long debounceDelay = 5; // the debouce time in milliseconds; increase if output flickers
unsigned long lastClockDelay = 0;
unsigned long clockDelay = 5000; // delay for printing clock


const byte hours = 12;
const byte minutes = 0;
const byte seconds = 0;

const byte month = 2;
const byte day = 5;
const byte year = 20;

int selectHours;
int selectMinutes;
int selectSeconds;

int selectMonth;
int selectDay;
int selectYear;

int setSelectedVal;

//vars for dotmatrix clock
String myHours;
String myMinutes;
String mySeconds;

String myMonth;
String myDay;
String myYear;

String myTime;
String myDate;

String newNum;



// GOOGLY EYES
#define MAX_EYE_PAIR (MAX_DEVICES/2)
MD_EyePair E[MAX_EYE_PAIR];
// delay for eyes
#define  DELAYTIME  500  // in milliseconds - controls annimation speed




void setup() {

  Serial.begin(9600);

  // initialize rotary buttons
  pinMode(modeButton, INPUT_PULLUP);
  pinMode(setButton, INPUT_PULLUP);
 
  // initialize RTC
  rtc.begin();
  rtc.setHours(hours);
  rtc.setMinutes(minutes);
  rtc.setSeconds(seconds);

  //set the date
  rtc.setDay(day);
  rtc.setMonth(month);
  rtc.setYear(year);

  //initialize dotmatrix
  P.begin();

  // initialize the eye view
  M.begin();


}

long positionLeft  = -999;
long positionRight = -999;

void loop() {

  //####################
  // Rotary button bits
  //####################
  //read rotary button states
  modeReading = digitalRead(modeButton);
  setReading = digitalRead(setButton);

  // THE LEFT ROTARY
  // check mode button state and change
  if (modeReading != lastModeButtonState) {
    // reset mode debouce time
    lastModeDebounceTime = millis();
  }

  if ((millis() - lastModeDebounceTime) > debounceDelay) {
    //checking to see if debounceDelay has passed before checking state

    if (modeReading != modeButtonState) {
      modeButtonState = modeReading;

      if (modeButtonState == HIGH) {

        Serial.println("mode Button Pressed");
        if (feature == 1) { //only do this if in set feature / mode

          //clearing values here was causing too many bugs - figure out how to add longpress button
          setKnob.write(0);  // added back still not best UX
          //Serial.println("clear set val");
          modeButtonPushCounter++;
          Serial.print("number of mode button pushes: ");
          Serial.println(modeButtonPushCounter);





        }
        Serial.println(modeButtonPushCounter);

      }

    }


  }

  // save state for next time through the loop
  lastModeButtonState = modeReading;

  // THE RIGHT ROTARY
  // check set button state and change
  if (setReading != lastSetButtonState) {
    // reset mode debouce time
    lastSetDebounceTime = millis();
  }


  if ((millis() - lastSetDebounceTime) > debounceDelay) {
    //checking to see if debounceDelay has passed before checking state

    if (setReading != setButtonState) {
      setButtonState = setReading;

      if (setButtonState == HIGH) {
        if (feature == 1) { //only do this if in set feature / mode

          Serial.println("set Button Pressed");
          Serial.print("my mode #");
          Serial.print(newMode);
          Serial.print(" to set are = ");
          Serial.println(setSelectedVal);
          timeSet(settingMode, setSelectedVal);

        }

      }

    }


  }

  // save state for next time through the loop
  lastSetButtonState = setReading;


  // select mode/feature based on left rotary
  switch (feature) {
    case 0: // no feature
      break;
    case 1: // set mode;
      setAndDisplay(modeButtonPushCounter);
      break;
    case 2: // time mode
      concatClock();
      break;
    case 3: // date mode
      break;
    case 4: // googly eye annimation mode
      for (uint8_t i = 0; i < MAX_EYE_PAIR; i++)
        E[i].animate();
      break;
  }

  //####################
  // Rotary Encoder bits
  //####################
  newMode = constrain(modeKnob.read() / 4, 0, 6);
  if (newMode != positionLeft) {
    positionLeft = newMode;
    featureSelect(newMode);

  }

  newSet = setKnob.read() / 4;
  if (newSet != positionRight) {
    positionRight = newSet;
    dateTimeSelect(settingMode, newSet);

  }



  //####################
  // RTC bits
  //####################
  // serial moniter clock display
  if ((millis() - lastClockDelay) > clockDelay) {
    //Print date
    print2digits(rtc.getMonth());
    Serial.print("/");
    print2digits(rtc.getDay());
    Serial.print("/");
    print2digits(rtc.getYear());
    Serial.print(" ");
    //then time
    int twelveHours;  // use this var to display hours in 12 hour / standard time
    if (rtc.getHours() > 12) {
      twelveHours  = rtc.getHours() - 12;
    } else if (rtc.getHours() == 0) {
      twelveHours  = rtc.getHours() + 12;
    } else {
      twelveHours  = rtc.getHours();
    }

    print2digits(twelveHours);
    Serial.print(":");
    print2digits(rtc.getMinutes());
    Serial.print(":");
    print2digits(rtc.getSeconds());

    Serial.println();

    lastClockDelay = millis();
  }
  // end serial moniter clock display

  //####################
  //dot matrix bits
  //####################
  //concatClock();
  //this is actually all on the setAndDisplay() func

}

// select clock feature (set, displayTime, displayDate, annimation)
void featureSelect (int thisFeature) {
  if (feature > 3 || feature < 1 ) {
    modeKnob.write(1);
  }
  switch (thisFeature) {
    case 0:  // not a mode
      break;
    case 1: // select set feature
      Serial.println("in set mode");
      Serial.println(thisFeature);
      feature = 1;
      P.displayText("HELLO", PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
      break;
    case 2: // select time mode
      Serial.println("in time mode");
      Serial.println(thisFeature);
      feature = 2;
      P.displayText("HELLO", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
      concatClock();
      break;
    case 3: // select date mode
      Serial.println("in date mode");
      Serial.println(thisFeature);
      feature = 3;
      P.displayText("HELLO", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
      concatDate();
      break;
    case 4: // select animation mode
      Serial.println("in animation mode");
      Serial.println(thisFeature);
      feature = 4;
      goGoogly();
      break;
  }
}

// select and dislpay hour, minute, seconds, month, day, year while selecting
// RTC set functions happen on set (Right) rotary btn press
void setAndDisplay(int thisModeButtonPushCounter) {
  switch (thisModeButtonPushCounter) {
    case 0:  // default - show clock
      concatClock();
      break;
    case 1: // set hours
      { String selectHoursCat;
        
        if (newSet < 10) {
          selectHoursCat.concat("0");
          selectHoursCat.concat(newSet);
        } else {
          selectHoursCat.concat(newSet);
        }

        selectHoursCat.concat(":");
        if (myMinutes.toInt() < 10) {
          selectHoursCat.concat("0");
          selectHoursCat.concat(myMinutes);
        } else {
          selectHoursCat.concat(myMinutes);
        }
        selectHoursCat.concat("h");

        P.print(selectHoursCat);
        settingMode = 1;
        //        timeSet(1, newSet);

      }
      break;
    case 2:  // set minutes
      { String selectMinutesCat;
        String selectMinCatHours = String(rtc.getHours());
        if (selectMinCatHours.toInt() < 10) {
          selectMinutesCat.concat("0");
          selectMinutesCat.concat(selectMinCatHours);
        } else {
          selectMinutesCat.concat(selectMinCatHours);
        }

        selectMinutesCat.concat(":");
        if (newSet < 10) {
          selectMinutesCat.concat("0");
          selectMinutesCat.concat(newSet);
        } else {
          selectMinutesCat.concat(newSet);
        }

        selectMinutesCat.concat("m");

        P.print(selectMinutesCat);
        settingMode = 2;
        //        timeSet(2, newSet);
      }

      break;
    case 3: // select seconds mode
      //      Serial.println("seconds set mode");
      P.displayText("HELLO", PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
      { String selectSecondsCat;


        selectSecondsCat.concat(":");
        if (newSet < 10) {
          selectSecondsCat.concat("0");
          selectSecondsCat.concat(newSet);
        } else {
          selectSecondsCat.concat(newSet);
        }
        selectSecondsCat.concat("s");

        P.print(selectSecondsCat);

      }
      // modeSelect(3);
      settingMode = 3;
      break;
    case 4: // select month mode
      // modeSelect(4);
      //      Serial.println("month set mode");
      P.displayText("HELLO", PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
      { String selectMonthCat;


        if (newSet < 10) {
          selectMonthCat.concat("0");
          selectMonthCat.concat(newSet);
        } else {
          selectMonthCat.concat(newSet);
        }
        selectMonthCat.concat("m");

        P.print(selectMonthCat);

      }
      settingMode = 4;
      break;
    case 5: // select day mode
      // modeSelect(5);
      //      Serial.println("day set mode");
      P.displayText("HELLO", PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
      { String selectDayCat;


        if (newSet < 10) {
          selectDayCat.concat("0");
          selectDayCat.concat(newSet);
        } else {
          selectDayCat.concat(newSet);
        }
        selectDayCat.concat("d");

        P.print(selectDayCat);

      }
      settingMode = 5;
      break;
    case 6: // select year mode
      // modeSelect(6);
      //      Serial.println("year set mode");
      P.displayText("HELLO", PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
      { String selectYearCat;


        selectYearCat.concat("20");
        if (newSet < 10) {
          selectYearCat.concat("0");
          selectYearCat.concat(newSet);
        } else {
          selectYearCat.concat(newSet);
        }
        selectYearCat.concat("y");

        P.print(selectYearCat);

      }

      settingMode = 6;
      break;
    case 7:
      settingMode = 1;
      modeButtonPushCounter = 0;
      Serial.print("mode button set back to 0");
      Serial.println(modeButtonPushCounter);
      break;


  }
}

void dateTimeSelect(int mode, int setVal) {
  //void timeSet(int mode) {
  switch (mode) {
    case 0:  // not a mode
      break;
    case 1: // set hours
      {
        //still trying to figure out how to constrain rotary
        if (setVal > 23) {
          setKnob.write(1);
        }

        selectHours = setKnob.read();
        if (selectHours < 1 ) {
          setKnob.write(1);
        }

        selectHours = constrain(setVal, 1, 23);
        Serial.print("hours = ");
        Serial.println(selectHours);
        setSelectedVal = selectHours;

      }
      break;
    case 2: // select minutes
      {
        if (setVal > 59) {
          setKnob.write(1);
        }
        selectMinutes = constrain(setVal, 0, 59);
        Serial.print("minutes = ");
        Serial.println(selectMinutes);
        setSelectedVal = selectMinutes;
      }
      break;
    case 3: // select seconds
      {
        if (setVal > 59) {
          setKnob.write(1);
        }
        selectSeconds = constrain(setVal, 0, 59);
        Serial.print("seconds = ");
        Serial.println(selectSeconds);
        setSelectedVal = selectSeconds;
      }
      break;
    case 4: // select month
      {
        if (setVal > 12) {
          setKnob.write(1);
        }
        selectMonth = constrain(setVal, 0, 12);
        Serial.print("month = ");
        Serial.println(selectMonth);
        setSelectedVal = selectMonth;
      }
      break;
    case 5: // select day
      {
        if (setVal > 31) {
          setKnob.write(1);
        }
        selectDay = constrain(setVal, 0, 31);
        Serial.print("day = ");
        Serial.println(selectDay);
        setSelectedVal = selectDay;
      }
      break;
    case 6: // select year
      {
        if (setVal > 100) {
          setKnob.write(1);
        }
        selectYear = constrain(setVal, 0, 100);
        Serial.print("year = ");
        Serial.println(selectYear);
        setSelectedVal = selectYear;
      }
      break;
      //    default;


  }

}


void timeSet(int thisMode, int selectedVal) {
  switch (thisMode) {
    case 0: // do nothing
      break;
    case 1: //set hours
      rtc.setHours(selectedVal);
      break;
    case 2: //set minutes
      rtc.setMinutes(selectedVal);
      break;
    case 3: //set seconds
      rtc.setSeconds(selectedVal);
      break;
    case 4: //set month
      rtc.setMonth(selectedVal);
      break;
    case 5: //set day
      rtc.setDay(selectedVal);
      break;
    case 6: //set year
      rtc.setYear(selectedVal);
      break;
  }
}


void print2digits(int number) {
  if (number < 10) {
    Serial.print("0"); // add 0 if the time < 10
  }
  Serial.print(number);
  //return number;

}


String print2digitsNew(int thisNumber) {
  //  newNum == '';
  if (thisNumber < 10) {
    newNum.concat("0"); // add 0 if the time < 10
    newNum.concat(String(thisNumber));
  } else {
    newNum.concat(String(thisNumber));
  }

  return newNum;

}



void concatClock() {

  int twelveHours;  // use this var to display hours in 12 hour / standard time
  if (rtc.getHours() > 12) {
    twelveHours  = rtc.getHours() - 12;
  } else if (rtc.getHours() == 0) {
    twelveHours  = rtc.getHours() + 12;
  } else {
    twelveHours  = rtc.getHours();
  }

  myHours = String(twelveHours);
  int myHoursInt = rtc.getHours(); // keep hours as int to add 0 if < 10

  myMinutes = String(rtc.getMinutes());
  int myMinutesInt = rtc.getMinutes();  // keep minutes as int to add 0 if < 10

  mySeconds = String(rtc.getSeconds());

  myTime = "";
  if (myHoursInt < 10) {
    myTime.concat("0");
    myTime.concat(myHours);
  } else {

    myTime.concat(myHours);
  }
  //    myTime.concat(myHours);
  myTime.concat(":");
  if (myMinutesInt < 10) {
    myTime.concat("0");
    myTime.concat(myMinutes);
  } else {

    myTime.concat(myMinutes);
  }

  P.print(myTime);

}

void concatDate() {
  myMonth = String(rtc.getMonth());
  myDay = String(rtc.getDay());

  myDate = "";
  //myDate = String(myMonth);
  if (myMonth.toInt() < 10) {
    myDate.concat("0");
    myDate.concat(myMonth);
  } else {

    myDate.concat(myMonth);
  }


  myDate.concat("/");
  if (myDay.toInt() < 10) {
    myDate.concat("0");
    myDate.concat(myDay);
  } else {

    myDate.concat(myDay);
  }
  // trying to rework print2digits func to minimize code reuse - failing 20200210
  // myDay = print2digitsNew(newNum);
  // myDate.concat(myDay);

  P.print(myDate);

}

void goGoogly() {

  P.displayReset();
  M.clear();
  for (uint8_t i = 0; i < MAX_EYE_PAIR; i++)
    E[i].begin(i * 2, &M, DELAYTIME);

}
