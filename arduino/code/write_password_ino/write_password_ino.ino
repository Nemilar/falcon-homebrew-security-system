// Utility to write some bytes to the EEProm.

#include <EEPROM.h>

char passcode[5] = "----";

void setup()
{
      Serial.begin(9600);
      for (int i=0;i<200;i++)
      {
          EEPROM.write(i, passcode[i]);
      }
}

void loop()
{
    for (int i=0;i<10;i++)
{
    Serial.print(EEPROM.read(i), HEX);
  
}
Serial.print("\n");
}

