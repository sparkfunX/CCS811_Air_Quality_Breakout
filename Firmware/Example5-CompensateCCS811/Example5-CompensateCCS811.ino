/*
  Use BME280 to compensate and improve CCS811 readings
  By: Nathan Seidle
  SparkFun Electronics
  Date: April 6th, 2017
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Works with SparkFun combo board https://www.sparkfun.com/products/14241
  Enjoy this code? Buy a board and help support SparkFun! 
  
  Sends the humidity and temperature from a separate sensor to the CCS811 so
  that the CCS811 can adjust its algorithm.
  
  Hardware Connections (Breakoutboard to Arduino):
  Attach a Qwiic Shield to your RedBoard, Photon, or ESP32.
  Plug the Qwiic Air Quality Combo board into one of the connectors
  Serial.print it out at 9600 baud to serial monitor.
*/


#include <Wire.h>
#include "SparkFunBME280.h" //Library for BME280 from library manager or https://github.com/sparkfun/SparkFun_BME280_Arduino_Library

BME280 myBME280; //Global sensor object for BME280

//Global variables obtained from the two sensors
unsigned int tVOC = 0;
unsigned int CO2 = 0;
float tempF = 0;
float tempC = 0;
float humidity = 0;
long pascals = 0;
float pressureInHg = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("CCS811+BME280 Compensation Example");

  Wire.begin();

  if (configureCCS811() == false)
  {
    Serial.println("Problem with CCS811");
  }
  else
  {
    Serial.println("CCS811 online");
  }

  //Setup the BME280 for basic readings
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; //  3, Normal mode
  myBME280.settings.tStandby = 0; //  0, 0.5ms
  myBME280.settings.filter = 0; //  0, filter off
  myBME280.settings.tempOverSample = 1;
  myBME280.settings.pressOverSample = 1;
  myBME280.settings.humidOverSample = 1;

  delay(10); //Give BME280 time to come on
  //Calling .begin() causes the settings to be loaded
  byte id = myBME280.begin(); //Returns ID of 0x60 if successful
  if (id != 0x60)
  {
    Serial.println("Problem with BME280");
  }
  else
  {
    Serial.println("BME280 online");
  }
  
}
//---------------------------------------------------------------
void loop()
{
  if (dataAvailable())
  {
    readAlgorithmResults(); //Read latest from CCS811 and update tVOC and CO2 variables
    getWeather(); //Get latest humidity/pressure/temp data from BME280
    printData(); //Pretty print all the data

    setEnvironmentalData(humidity, tempC); //Feed the current humidity and pressure back to the CCS811
    Serial.println("Compensation data sent");
  }
  else if (checkForError())
  {
    printError();
  }

  delay(2000); //Wait for next reading
}

//Gets the latest temperature, pressure and humidity from BME280
void getWeather()
{
  //Measure the Barometer temperature in F from the BME280
  tempF = myBME280.readTempF();
  tempC = myBME280.readTempC();

  //Measure Pressure from the BME280
  pascals = myBME280.readFloatPressure();
  pressureInHg = pascals * 0.0002953; // Calc for converting Pa to inHg

  // Measure Relative Humidity from the BME280
  humidity = myBME280.readFloatHumidity();
}

//Print CO2, TVOC, Humidity, Pressure and Temp
void printData()
{
  Serial.print(" CO2[");
  Serial.print(CO2);
  Serial.print("]ppm");

  Serial.print(" TVOC[");
  Serial.print(tVOC);
  Serial.print("]ppb");

  Serial.print(" temp[");
  Serial.print(tempC, 1);
  Serial.print("]C");

  //Serial.print(" temp[");
  //Serial.print(tempF, 1);
  //Serial.print("]F");

  Serial.print(" pressure[");
  Serial.print(pascals);
  Serial.print("]Pa");

  //Serial.print(" pressure[");
  //Serial.print(pressureInHg, 2);
  //Serial.print("]InHg");

  //Serial.print("altitude[");
  //Serial.print(pressureSensor.readFloatAltitudeMeters(), 2);
  //Serial.print("]m");

  //Serial.print("altitude[");
  //Serial.print(pressureSensor.readFloatAltitudeFeet(), 2);
  //Serial.print("]ft");

  Serial.print(" humidty[");
  Serial.print(humidity, 0);
  Serial.print("]%");

  Serial.println();
}

