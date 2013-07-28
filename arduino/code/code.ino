#include <SoftwareSerial.h>
#include <string.h>


// Some basic shortcut-type stuff.
// For the LCD display:
const int BACKLIGHT_OFF =  18;
const int BACKLIGHT_ON  =  17;
const int LCD_CLEAR     =  12;
const int ALARM_TONE    =  223; // The tone to play when the alarm is tripped
// See http://www.parallax.com/Portals/0/Downloads/docs/prod/displays/27979-Parallax-Serial-LCDs-Product-Guide-v3.1.pdf
// For more information including available tones to play on trip.  They're all pretty obnoxious.
const int LCD_ALARM     = 1; 
const int LCD_READY     = 0;

// For the RFID 
#define RFID_LEGACY        0x0F
#define RFID_READ          0x01
#define RFID_WRITE         0x02
#define RFID_RESET         0x06
const int MAX_RFID_SLEEP = 750; // Time to wait for the RFID to respond (ms)


// Pin assignments
// Digital pins - Output pins
const int audibleIndicatorLED = 4; 
const int tripIndicatorLED = 3;
const int LCDPin = 7;
const int RFID_Out = 11;
//Digital pins - Input pins
const int SensorTrip = 6; // PIR 
const int s1 = 5; // switch 1 - audible alarm
const int s2 = 2; // switch 2 - send email
const int RFID_In = 10;


// Other stuff - User defined
const int blinkDuration =                 1000;// how long to blink after detection of motion; ms
const int tripIndicatorLEDBlinkInterval = 100; // blinking interval; ms
                                               // Note that this controls the # of blinks


// Working variables.
int tripLIndicatorLEDState = 0;
int LCDState = LCD_READY;

long  lastBlink;
long tripIndicatorLEDBlinkUntil = 0; 
int audibleAlarm = 0;
char* message;
byte oldState;

SoftwareSerial lcdInterface = SoftwareSerial(255,LCDPin);

void setup()
{
    Serial.begin(9600);
    Serial.println("System Live...");
    pinMode(SensorTrip, INPUT); // readings from the PIR 
    pinMode(s1, INPUT); // switch toggle 1 - audible alarm switch
    pinMode(s2, INPUT); // switch toggle 2 - send email 
    pinMode(RFID_In, INPUT);
    
    pinMode(RFID_Out, OUTPUT);
    pinMode(LCDPin, OUTPUT); // Set Digitalpin 4 to an output pin
    pinMode(audibleIndicatorLED, OUTPUT); // LED indicating whether the alarm will sound
    pinMode(tripIndicatorLED, OUTPUT); // Indicates a trip of the sensor
    
    digitalWrite(LCDPin, HIGH); // Turn on the LCD to start
    
    
    lcdInterface.begin(9600);
    lcdInterface.write(213);  // set tone to 1/2 note (1sec)
    //lcdInterface.write(219); // set to MAXIMUM ANNOYANCE!!
    lcdInterface.write(12); // clear
    lcdInterface.write(BACKLIGHT_ON); //backlight on
    delay(5); // required, for some reason...
}

void loop()
{

      /* This section controls the blinking of the tripIndicatorLED.  This LED blinks
      whenever the sensor is tripped (when SensorTrip is high).  When tripped, the 
      tripIndicatorLEDBlinkUntil is set to some future time.  The following code checks
      whether this is the case, and if so, blinks accordingly (with some additional logic
      to control the frequency of the blinking */
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
        lcdInterface.write(BACKLIGHT_ON);
      }
      else
      {
        digitalWrite(tripIndicatorLED, LOW);
        /* If the LCD is currently showing that we are alarmed, then
        clear it.  Otherwise, ignore. */
        if (LCDState == LCD_ALARM)
        {
          lcdInterface.write(BACKLIGHT_OFF);
          lcdInterface.write(LCD_CLEAR);
          lcdInterface.write("Alarm system    activated.");
          LCDState = LCD_READY;
        }
      }
      
     /* Check if switch 1 is set to ON.  This tells us whether or not we 
     should beep audibly on a SensorTrip.  It's useful to have this, since 
     when you're building and debugging the system, you'll be tripping the 
     sensor a lot, and it can be incredibly annoying. */
     
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
     
     
     
     /* Code that is executed when the sensor actually trips (SensorTrip reads HIGH). */
     /*********************************************************************************/
     if (digitalRead(SensorTrip) == HIGH && oldState == LOW)
     {
      lcdInterface.write(LCD_CLEAR);
      lcdInterface.write(BACKLIGHT_ON);
      lcdInterface.write("ALARM TRIPPED!  ");
      LCDState = LCD_ALARM;
      
      if (audibleAlarm == 1)
      {
        lcdInterface.write(ALARM_TONE); 
      }
      oldState = HIGH;
      if (digitalRead(s2) == HIGH)
       {
         // switch two indicates that we need to send an email.
         // the pi reads this string and sends it.
         Serial.println("Motion detected!");
         lcdInterface.write("EMAIL/PHOTO SENT");
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

       oldState = LOW;
  
     }
     
}
