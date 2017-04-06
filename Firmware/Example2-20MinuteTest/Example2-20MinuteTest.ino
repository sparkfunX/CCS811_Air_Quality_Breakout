/*
  CCS811 Air Quality Sensor Example Code
  By: Nathan Seidle
  SparkFun Electronics
  Date: February 7th, 2017
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Works with SparkFun CCS811 board https://www.sparkfun.com/products/14181
  Or Combo board https://www.sparkfun.com/products/14241
  Enjoy this code? Buy a board and help support SparkFun! 

  Calculates the current run time and indicates when 20 minutes has passed

  Read the TVOC and CO2 values from the SparkFun CSS811 breakout board

  A new sensor requires at 48-burn in. Once burned in a sensor requires
  20 minutes of run in before readings are considered good.

  Hardware Connections (Breakoutboard to Arduino):
  3.3V = 3.3V
  GND = GND
  SDA = A4
  SCL = A5

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
}

void loop()
{
  if (dataAvailable())
  {
    readAlgorithmResults(); //Calling this function updates the global tVOC and CO2 variables

    Serial.print("CO2[");
    Serial.print(CO2);
    Serial.print("] tVOC[");
    Serial.print(tVOC);
    Serial.print("] ");

    printRunTime();

    Serial.println();
  }
  else if (checkForError())
  {
    printError();
  }

  delay(1000); //Wait for next reading
}

//Prints the amount of time the board has been running
//Does the hour, minute, and second calcs
void printRunTime()
{
  char buffer[50];

  unsigned long runTime = millis();

  int hours = runTime / (60 * 60 * 1000L);
  runTime %= (60 * 60 * 1000L);
  int minutes = runTime / (60 * 1000L);
  runTime %= (60 * 1000L);
  int seconds = runTime / 1000L;
  
  sprintf(buffer, "RunTime[%02d:%02d:%02d]", hours, minutes, seconds);
  Serial.print(buffer);

  if(hours == 0 && minutes < 20) Serial.print(" Not yet valid");
}

