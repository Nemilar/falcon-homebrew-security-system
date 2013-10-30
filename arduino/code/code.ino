// Author: Jonathan DePrizio
// jondeprizio@gmail.com
// github.com/nemilar
//
// July, 2013
//
//
// 

// Some verbiage:
// A "sleeping' device = an alarm that is deactivated entirely, and requires the user or a scheduler to activate it.
// An "armed" device = an alarm that will go into "alarm" state if a sensor-trip tells it to do so. 
// An "alarmed" device = a device that has detected an intrusion and is acting accordingly.  An alarmed device must
// 						be put to sleep by a user command, and will remain in alarmed state until that happens.


#include <SoftwareSerial.h>
#include <string.h>
#include <EEPROM.h>

#define DEBUG  0

// Some alarm state definitions
#define SLEEPING 0
#define ARMED 1
#define ALARMING 2

// Some basic shortcut-type stuff.
// For the LCD display:
const int BACKLIGHT_OFF =  18;
const int BACKLIGHT_ON  =  17;
const int LCD_CLEAR     =  12;
const int ALARM_TONE    =  223; // The tone to play when the alarm is tripped
// See http://www.parallax.com/Portals/0/Downloads/docs/prod/displays/27979-Parallax-Serial-LCDs-Product-Guide-v3.1.pdf
// For more information including available tones to play on trip.  They're all pretty obnoxious.


// For the RFID 
#define RFID_LEGACY        0x0F
#define RFID_READ          0x01
#define RFID_WRITE         0x02
#define RFID_RESET         0x06
const int MAX_RFID_SLEEP = 750; // Time to wait for the RFID to respond (ms)
const int RFID_LOCATION  = 4; // Location on RFID card to read.  For now, only reads 1 location (4 bytes).


// Pin assignments
// Digital pins - Output pins
const int armedIndicatorLED = 4; 
//const int audibleIndicatorLED = 4; 
const int tripIndicatorLED    = 3;
const int LCDPin              = 7;  // Parallax 2x16+speaker LCD display #27977-RT
const int RFID_Out            = 11; // Parallax RFID reader, #28443-RT
const int rangefinder         = 12; // Is both output and input, Parallax #28015-RT
//Digital pins - Input pins
const int SensorTrip          = 6; // Parallax PIR sensor, #555-28027-RT
const int SensorPWR           = 8; /* The PIR is powered from a pin so it can be disabled entirely when the system is off.  
                                      This prevents the red LED in the PIR from glowing. */
const int s1                  = 5; // switch 1 - audible alarm
const int s2                  = 2; // switch 2 - send email
const int RFID_In             = 10; 


const int activateSwitch	   = 13; 

// Other stuff - User defined
const int blinkDuration                  = 100000;// how long for the alarm to remain alarming, once it's 
												 // been tripped.  This is for annoying things like beeping, 
												 // sending pictures, blinking the LED, etc...  It's generally
												// assumed that you're going to want to deal with the problem, 
												// but you don't want to annoy your neighbors.  
const int tripIndicatorLEDBlinkInterval  = 100; // blinking interval; ms
                                                // Note that this controls the # of blinks

const int DELAY_TO_ARM = 10; // 400ms per 1 DELAY_TO_ARM.  This is the amount of time you have to get out 
								// after turning the alarm on. The trip indicator will blink during this time.

// Working variables.
int alarmState                             = ARMED; // LCD_SLEEPING; ARMED; ALARMING;
byte rfidByte;                           // simple byte to store some RFID data.
byte rfidData[4];                        // Passcode fro mthe RFID.  See passcode[4] and challenge.
unsigned long lastBlink;                // last time the LED blinked.
unsigned long alarmRemainAlarmedUntil = 0; // how far in the future we'll blink until.
int audibleAlarm                         = 0; // whether or not to sound the BEEP.  Annoying. Set by s1.
byte oldState;                           // last recorded state of the PIR sensor.
const int rfidReadInterval               = 3000;
unsigned long lastRFIDReadTime;          // The last time an RFID read was issued
int rfidReadActive                       = 0;
char passcode[4];                        // Actual read from the EEPROM
byte challenge                            = 1; // 0 when the passcode is right; 1 when it is wrong.
byte emailAlarm;

SoftwareSerial lcdInterface = SoftwareSerial(255,LCDPin); 
SoftwareSerial rfidInterface = SoftwareSerial(RFID_In, RFID_Out); 

void backlightOn()
{
      lcdInterface.write(BACKLIGHT_ON);
}
void backlightOff()
{
      lcdInterface.write(BACKLIGHT_OFF);
}

// Simple function that queries a rangefinder for the current range.

int getRangeFromFinder(int rangeFinderPin)
{
         if (DEBUG)
          Serial.println("Chirp\n");
         pinMode(rangeFinderPin, OUTPUT);
         digitalWrite(rangeFinderPin, LOW);
         delayMicroseconds(2);
         digitalWrite(rangeFinderPin, HIGH);
         delayMicroseconds(5);
         digitalWrite(rangeFinderPin, LOW);
         pinMode(rangeFinderPin, INPUT);
         return pulseIn(rangeFinderPin, HIGH)/74/2;
}

void readDipSwitchAndSetValues()
{
	     /* Check if switch 1 is set to ON.  This tells us whether or not we 
	     should beep audibly on a SensorTrip.  It's useful to have this, since 
	     when you're building and debugging the system, you'll be tripping the 
	     sensor a lot, and it can be incredibly annoying. */
	     
	     if (digitalRead(s1) == HIGH)
	     {
	       audibleAlarm = 1;
	    //   digitalWrite(audibleIndicatorLED, HIGH);                                                
	       
	     }
	     else
	     {
	       audibleAlarm = 0;
	    //   digitalWrite(audibleIndicatorLED, LOW);
	     }
	
	     if (digitalRead(s2) == HIGH)
	     {
	     	// switch two indicates that we need to send an email.
	        // the pi reads this string and sends it.
			emailAlarm = 1; 
	      }
	
	
}
void setup()
{
    Serial.begin(9600);
    Serial.println("System starting...");
    pinMode(SensorTrip, INPUT); // readings from the PIR 
    pinMode(s1, INPUT); // switch toggle 1 - audible alarm switch
    pinMode(s2, INPUT); // switch toggle 2 - send email 
    pinMode(RFID_In, INPUT);
    
    pinMode(SensorPWR, OUTPUT);
    digitalWrite(SensorPWR, HIGH);
    pinMode(RFID_Out, OUTPUT);
    pinMode(LCDPin, OUTPUT); 
  //  pinMode(audibleIndicatorLED, OUTPUT); 
	pinMode(armedIndicatorLED, OUTPUT);
    pinMode(tripIndicatorLED, OUTPUT); 
    
    digitalWrite(LCDPin, HIGH); // Turn on the LCD to start
    
    
    //lcdInterface.begin(9600);
    //lcdInterface.write(LCD_CLEAR);
    //backlightOn();
    
	setAlarmState(ARMED);
    rfidInterface.begin(9600);

    /* Read the passcode from the EEPROM.  Right now we're just reading a single memory
    location on the RFID card, so we only read 4 bytes. */
    for (int r=0;r<4;r++)
    {
      passcode[r] = EEPROM.read(r);
     
    }
    Serial.print("RFID Unlock value: ");
    Serial.println(passcode);
    
    // The rangefinder is not included here, because it is only
    // turned on when the system is off, and the system is by default
    // turned on.
    
    delay(1000); // Give everything a second.  Literally.
	Serial.println("System started.");
}


/* blink the trip indicator a few times for a visual indication of something. */
void quickBlinkTrip(int numberOfBlinks)
{
	digitalWrite(tripIndicatorLED, LOW);
	for (int x=0;x<numberOfBlinks;x++)
	{
		digitalWrite(tripIndicatorLED, HIGH);
		delay(200);
		digitalWrite(tripIndicatorLED, LOW);
		delay(200);
	}	
}
// When an action is performed (e.g, an RFID card is presented), we change the global
// state of the alarm.  This function does just that. 
void setAlarmState(int newState)
{
	if (newState == SLEEPING)
	{
		         alarmState = SLEEPING;
				 challenge = 1;
		         alarmRemainAlarmedUntil = 0;
		         pinMode(SensorPWR, INPUT); // Is there a better way to disable a pin?
				 digitalWrite(armedIndicatorLED, LOW);
				 digitalWrite(tripIndicatorLED, LOW);
				 quickBlinkTrip(3);
				 Serial.println("Alarm going to sleep.");
		         delay(5000); // dislike, but simple...
				 Serial.println("Alarm sleeping.");
				 quickBlinkTrip(2);
	}
	else if (newState == ALARMING)
	{
		        alarmState = ALARMING;
		        // Do different things depending on the settings from the dip switch
		        if (audibleAlarm == 1)
		        {
		          lcdInterface.write(ALARM_TONE); 
		        }
		        if (emailAlarm == 1)
		         {
		           // switch two indicates that we need to send an email.
		           // the pi reads this string and sends it.
		           Serial.println("Motion detected!");
		           lcdInterface.write("EMAIL/PHOTO SENT");
		         }
				// end of dip-switch-specific stuff.  
		        alarmRemainAlarmedUntil = millis() + blinkDuration;
	}
	else if (newState == ARMED)
	{
		Serial.println("Arming..");
		digitalWrite(armedIndicatorLED, HIGH);
		alarmState = ARMED;
		alarmRemainAlarmedUntil = 0;
		digitalWrite(tripIndicatorLED, LOW);
		rfidReadActive = 0;
		challenge = 1;
		Serial.println("ARMED.");
		quickBlinkTrip(DELAY_TO_ARM); // this also serves as a delay to GTFO.  Once it stops blinking,
							// a trip will set it off.
	}
}

void loop()
{
	  /* the DIP switch values are independent of everything else.  They control
	  general behavior such as the audible alarm, etc..  Read and set them first. */
	  readDipSwitchAndSetValues();
	

      /* This section controls the blinking of the tripIndicatorLED.  This LED blinks
      whenever the sensor is tripped (when SensorTrip is high).  When tripped, the 
      alarmRemainAlarmedUntil is set to some future time.  The following code checks
      whether this is the case, and if so, blinks accordingly (with some additional logic
      to control the frequency of the blinking */
      if (alarmState == ALARMING && (millis() - lastBlink > tripIndicatorLEDBlinkInterval))
        {
           if (digitalRead(tripIndicatorLED) == LOW)
           {
             digitalWrite(tripIndicatorLED, HIGH);
             if (DEBUG)
               Serial.write("Turn indicator to high\n");
           }
           else
           {
            digitalWrite(tripIndicatorLED, LOW);
            if (DEBUG)
              Serial.write("Turn indicator to low\n");
           } 
           lastBlink = millis();
        }
	
      
      
     /* The RFID logic is a bit extreme; it makes up the bulk of this function, I think.
	Therefore, it needs to be moved out (FIXME).  Anyhow, read the RFID; if the password
	matches, toggle challenge to 0.  Then arm/disarm the system, depending on state. */
     

       // If we haven't issued a read command yet, issue one.
       if (rfidReadActive == 0)
       {

         rfidInterface.write(RFID_RESET);
         rfidInterface.write("!RW"); 
         rfidInterface.write(byte(RFID_READ));
         rfidInterface.write(byte(RFID_LOCATION));
         rfidReadActive = 1;
         lastRFIDReadTime = millis();
         Serial.println("Issued new RFID request.");
         
       }
       else
       {
         // There is an outstanding read to the RFID.
         // Is anything available yet?
         if (rfidInterface.available() > 0)
         {
           rfidByte = rfidInterface.read();
           if (rfidByte == 1)
           {
             challenge=0;
             for (int i=0;i<4;i++)
             {
               Serial.print("-Valid data\n");
               rfidData[i] = rfidInterface.read();
               Serial.print(rfidData[i], HEX);
               if (rfidData[i] == passcode[i])
               {
                  if (DEBUG)
                   Serial.print("-Passcode_Match-");
               }
               else
               {
                 challenge=1;  // Indicates that the password is wrong.
                 if (DEBUG)
                   Serial.print("-Passcode_Mismatch-");
               }
             }
             if (DEBUG)
              Serial.print(challenge);
           }
           else
           {
             if (DEBUG)
              {    
                Serial.println("Data available, but garbage.\n");
                Serial.println(rfidByte, HEX);
              }  
           }
         }
         else
         {
           // the RFID still isn't sending any data.  If we've waited long
           // enough, invalidate the current read attempt so we try
           // again on the next loop.
           if ((lastRFIDReadTime + rfidReadInterval) < millis())
           {
             rfidInterface.write(RFID_RESET);
             rfidInterface.flush();
             Serial.println("Invalidate previous RFID request.");
             rfidReadActive = 0;
             
           }
         }
       }
     
     /* If challenge has been set to 0, that means that a valid RFID card has been
       presented. for simplicity, this check is performed seperately. challenge is set
       above, when the RFID reading occurs. Turn everything off. */
       if (challenge == 0)
       {
         if (alarmState == ALARMING || alarmState == ARMED)
        {  
		  setAlarmState(SLEEPING);
        } 
		 else if(alarmState == SLEEPING)
		{
			Serial.println("User-issued arm.");
			setAlarmState(ARMED);
		}
   	   }
         
       
   	 /* The following giant if statement controls the behavior that happens, depending
	upon the current and previous state of the alarm */          
     

	 if (alarmState == ARMED || alarmState == ALARMING)
	{
	     /* First-run when system goes from waiting to actually tripped.                  */
	     /*********************************************************************************/
	     if (digitalRead(SensorTrip) == HIGH && oldState == LOW)
	     {
			setAlarmState(ALARMING);
	     }

	     /* When we remain active (we're already active) */
	     else if (digitalRead(SensorTrip) == HIGH && oldState == HIGH)
	     {
	       oldState = HIGH;
	       if (challenge == 1)
	         alarmRemainAlarmedUntil = millis() + blinkDuration;
	     }


		/* We are no longer being tripped, but we were just a few moments ago */
		else if (digitalRead(SensorTrip) == LOW && oldState == HIGH)
		{

		   oldState = LOW;
		 } 

		 /* Normal behavior - sensor has not detected anything for a while. */
		
	     else if (digitalRead(SensorTrip) == LOW && oldState == LOW)
	     {
	       oldState = LOW;
	     }
	}
      
}
