/*
  CCS811 Air Quality Sensor Example Code
  By: Nathan Seidle
  SparkFun Electronics
  Date: February 7th, 2017
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Works with SparkFun CCS811 board https://www.sparkfun.com/products/14181
  Or Combo board https://www.sparkfun.com/products/14241
  Enjoy this code? Buy a board and help support SparkFun! 

  Reads the baseline number from the CCS811. You are supposed to gather this value
  when the sensor is in normalized clean air and use it to adjust (calibrate) the sensor 
  over the life of the sensor as it changes due to age.

  Hardware Connections (Breakoutboard to Arduino):
  3.3V = 3.3V
  GND = GND
  SDA = A4
  SCL = A5
  WAKE = D5 - Optional, can be left unconnected

  Serial.print it out at 9600 baud to serial monitor.
*/

#include <Wire.h>

//These are the air quality values obtained from the sensor
unsigned int tVOC = 0;
unsigned int CO2 = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("CCS811 Read Example");

  Wire.begin();

  if (configureCCS811() == false)
  {
    Serial.println("Problem with CCS811");
    while(1);
  }
  else
  {
    Serial.println("CCS811 online");
  }
  
  unsigned int result = getBaseline();
  Serial.print("baseline for this sensor: 0x");
  Serial.println(result, HEX);
}

void loop()
{
  Serial.println("Done");
  delay(1000);
}

