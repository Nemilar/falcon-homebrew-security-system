#include <SoftwareSerial.h>
#include <string.h>

const int BACKLIGHT_OFF=18;
const int BACKLIGHT_ON=17;

const int audibleIndicatorLED = 4 ;
const int tripIndicatorLED = 3;
const int LCDPin = 7;
const int SensorTrip = 6;
const int s1 = 5; // switch 1 - audible alarm
const int s2 = 2; // switch 2 - send email
const int blinkDuration = 1000; // how long to blink after detection of motion; ms

int tripLIndicatorLEDState = 0;
long   lastBlink;
int tripIndicatorLEDBlinkInterval = 100; // blinking interval; ms
long tripIndicatorLEDBlinkUntil = 0; 
int audibleAlarm = 0;
char* message;
byte oldState;

SoftwareSerial mySerial = SoftwareSerial(255,LCDPin);

void setup()
{
    Serial.begin(9600);
    Serial.println("System Live...");
    pinMode(LCDPin, OUTPUT); // Set Digitalpin 4 to an output pin
    pinMode(SensorTrip, INPUT); // readings from the PIR 
    pinMode(s1, INPUT); // switch toggle 1 - audible alarm switch
    pinMode(s2, INPUT); // switch toggle 2 - send email 
    
    pinMode(audibleIndicatorLED, OUTPUT); // LED indicating whether the alarm will sound
    pinMode(tripIndicatorLED, OUTPUT);
    digitalWrite(LCDPin, HIGH); // enabled
    
    
    mySerial.begin(9600);
    mySerial.write(213);  // set tone to 1/2 note (1sec)
    //mySerial.write(219); // set to MAXIMUM ANNOYANCE!!
    mySerial.write(12); // clear
    mySerial.write(BACKLIGHT_ON); //backlight on
    delay(5); // required, for some reason...
}

void loop()
{
      // time tracking for LED blinking
      if (tripIndicatorLEDBlinkUntil > millis())
      {
        // we are in a blinking state.
        // we have reached the blink interval.  Do something:
        if (millis() - lastBlink > tripIndicatorLEDBlinkInterval)
        {
           if (digitalRead(tripIndicatorLED) == LOW)
           {
             digitalWrite(tripIndicatorLED, HIGH);
             Serial.write("Turn indicator to high\n");
           }
           else
           {
            digitalWrite(tripIndicatorLED, LOW);
            Serial.write("Turn indicator to low\n");
           } 
           lastBlink = millis();
        }
        mySerial.write(BACKLIGHT_ON);
      }
      else
      {
        digitalWrite(tripIndicatorLED, LOW);
      //       Serial.write("Out of blinking state.  Everyone off.");
        mySerial.write(BACKLIGHT_OFF);
      
      }
      
      // If switch1 is on, we'll beep.  This setting is indicated
     // by an LED on audibleInicatorLED (yellow).
     
     if (digitalRead(s1) == HIGH)
     {
       audibleAlarm = 1;
       digitalWrite(audibleIndicatorLED, HIGH);                                                
       
     }
     else
     {
       audibleAlarm = 0;
       digitalWrite(audibleIndicatorLED, LOW);
     }
     
     // When the PIR is tripped, do this:  * ACTION *     
     //
     //
     ///////////////////////////////////////////////////////
     if (digitalRead(SensorTrip) == HIGH && oldState == LOW)
     {
      mySerial.write(12);
      mySerial.write(BACKLIGHT_ON);
      
      
      //mySerial.write("Backlight Off");
      if (audibleAlarm == 1)
      {
        mySerial.write(223);
      }
      oldState = HIGH;
      if (digitalRead(s2) == HIGH)
       {
         // switch two indicates that we need to send an email.
         // the pi reads thins string and sends it.
         Serial.println("Motion detected!");
       }
      

      tripIndicatorLEDBlinkUntil = millis() + blinkDuration;
      
      
     }
     
     else if (digitalRead(SensorTrip) == HIGH && oldState == HIGH)
     {
       oldState = HIGH;

        tripIndicatorLEDBlinkUntil = millis() + blinkDuration;
     }
     else if (digitalRead(SensorTrip) == LOW && oldState == LOW)
     {
       oldState = LOW;
     }
     else if (digitalRead(SensorTrip) == LOW && oldState == HIGH)
     {
       mySerial.write(12);
       mySerial.write(BACKLIGHT_OFF);
       //mySerial.write("Backlight Off");
       oldState = LOW;
  
     }
     
}
