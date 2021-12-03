/** =========================================================================
 * @file DRWI_LTE.ino
 * @brief Example for DRWI CitSci LTE sites.
 *
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 * @copyright (c) 2017-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 *
 * Build Environment: Visual Studios Code with PlatformIO
 * Hardware Platform: EnviroDIY Mayfly Arduino Datalogger
 *
 * DISCLAIMER:
 * THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
 * ======================================================================= */
 
// ==========================================================================
//  Defines for the Arduino IDE
//  NOTE:  These are ONLY needed to compile with the Arduino IDE.
//         If you use PlatformIO, you should set these build flags in your
//         platformio.ini
// ==========================================================================
/** Start [defines] */
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 64
#endif
#ifndef TINY_GSM_YIELD_MS
#define TINY_GSM_YIELD_MS 2
#endif
/** End [defines] */
 
// ==========================================================================
//  Include the libraries required for any data logger
// ==========================================================================
/** Start [includes] */
// The Arduino library is needed for every Arduino program.
#include <Arduino.h>
 
// EnableInterrupt is used by ModularSensors for external and pin change
// interrupts and must be explicitly included in the main program.
#include <EnableInterrupt.h>

// Include the main header for ModularSensors
#include <ModularSensors.h>
#include <LoggerBase.h>
 
#include <SoftwareSerial_ExtInts.h>
/** End [includes] */
 
 
// ==========================================================================
//  Data Logging Options
// ==========================================================================
/** Start [logging_options] */
// The name of this program file
const char* sketchName = "DUNEX_LTE.ino";
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char* LoggerID = "MEOW2";
// How frequently (in minutes) to log data
const uint8_t loggingInterval = 5; //using 30 minutes to test overnight 
// Your logger's timezone.
const int8_t timeZone = -5;  // Eastern Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!
 
// Set the input and output pins for the logger
// NOTE:  Use -1 for pins that do not apply
const int32_t serialBaud = 115200;  // Baud rate for debugging
const int8_t  greenLED   = 8;       // Pin for the green LED
const int8_t  redLED     = 9;       // Pin for the red LED
const int8_t  buttonPin  = 21;      // Pin for debugging mode (ie, button pin)
const int8_t  wakePin    = 31;  // MCU interrupt/alarm pin to wake from sleep
// Mayfly 0.x D31 = A7
// Set the wake pin to -1 if you do not want the main processor to sleep.
// In a SAMD system where you are using the built-in rtc, set wakePin to 1
const int8_t sdCardPwrPin   = -1;  // MCU SD card power pin
const int8_t sdCardSSPin    = 12;  // SD card chip select/slave select pin
const int8_t sensorPowerPin = 22;  // MCU pin controlling main sensor power
/** End [logging_options] */
 
 
// ==========================================================================
//  Wifi/Cellular Modem Options
// ==========================================================================
/** Start [xbee_cell_transparent] */
// For any Digi Cellular XBee's
// NOTE:  The u-blox based Digi XBee's (3G global and LTE-M global)
// are more stable used in bypass mode (below)
// The Telit based Digi XBees (LTE Cat1) can only use this mode.
#include <modems/DigiXBeeLTEBypass.h>
 
// Create a reference to the serial port for the modem
HardwareSerial& modemSerial = Serial1;  // Use hardware serial if possible
const int32_t   modemBaud   = 9600;     // All XBee's use 9600 by default
 
// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
const int8_t modemVccPin    = -2;    // MCU pin controlling modem power
const int8_t modemStatusPin = 19;    // MCU pin used to read modem status
const bool useCTSforStatus = false;  // Flag to use the modem CTS pin for status
const int8_t modemResetPin = 20;     // MCU pin connected to modem reset pin
const int8_t modemSleepRqPin = 23;   // MCU pin for modem sleep/wake request
const int8_t modemLEDPin = redLED;   // MCU pin connected an LED to show modem
                                     // status (-1 if unconnected)
 
// Network connection information
const char* apn = "hologram";  // The APN for the gprs connection
 
DigiXBeeLTEBypass modemXBCT(&modemSerial, modemVccPin, modemStatusPin,
                                      useCTSforStatus, modemResetPin,
                                      modemSleepRqPin, apn);
// Create an extra reference to the modem by a generic name
DigiXBeeLTEBypass modem = modemXBCT;
/** End [xbee_cell_transparent] */
 
 
// ==========================================================================
//  Using the Processor as a Sensor
// ==========================================================================
/** Start [processor_sensor] */
#include <sensors/ProcessorStats.h>
 
// Create the main processor chip "sensor" - for general metadata
const char*    mcuBoardVersion = "v0.5b";  //KA: this might be the wrong board
ProcessorStats mcuBoard(mcuBoardVersion);
/** End [processor_sensor] */

//// ==========================================================================
////  Maxim DS3231 RTC (Real Time Clock)
//// ==========================================================================
///** Start [ds3231] */
//#include <sensors/MaximDS3231.h>
// 
//// Create a DS3231 sensor object
//MaximDS3231 ds3231(1);
///** End [ds3231] */
// 

// ==========================================================================
// Set up the sonar sensor object
// ==========================================================================
/** Start [sonar] */
#include <sensors/MaxBotixSonar.h>
//#include <SoftwareSerial_PCINT12.h>
//
//SoftwareSerial sonarSerial(11, -1);

// Create a reference to the serial port for the sonar 
const int8_t sonarSerial = 11;  // The Sonar Data pin
const int8_t SonarPower = 10;  // excite (power) pin
const int8_t SonarTrigger = -1;  // Trigger pin (a unique negative number if unconnected)
 
MaxBotixSonar sonar(sonarSerial, SonarPower, SonarTrigger);

// Create a new instance of the range variable;
MaxBotixSonar_Range sonar_range(&sonar);
/** End [sonar] */
 
// ==========================================================================
//  Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================
/** Start [variable_arrays] */
Variable* variableList[] = {
  new MaxBotixSonar_Range(&sonar),
};
 
// All UUID's, device registration, and sampling feature information can be
// pasted directly from Monitor My Watershed.  To get the list, click the "View
// token UUID list" button on the upper right of the site page.
 
// *** CAUTION --- CAUTION --- CAUTION --- CAUTION --- CAUTION ***
// Check the order of your variables in the variable list!!!
// Be VERY certain that they match the order of your UUID's!
// Rearrange the variables in the variable list if necessary to match!
// *** CAUTION --- CAUTION --- CAUTION --- CAUTION --- CAUTION ***
const char *UUIDs[] =                                                      // UUID array for device sensors
{
    "a85a0aa1-df81-46ca-92c7-d9e97970050a"    // Distance (MaxBotix_MB7389_Distance)
};
const char *registrationToken = "e8393648-0d3c-4264-9848-a57bc73978ad";   // Device registration token
const char *samplingFeature = "d4a8f7f0-23fa-405e-8abd-535a59c49321";     // Sampling feature UUID
 
// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);
 
// Create the VariableArray object
VariableArray varArray(variableCount, variableList, UUIDs);
/** End [variable_arrays] */
 
 
// ==========================================================================
//  The Logger Object[s]
// ==========================================================================
/** Start [loggers] */
// Create a new logger instance
Logger dataLogger(LoggerID, loggingInterval, &varArray);
/** End [loggers] */
 
 
// ==========================================================================
//  Creating Data Publisher[s]
// ==========================================================================
/** Start [publishers] */
// Create a data publisher for the Monitor My Watershed/EnviroDIY POST endpoint
#include <publishers/EnviroDIYPublisher.h>
EnviroDIYPublisher EnviroDIYPOST(dataLogger, &modem.gsmClient,
                                 registrationToken, samplingFeature);
/** End [publishers] */
 
 
// ==========================================================================
//  Working Functions
// ==========================================================================
/** Start [working_functions] */
// Flashes the LED's on the primary board
void greenredflash(uint8_t numFlash = 4, uint8_t rate = 75) {
    for (uint8_t i = 0; i < numFlash; i++) {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        delay(rate);
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        delay(rate);
    }
    digitalWrite(redLED, LOW);
}
 
// Reads the battery voltage
// NOTE: This will actually return the battery level from the previous update!
float getBatteryVoltage() {
    if (mcuBoard.sensorValues[0] == -9999) mcuBoard.update();
    return mcuBoard.sensorValues[0];
}
 
 
// ==========================================================================
// Arduino Setup Function
// ==========================================================================
/** Start [setup] */
void setup() {
    // Start the primary serial connection
    Serial.begin(serialBaud);
 
    // Print a start-up note to the first serial port
    Serial.print(F("Now running "));
    Serial.print(sketchName);
    Serial.print(F(" on Logger "));
    Serial.println(LoggerID);
    Serial.println();
 
    Serial.print(F("Using ModularSensors Library version "));
    Serial.println(MODULAR_SENSORS_VERSION);
    Serial.print(F("TinyGSM Library version "));
    Serial.println(TINYGSM_VERSION);
    Serial.println();
 
    // Start the serial connection with the modem
    modemSerial.begin(modemBaud);
 
    // Set up pins for the LED's
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();
 
    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    Logger::setRTCTimeZone(0);
 
    // Attach the modem and information pins to the logger
    dataLogger.attachModem(modem);
    modem.setModemLED(modemLEDPin);
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED);
 
    // Begin the logger
    dataLogger.begin();
 
    // Note:  Please change these battery voltages to match your battery
    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("Setting up sensors..."));
        varArray.setupSensors();
    }
 
    // Extra modem set-up - selecting AT&T as the carrier and LTE-M only
    // NOTE:  The code for this could be shortened using the "commandMode" and
    // other XBee specific commands in TinyGSM.  I've written it this way in
    // this example to show how the settings could be changed in either bypass
    // OR transparent mode.
    Serial.println(F("Waking modem and setting Cellular Carrier Options..."));
    modem.modemWake();  // NOTE:  This will also set up the modem
    // Go back to command mode to set carrier options
    for (uint8_t i = 0; i < 5; i++) {
        // Wait the required guard time before entering command mode
        delay(1010);
        modem.gsmModem.streamWrite(GF("+++"));  // enter command mode
        if (modem.gsmModem.waitResponse(2000, GF("OK\r")) == 1) break;
    }
    // Carrier Profile - 0 = Automatic selection
    //                 - 1 = No profile/SIM ICCID selected
    //                 - 2 = AT&T
    //                 - 3 = Verizon
    // NOTE:  To select T-Mobile, you must enter bypass mode!
    modem.gsmModem.sendAT(GF("CP"), 2);
    modem.gsmModem.waitResponse(GF("OK\r"));
    // Cellular network technology - 0 = LTE-M with NB-IoT fallback
    //                             - 1 = NB-IoT with LTE-M fallback
    //                             - 2 = LTE-M only
    //                             - 3 = NB-IoT only
    modem.gsmModem.sendAT(GF("N#"), 2);
    modem.gsmModem.waitResponse();
    // Write changes to flash and apply them
    Serial.println(F("Wait while applying changes..."));
    // Write changes to flash
    modem.gsmModem.sendAT(GF("WR"));
    modem.gsmModem.waitResponse(GF("OK\r"));
    // Apply changes
    modem.gsmModem.sendAT(GF("AC"));
    modem.gsmModem.waitResponse(GF("OK\r"));
    // Reset the cellular component to ensure network settings are changed
    modem.gsmModem.sendAT(GF("!R"));
    modem.gsmModem.waitResponse(30000L, GF("OK\r"));
    // Force reset of the Digi component as well
    // This effectively exits command mode
    modem.gsmModem.sendAT(GF("FR"));
    modem.gsmModem.waitResponse(5000L, GF("OK\r"));
 
    // Sync the clock if it isn't valid or we have battery to spare
    if (getBatteryVoltage() > 3.55 || !dataLogger.isRTCSane()) {
        // Synchronize the RTC with NIST
        // This will also set up the modem
        dataLogger.syncRTC();
    }
 
    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("Setting up file on SD card"));
        dataLogger.turnOnSDcard(
            true);  // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(
            true);  // true = wait for internal housekeeping after write
    }
 
    // Call the processor sleep
    Serial.println(F("Putting processor to sleep\n"));
    dataLogger.systemSleep();
}
/** End [setup] */
 
 
// ==========================================================================
//  Arduino Loop Function
// ==========================================================================
/** Start [loop] */
// Use this short loop for simple data logging and sending
void loop() {
    // Note:  Please change these battery voltages to match your battery
    // At very low battery, just go back to sleep
    if (getBatteryVoltage() < 3.4) {
        dataLogger.systemSleep();
    }
    // At moderate voltage, log data but don't send it over the modem
    else if (getBatteryVoltage() < 3.55) {
        dataLogger.logData();
    }
    // If the battery is good, send the data to the world
    else {
        dataLogger.logDataAndPublish();
    }
}
/** End [loop] */
