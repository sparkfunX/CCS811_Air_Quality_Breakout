/*

  Basic control functions of the CCS811

*/

//Register addresses
#define CSS811_STATUS 0x00
#define CSS811_MEAS_MODE 0x01
#define CSS811_ALG_RESULT_DATA 0x02
#define CSS811_RAW_DATA 0x03
#define CSS811_ENV_DATA 0x05
#define CSS811_NTC 0x06
#define CSS811_THRESHOLDS 0x10
#define CSS811_BASELINE 0x11
#define CSS811_HW_ID 0x20
#define CSS811_HW_VERSION 0x21
#define CSS811_FW_BOOT_VERSION 0x23
#define CSS811_FW_APP_VERSION 0x24
#define CSS811_ERROR_ID 0xE0
#define CSS811_APP_START 0xF4
#define CSS811_SW_RESET 0xFF

#define CCS811_ADDR 0x5B //7-bit unshifted default I2C Address

//Updates the total voltatile organic compounds (TVOC) in parts per billion (PPB)
//and the CO2 value
//Returns nothing
void readAlgorithmResults()
{
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CSS811_ALG_RESULT_DATA);
  Wire.endTransmission();

  Wire.requestFrom(CCS811_ADDR, 4); //Get four bytes

  byte co2MSB = Wire.read();
  byte co2LSB = Wire.read();
  byte tvocMSB = Wire.read();
  byte tvocLSB = Wire.read();

  CO2 = ((unsigned int)co2MSB << 8) | co2LSB;
  tVOC = ((unsigned int)tvocMSB << 8) | tvocLSB;
}

//Turns on the sensor and configures it with default settings
boolean configureCCS811()
{
  //Verify the hardware ID is what we expect
  byte hwID = readRegister(0x20); //Hardware ID should be 0x81
  if (hwID != 0x81)
  {
    Serial.print("Hardware ID wrong: 0x");
    Serial.println(hwID, HEX);
    return (false);
  }

  //Check for errors
  if (checkForError() == true)
  {
    Serial.println("Error at Startup");
    printError();
    return (false);
  }

  //Tell App to Start
  if (appValid() == false)
  {
    Serial.println("Error: App not valid.");
    return (false);
  }

  //Write to this register to start app
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CSS811_APP_START);
  Wire.endTransmission();

  //Check for errors
  if (checkForError() == true)
  {
    Serial.println("Error at AppStart");
    printError();
    return (false);
  }

  //Set Drive Mode
  setDriveMode(1); //Read every second

  //Check for errors
  if (checkForError() == true)
  {
    Serial.println("Error at setDriveMode");
    printError();
    return (false);
  }

  return (true); //All set!
}

//Checks to see if error bit is set
boolean checkForError()
{
  byte value = readRegister(CSS811_STATUS);
  return (value & 1 << 0);
}

//Displays the type of error
//Calling this causes reading the contents of the ERROR register
//This should clear the ERROR_ID register
void printError()
{
  byte error = readRegister(CSS811_ERROR_ID);

  Serial.print("Error: ");
  if (error & 1 << 5) Serial.print("HeaterSupply ");
  if (error & 1 << 4) Serial.print("HeaterFault ");
  if (error & 1 << 3) Serial.print("MaxResistance ");
  if (error & 1 << 2) Serial.print("MeasModeInvalid ");
  if (error & 1 << 1) Serial.print("ReadRegInvalid ");
  if (error & 1 << 0) Serial.print("MsgInvalid ");

  Serial.println();
}

//Returns the baseline value
//Used for telling sensor what 'clean' air is
//You must put the sensor in clean air and record this value
unsigned int getBaseline()
{
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CSS811_BASELINE);
  Wire.endTransmission();

  Wire.requestFrom(CCS811_ADDR, 2); //Get two bytes

  byte baselineMSB = Wire.read();
  byte baselineLSB = Wire.read();

  unsigned int baseline = ((unsigned int)baselineMSB << 8) | baselineLSB;

  Serial.print("Baseline: ");
  Serial.println(baseline);

  return (baseline);
}

//Checks to see if DATA_READ flag is set in the status register
boolean dataAvailable()
{
  byte value = readRegister(CSS811_STATUS);
  return (value & 1 << 3);
}

//Checks to see if APP_VALID flag is set in the status register
boolean appValid()
{
  byte value = readRegister(CSS811_STATUS);
  return (value & 1 << 4);
}

//Enable the nINT signal
void enableInterrupts(void)
{
  byte setting = readRegister(CSS811_MEAS_MODE); //Read what's currently there
  setting |= (1 << 3); //Set INTERRUPT bit
  writeRegister(CSS811_MEAS_MODE, setting);
}

//Disable the nINT signal
void disableInterrupts(void)
{
  byte setting = readRegister(CSS811_MEAS_MODE); //Read what's currently there
  setting &= ~(1 << 3); //Clear INTERRUPT bit
  writeRegister(CSS811_MEAS_MODE, setting);
}

//Mode 0 = Idle
//Mode 1 = read every 1s
//Mode 2 = every 10s
//Mode 3 = every 60s
//Mode 4 = RAW mode
void setDriveMode(byte mode)
{
  if (mode > 4) mode = 4; //Error correction

  byte setting = readRegister(CSS811_MEAS_MODE); //Read what's currently there

  setting &= ~(0b00000111 << 4); //Clear DRIVE_MODE bits
  setting |= (mode << 4); //Mask in mode
  writeRegister(CSS811_MEAS_MODE, setting);
}

//Given a temp and humidity, write this data to the CSS811 for better compensation
//This function expects the humidity and temp to come in as floats
//From Programming Guide, page 18
void setEnvironmentalData(float relativeHumidity, float temperature)
{
  long rH = relativeHumidity * 1000; //42.348 becomes 42348
  long temp = temperature * 1000; //23.2 becomes 23200

  byte envData[4];

  //Split relative humidity value into 7-bit integer and 1-bit fractional
  //From our example: 42348 % 1000 = 348
  //348 / 100 = 3
  //This is basically .3 which is not high enough to round up (above .7 is required to round up)
  if (((rH % 1000) / 100) > 7)
    envData[0] = (rH / 1000 + 1) << 1; //Add 1% to the humidity and shift to 7 bits
  else
    envData[0] = (rH / 1000) << 1; //Just shift to fit in the 7 bit spot

  //Now check to see if there is a half of % that we can stick in bit 0
  //If our .3 is more than .2 and less than .8 then we can say this is 42.5% humidty
  if (((rH % 1000) / 100) > 2 && (((rH % 1000) / 100) < 8))
  {
    envData[0] |= 1; //Set 9th bit of fractional to indicate 0.5%
  }

  envData[1] = 0; //CCS811 only supports increments of 0.5 (bit 0 of byte 0) so byte 1 will always be zero

  //Now we deal with temperature
  temp += 25000; //Add the 25C offset

  //Split value into 7-bit integer and 1-bit fractional
  //23200 + 25000 = 48200
  //48200 % 1000 = 200
  //200 / 100 = 2
  //This is basically .2 C which is not high enough to round up (above .7 is required to round up)
  if (((temp % 1000) / 100) > 7)
  {
    envData[2] = (temp / 1000 + 1) << 1; //Add 1 degree C to the temp and shift to 7 bits
  }
  else
  {
    envData[2] = (temp / 1000) << 1; //Don't add anything, just shift to fit into 7 bits
  }

  //Now check to see if there is a half a degree that we can stick in bit 0
  //If our .2 C is not .2 AND less than .8 so we call our temp 48C with no fraction of C in bit 0
  if (((temp % 1000) / 100) > 2 && (((temp % 1000) / 100) < 8))
  {
    envData[2] |= 1;  //Set 9th bit of fractional to indicate 0.5C
  }

  envData[3] = 0; //CCS811 only supports increments of 0.5 (bit 0 of byte 0) so byte 1 will always be zero

  /*Serial.println("envData: ");
  Serial.println(envData[0], HEX);
  Serial.println(envData[1], HEX);
  Serial.println(envData[2], HEX);
  Serial.println(envData[3], HEX);*/

  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CSS811_ENV_DATA); //We want to write our RH and temp data to the ENV register
  Wire.write(envData[0]);
  Wire.write(envData[1]);
  Wire.write(envData[2]);
  Wire.write(envData[3]);
  Wire.endTransmission();
}

//Reads from a give location from the CSS811
byte readRegister(byte addr)
{
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(addr);
  Wire.endTransmission();

  Wire.requestFrom(CCS811_ADDR, 1);

  return (Wire.read());
}

//Write a value to a spot in the CCS811
void writeRegister(byte addr, byte val)
{
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(addr);
  Wire.write(val);
  Wire.endTransmission();
}

