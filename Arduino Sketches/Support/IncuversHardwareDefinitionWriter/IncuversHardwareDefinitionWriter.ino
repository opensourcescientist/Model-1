// Hardware tag to ensure incorrect data is not read.
// M1a = Model 1, release a
#define HARDWARE_IDENT "M1a"
#define HARDWARE_ADDRS 4 

// Structure
struct HardwareStruct {
  byte ident[3];
  // Identification
  byte hVer[3];
  int serial;
  // Temp settings
  byte countOfTempSensors;
  byte sensorAddrDoorTemp[8];
  byte sensorAddrChamberTemp[8];
  // CO2 settings
  bool hasCO2Sensor;
  byte CO2RxPin;
  byte CO2TxPin;
  // O2 settings
  bool hasO2Sensor;
  byte O2RxPin;
  byte O2TxPin;
  // Gas Relay
  byte gasRelayPin;
  bool secondGasRelay;
  byte gasRelayTwoPin;
  // Ethernet
  bool ethernetSupport;
  byte ethernetPin;
};

#include "OneWire.h"
#include "DallasTemperature.h"
#include "Wire.h"
#include "LiquidTWI2.h"
#include <EEPROM.h>  

HardwareStruct hardwareDefinition;
float tempDoor;
float tempChmb;
OneWire oneWire(4);                  // Setup a oneWire instance to communicate with ANY OneWire devices
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
LiquidTWI2 lcd(0);

/*  ATMEGA328 Pins In Use:
 *    D2 - Rx for CO2 sensor
 *    D3 - Tx for CO2 sensor
 *    D4 - DS18B20 temperature sensors
 *    D5 - Chamber Heater relay
 *    D6 - Fan relay
 *    D7 - CO2 Solenoid relay
 *    D8 - Door Heater relay
 *    D9 - Unused
 *    D10 - Ethernet 
 *    A4 - SDA (I2C) - Comms with MCP23017 I/O port expander  
 *    A5 - SCL (I2C) - Comms with MCP23017 I/O port expander 
 */
void setup() {
  Serial.begin(9600);
  sensors.begin();
  lcd.setMCPType(LTI_TYPE_MCP23017);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print(F("Incuvers Hwrdwr"));
  lcd.setCursor(0,1);
  lcd.print(F("    Settings"));  
  Wire.begin(); // wake up I2C bus
  Wire.beginTransmission(0x20);
  Wire.write(0x00); // IODIRA register
  Wire.write(0x01); // set all of port A to inputs
  Wire.endTransmission(); 
  delay(1000);
  lcd.clear(); 
  
  // Hardware version information.  0.9.3 is the PCB revision for Model 1
  hardwareDefinition.hVer[0]=0;
  hardwareDefinition.hVer[1]=9;
  hardwareDefinition.hVer[2]=3;
  // Hardware serial number
  hardwareDefinition.serial=9999;             // Serial number as printed inside the top cover
  // Temperature sensors
  hardwareDefinition.countOfTempSensors=2;    // Count of temperature sensors installed
  for (int i = 0; i <8; i++) {
    hardwareDefinition.sensorAddrDoorTemp[i] = 0;
    hardwareDefinition.sensorAddrChamberTemp[i] = 0;
  }
  // CO2 sensor
  hardwareDefinition.hasCO2Sensor=true;       // Is there a CO2 sensor present?
  hardwareDefinition.CO2RxPin=2;              // Default: 2
  hardwareDefinition.CO2TxPin=3;              // Default: 3
  // Oxygen sensor
  hardwareDefinition.hasO2Sensor=false;       // Is there an O2 sensor present?
  hardwareDefinition.O2RxPin=0;               // 
  hardwareDefinition.O2TxPin=0;               // 
  // Gas Relay
  hardwareDefinition.gasRelayPin = 7;         // Default: 7
  hardwareDefinition.secondGasRelay=false;    // Is there a pin for a second gas valve/relay present?
  hardwareDefinition.gasRelayTwoPin = 0;      // 
  // Ethernet
  hardwareDefinition.ethernetSupport=false;   // Is there an ethernet shield installed?
  hardwareDefinition.ethernetPin = 0;         // Default: 10 (when an O2 sensor is not installed)
}

void loop() {
  DoTempSensorAssign();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Saving "));
  lcd.print(sizeof(hardwareDefinition));
  lcd.print(F(" bytes"));
  lcd.setCursor(0,1);
  lcd.print(F("at "));
  lcd.print(HARDWARE_ADDRS);
  lcd.print(F(" w/ ID "));
  lcd.print(HARDWARE_IDENT);
    
  PerformSaveHardwareDefinition();
  delay (1500);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Setup for dev"));
  lcd.setCursor(0,1);
  lcd.print(hardwareDefinition.serial);
  lcd.print(F(" on "));
  lcd.print(hardwareDefinition.hVer[0]);
  lcd.print(F("."));
  lcd.print(hardwareDefinition.hVer[1]);
  lcd.print(F("."));
  lcd.print(hardwareDefinition.hVer[2]);
  delay(1500);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Done!"));
  delay (1000);

  while (true) {
    // put your main code here, to run repeatedly:
    Serial.println(F("Writing hardware definitions to EEPROM was a success, nothing left to do."));
    lcd.setCursor(0,0);
    lcd.print(F("Please flash w/"));
    lcd.setCursor(0,1);
    lcd.print(F("latest Incuvers"));
    delay(4000);
    lcd.clear();
    delay(1000);
  }
}

void PerformSaveHardwareDefinition() {
  Serial.println(F("Call to PerformSaveHardwareDefinition()"));

  for (int i=0; i < 3; i++ ) {
    hardwareDefinition.ident[i] = HARDWARE_IDENT[i];
  }
  
  Serial.print(F("  ID: "));
  Serial.print(HARDWARE_IDENT);
  Serial.print(", ");
  Serial.print(sizeof(hardwareDefinition));
  Serial.print(" bytes");

  for (unsigned int i=0; i < sizeof(hardwareDefinition); i++) {
    EEPROM.write(HARDWARE_ADDRS + i, *((char*)&hardwareDefinition + i));
    Serial.print(".");
  }
  
  Serial.println("  Done.");
  Serial.println(F("End of PerformSaveHardwareDefintion()"));
}

void GetTemperatureReadings() {
  Serial.println(F("Call to GetTemperatureReadings()"));
  // Request the temperatures
  sensors.requestTemperatures();
  // Record the values
  tempDoor = sensors.getTempC(hardwareDefinition.sensorAddrDoorTemp);
  tempChmb = sensors.getTempC(hardwareDefinition.sensorAddrChamberTemp);

  for (int i=0; i < 8; i++) {
    Serial.print(hardwareDefinition.sensorAddrDoorTemp[i]);
  }
  Serial.print(F(" (Door) temperature reading: "));
  Serial.print(tempDoor);
  Serial.println(F("*C"));
  for (int i=0; i < 8; i++) {
    Serial.print(hardwareDefinition.sensorAddrChamberTemp[i]);
  }
  Serial.print(F(" (Chamber) temperature reading: "));
  Serial.print(tempChmb);
  Serial.println("*C");
}

int PollButton() {
    Wire.beginTransmission(0x20); // Connect to chip
    Wire.write(0x12);             // Address port A
    Wire.endTransmission();       // Close connection
    Wire.requestFrom(0x20, 1);    // Request one Byte
    return(Wire.read());          // Return the state 
                                  //  0 = no buttons pushed
                                  //  1 = upper button pushed
                                  //  2 = lower button pushed
                                  //  3 = both buttons pushed
}

int GetButtonState() {
  // instead of reading the button once which can cause misreads during a button change operation, if the first reading is not 0, take two more readings a few moments apart to verify what the user wants to do.
  int f, s, t, r;

  r = 0;
  
  f = PollButton();

  if (f != 0) {
    // let's do some extra polling, to make sure we get the right value.
    delay(200);
    s = PollButton();
    delay(200);
    t = PollButton();
  
    if (f == 3 || s == 3 || t == 3) {
      r = 3;
    } else if (f == 2 || s == 2 || t == 2) {
      r = 2;
    } else if (f == 1 || s == 1 || t == 1) {
      r = 1;
    }  
  }
  
  return r;
}

void DoTempSensorAssign() {
  int i, j, k;
  byte addrA[4][8];
  byte addr[8];
  boolean doLoop = true;
  boolean redraw = true;
  int userInput;
  int serialZeros;
  
  j=0;
  while(oneWire.search(addr)) {
    for( i = 0; i < 8; i++) {
      addrA[j][i] = addr[i]; 
    }
    j++;
  }

  hardwareDefinition.countOfTempSensors = j;

  Serial.print("Found ");
  Serial.print(j);
  Serial.println(F(" OneWire devices"));

  lcd.setCursor(0,0);
  lcd.print(F("Found "));
  lcd.print(j);
  lcd.print(F(" devices"));
  if (j != 2) {
    lcd.setCursor(0,1);
    lcd.print(F("    WARNING!"));
    Serial.println(F("Warning: we didn't detect the two standard temperature sensors we expected to.  The incubator may not work as intented."));
  }
  delay (1000);
  
  
  if (j == 0 ) {
    // no temp sensors
  } else if (j >= 1) {
    serialZeros = 0;
    for (i = 0; i < 8; i++) {
      if (addrA[0][i] == 0) {
        serialZeros++;
      }
    }
    if (serialZeros < 6) {
      Serial.print(F("Setting Door Sensor Serial: "));
      for (k = 0; k < 8; k++) {
        hardwareDefinition.sensorAddrDoorTemp[k] = addrA[0][k];
        Serial.print(hardwareDefinition.sensorAddrDoorTemp[k]);
      }
      Serial.println(F("."));
    }
  } 
  
  if (j >= 2) {
    serialZeros = 0;
    for (i = 0; i < 8; i++) {
      if (addrA[1][i] == 0) {
        serialZeros++;
      }
    }
    if (serialZeros < 6) {
      Serial.print(F("Setting Chamber Sensor Serial: "));
      for (k = 0; k < 8; k++) {
        hardwareDefinition.sensorAddrChamberTemp[k] = addrA[1][k];
        Serial.print(hardwareDefinition.sensorAddrChamberTemp[k]);
      }
      Serial.println(F("."));
    }
  }
   
  while (doLoop) {
    if (redraw) {
      GetTemperatureReadings();
      lcd.setCursor(0, 0);
      lcd.print("Door: ");
      lcd.print(tempDoor, 1);
      lcd.print("    ");
      lcd.setCursor(0, 1);
      lcd.print("Chmbr: ");
      lcd.print(tempChmb, 1);
      lcd.print("    ");

      lcd.setCursor(13, 0);
      lcd.print("Swp");
      lcd.setCursor(13, 1);
      lcd.print("Sav");
    }
   
    userInput = GetButtonState();
    
    switch (userInput) {
      case 0:
        delay(250);
        redraw=true;
        break;
      case 1:
        // Swap
        for (i = 0; i < 8; i++) {
          addr[i] = hardwareDefinition.sensorAddrDoorTemp[i];
          hardwareDefinition.sensorAddrDoorTemp[i] = hardwareDefinition.sensorAddrChamberTemp[i];
          hardwareDefinition.sensorAddrChamberTemp[i] = addr[i];
        }
        redraw = true;
        break;
      case 2:
        // Exit
        doLoop=false;
        break;
      case 3:
        // exit
        doLoop=false;
        break;
    }
  }
}
