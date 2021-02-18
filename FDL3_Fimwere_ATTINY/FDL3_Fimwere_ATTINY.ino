#include <SoftRcPulseOut.h>
#define NOW

SoftRcPulseOut esc;
const int buttonPin = 4;
const int escPin = 2; //Uno 5 // ATtiny 2
const int ledPin = 1; // attiny 1 uno LED_BUILTIN

// constants:
const int motorStop = 20;
const int motorLow = 90;
const int motorMiddle = 100;
const int motorHigh = 110; // max 179
// variables:
int wantedMotorSpeed = motorStop;
int currentMotorSpeed = motorStop;


void setup() {
  
  //pinMode(buttonPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP); // ATTIny
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  esc.attach(escPin);
  delay(1000);
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
}

void loop() {
   // Get button event and act accordingly
   int b = checkButton();
   if (b == 1) clickEvent();
   if (b == 2) doubleClickEvent();
   if (b == 3) holdEvent();
   if (b == 4) longHoldEvent();
   if (b == 5) clickAndHoldEvent();
   if (b == 6) releaseEvent();
   runMotor();
}

void runMotor() {
  if (SoftRcPulseOut::refresh())
  {
    if (currentMotorSpeed < wantedMotorSpeed ) { // need to accelerate
      currentMotorSpeed = currentMotorSpeed + 1;
      esc.write(currentMotorSpeed);
      delay(20);
    } else if (currentMotorSpeed > wantedMotorSpeed ) { // need to decelerate
      currentMotorSpeed = currentMotorSpeed - 1;
      esc.write(currentMotorSpeed);
      delay(20);
    } else { // keep the speed
      esc.write(wantedMotorSpeed);
      currentMotorSpeed = wantedMotorSpeed;
      delay(20);
    }
  }
}

boolean isMotorRunning() {
  if (currentMotorSpeed == motorStop) {
    return false;
  }
  return true;
}

void clickEvent() {
}

void doubleClickEvent() {
  wantedMotorSpeed = motorHigh;
}

void holdEvent() {
  if (wantedMotorSpeed != motorHigh) {
    wantedMotorSpeed = motorLow;
  }
  digitalWrite(ledPin, HIGH);
}


void clickAndHoldEvent() {
  wantedMotorSpeed = motorMiddle;
  digitalWrite(ledPin, HIGH);
}

void longHoldEvent() {
}

void releaseEvent() {
  if (isMotorRunning()) {
   wantedMotorSpeed = motorStop;
   digitalWrite(ledPin, LOW);
  }
}


/* 
 *  Added by dukeem:
 *  status 5: click - release - hold
 *  status 6: release

4-Way Button:  Click, Double-Click, Press+Hold, and Press+Long-Hold Test Sketch

By Jeff Saltzman
Oct. 13, 2009

To keep a physical interface as simple as possible, this sketch demonstrates generating four output events from a single push-button.
1) Click:  rapid press and release
2) Double-Click:  two clicks in quick succession
3) Press and Hold:  holding the button down
4) Long Press and Hold:  holding the button for a long time 
*/
//=================================================
//  MULTI-CLICK:  One Button, Multiple Events

// Button timing variables
int debounce = 20;          // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 200;            // max ms between clicks for a double click event
int holdTime = 200;        // ms hold period: how long to wait for press+hold event
int longHoldTime = 3000;    // ms long hold period: how long to wait for press+hold event

// Button variables
boolean buttonVal = LOW;   // value read from button
boolean buttonLast = LOW;  // buffered value of the button's previous state
boolean DCwaiting = false;  // whether we're waiting for a double click (down)
boolean DConUp = false;     // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true;    // whether it's OK to do a single click
long downTime = -1;         // time the button was pressed down
long upTime = -1;           // time the button was released
boolean ignoreUp = false;   // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false;        // when held, whether to wait for the up event
boolean holdEventPast = false;    // whether or not the hold event happened already
boolean longHoldEventPast = false;// whether or not the long hold event happened already

int checkButton() {
   int event = 0;
   buttonVal = digitalRead(buttonPin);
  
   // Button pressed down
   if (buttonVal == HIGH && buttonLast == LOW && (millis() - upTime) > debounce)
   {
       downTime = millis();
       ignoreUp = false;
       waitForUp = false;
       singleOK = true;
       holdEventPast = false;
       longHoldEventPast = false;
       if ((millis()-upTime) < DCgap && DConUp == false && DCwaiting == true) {
        DConUp = true;
       }
       else {
        DConUp = false;
       }
       DCwaiting = false;
   }
   // Button released
   else if (buttonVal == LOW && buttonLast == HIGH && (millis() - downTime) > debounce)
   {        
       if (not ignoreUp)
       {
           upTime = millis();
           if (DConUp == false) {
            DCwaiting = true;
           }
           else
           {
               event = 2;
               DConUp = false;
               DCwaiting = false;
               singleOK = false;
           }
       }
   }
   // Test for normal click event: DCgap expired
   if ( buttonVal == LOW && (millis()-upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true && event != 2)
   {
       event = 1;
       DCwaiting = false;
   }
   // Test for hold
   if (buttonVal == HIGH && (millis() - downTime) >= holdTime) {
       // Trigger "normal" hold
       if (not holdEventPast)
       {
           if (DConUp) {
            event = 5; // click and hold
           } else {
            event = 3; // hold
           }           
           waitForUp = true;
           ignoreUp = true;
           DConUp = false;
           DCwaiting = false;
           //downTime = millis();
           holdEventPast = true;
           
       }
       // Trigger "long" hold
       if ((millis() - downTime) >= longHoldTime)
       {
           if (not longHoldEventPast)
           {
               event = 4;
               longHoldEventPast = true;
           }
       }
   }
   if (event == 0 && buttonLast == HIGH && buttonVal == LOW) {
    event = 6;
   }
   buttonLast = buttonVal;
   return event;
}
