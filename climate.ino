#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoUniqueID.h>
#include <Wire.h>
#include "SparkFun_SCD4x_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD4x

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Include the DHT11 library for interfacing with the sensor.
#include <DHT11.h>

#include <ESPAutoWiFiConfig.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <arduino-timer.h>

#define TYPE_TEMP 0
#define TYPE_HUMID 1
#define TYPE_CO2 2
#define TYPE_PRES 3
#define TYPE_REF_ALT 4
#define TYPE_REF_OFFSET 5
#define TYPE_SENSOR 6
#define TYPE_SERIAL 7

#define UNIT_CELSIUS 0
#define UNIT_FAHRENHEIT 1
#define UNIT_PERCENT 2
#define UNIT_PPM 3
#define UNIT_METERS 4
#define UNIT_FEET 5
#define UNIT_SENSOR 6
#define UNIT_SERIAL 7
#define UNIT_HZ 8
#define UNIT_DB 9

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

SCD4x mySensor;
DHT11 dht11(4);
int dht11Temperature = 0;
int dht11Humidity = 0;
int readDHT11 = 0;
int services = 0;
auto timer = timer_create_default();
int openLogRequest(WiFiClient &client) { 
  int c = 0;
    int connected = 0;
    
    for (int c = 0; c < services; c++) { 
      
    
      if (client.connect(MDNS.IP(c), MDNS.port(c))) { 
        connected = 1;
        break;
      }
      yield();
    }
    if (connected == 0) { 
        Serial.println("Connection failed."); 
        Serial.print("IP: "); 
        Serial.print(MDNS.IP(c));
        Serial.println();
        Serial.print("Port: ");
        Serial.print(MDNS.port(c));
        Serial.println();
        client.stop();
        return 0;
    } else { return 1; }
}
void addHeaders(WiFiClient &client) { 
 client.print(" HTTP/1.0\r\nX-UID: ");
    for (size_t i = 0; i < UniqueIDsize; i++) {
		  if (UniqueID[i] < 0x10)
			  client.print("0");
		  client.print(UniqueID[i], HEX);
	  }
    client.print("\r\n\r\n");
}
int awaitReply(WiFiClient &client) { 
  int maxloops = 0;

  //wait for the server's reply to become available
  while (!client.available() && maxloops < 1000)
  {
    maxloops++;
    delay(1); //delay 1 msec
    yield();
  }
  if (client.available() > 0)
  {
    //read back one line from the server
    // Ugh using String, might just read one byte at a time into a char[3] until we've got a status code
     String line = client.readStringUntil('\r');
    
     yield();
    //Serial.println(line);
    return true;
  }
  else
  {
    Serial.println("client.available() timed out ");
    return false;
  }
}    void fail(WiFiClient &client) { 
      Serial.println("Failed to send sensor val, we should probably be discover mdns");
      client.stop();
    }

void sendQuery(WiFiClient &client,uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, int8_t exponent=0) { 
    client.print("GET /log?type=");
    client.printf("%u",readingType);
    client.print("&unit=");
    client.printf("%u",readingUnit);
    client.print("&idx=");
    client.printf("%u",sensorIndex);
    client.print("&exp=");
    client.printf("%i",exponent);
    client.print("&val=");
}
void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, float val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%f",val);
    client.print("&ctype=float");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, double val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%lf",val);
    client.print("&ctype=double");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}
// 8 bit ints:
void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, uint8_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint8");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, int8_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%i",val);
    client.print("&ctype=int8");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}
// 16 bit ints:
void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, uint16_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint16");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, int16_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%i",val);
    client.print("&ctype=int16");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}
// 32 bit ints: 
void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, uint32_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint32");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, int32_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%i",val);
    client.print("&ctype=int32");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}
// 64 bit ints:
void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, uint64_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint64");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, int64_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%i",val);
    client.print("&ctype=int64");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

// String:
void uploadSensorVal(uint32_t readingType, uint32_t readingUnit, uint16_t sensorIndex, char *val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%s",val);
    client.print("&ctype=string");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}
void setup() {
  
  
    Serial.begin(115200);
  
Wire.begin();
    Serial.println();
    Serial.println();
    Serial.println("\n");
Serial.flush();

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }
  
    Serial.print("UniqueID: ");
	for (size_t i = 0; i < UniqueIDsize; i++)
	{
		if (UniqueID[i] < 0x10)
			Serial.print("0");
		Serial.print(UniqueID[i], HEX);
		
	}
	Serial.println();
  

  setESPAutoWiFiConfigDebugOut(Serial); // turns on debug output for the ESPAutoWiFiConfig code


  if (ESPAutoWiFiConfigSetup(2, true,0)) { // check if we should start access point to configure WiFi settings
    return; // in config mode so skip rest of setup
  }

      if (!MDNS.begin("ESP32_Browser")) {
        Serial.println("Error setting up MDNS responder!");
        while(1){
            delay(1000);
        }
    }
    services = MDNS.queryService("longtail", "tcp");
    if (services == 0) {
        Serial.println("no services found");

    }
    listServices();
 //.begin will start periodic measurements for us (see the later examples for details on how to override this)
  if (mySensor.begin() == false)
  {
    Serial.println(F("Sensor not detected. Please check wiring. Freezing..."));
    while (1)
      ;
  }
   //We need to stop periodic measurements before we can change the sensor signal compensation settings
  if (mySensor.stopPeriodicMeasurement() == true)
  {
    Serial.println(F("Periodic measurement is disabled!"));
  }  

  //Now we can change the sensor settings.
  //There are three signal compensation commands we can use: setTemperatureOffset; setSensorAltitude; and setAmbientPressure

  Serial.print(F("Temperature offset is currently: "));
  float to = mySensor.getTemperatureOffset();
  Serial.println(to, 2); // Print the temperature offset with two decimal places
      uploadSensorVal(TYPE_REF_OFFSET,UNIT_CELSIUS, 1, to);
    
  
  

  Serial.print(F("Sensor altitude is currently: "));
  uint16_t alt = mySensor.getSensorAltitude();
  Serial.println(alt); // Print the sensor altitude
      uploadSensorVal(TYPE_REF_ALT,UNIT_METERS, 1, alt);
      
 mySensor.persistSettings(); // Uncomment this line to store the sensor settings in EEPROM
  uploadSensorVal(TYPE_SENSOR, UNIT_SENSOR, 1, "SCD41");
  uploadSensorVal(TYPE_SENSOR, UNIT_SENSOR, 0, "DHT11");
  //Just for giggles, while the periodic measurements are stopped, let's read the sensor serial number
  char serialNumber[13]; // The serial number is 48-bits. We need 12 bytes plus one extra to store it as ASCII Hex
  if (mySensor.getSerialNumber(serialNumber) == true)
  {
    Serial.print(F("The sensor's serial number is: 0x"));
    Serial.println(serialNumber);
    uploadSensorVal(TYPE_SERIAL, UNIT_SERIAL, 1, serialNumber);
  }

  //Finally, we need to restart periodic measurements
  if (mySensor.startPeriodicMeasurement() == true)
  {
    Serial.println(F("Periodic measurements restarted!"));
  }

    

  Serial.println("OLED FeatherWing test");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  Serial.println("IO test");

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Connecting to Eek\n'adafruit':");
  display.print("connected!");
  display.println("IP: 10.0.1.23");
  display.println("Sending val #0");
  
  dht11.setDelay(5000);
  timer.every(5000, setReadDHT11);
  
  display.setCursor(0,0);
  display.display(); // actually display all of the above
}
bool setReadDHT11(void *) { 
  readDHT11=true;
  return true;
}
void renderDisplay(float tl, int tr, float main) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(tl,1);
  display.print('%');
  display.setCursor(64,0);
  display.print(tr, 1);
  display.print(F("ppm"));
  display.setCursor(0,16);
  display.setTextSize(2);
  display.print(main,1);
  display.print(F(" "));
  display.print((char) 247);
  display.print(F("C"));
  display.display();
}


void readSCD41() { 
   if (mySensor.readMeasurement()) // readMeasurement will return true when fresh data is available
  {
    float h = mySensor.getHumidity();
    uint16_t c = mySensor.getCO2();
    float t = mySensor.getTemperature();
    renderDisplay(h, c, t);
    yield();
    uploadSensorVal(TYPE_TEMP,UNIT_CELSIUS, 1, t);
    uploadSensorVal(TYPE_HUMID,UNIT_PERCENT, 1, h);
    uploadSensorVal(TYPE_CO2,UNIT_PPM, 1, c);

    Serial.print(F("CO2(ppm):"));
    Serial.print(c);

    Serial.print(F("\tTemperature(C):"));
    Serial.print(t, 1);

    Serial.print(F("\tHumidity(%RH):"));
    Serial.print(h, 1);
    Serial.println();
  }
}
bool doReadDHT11(int temp, int hum) { 
  uploadSensorVal(TYPE_TEMP,UNIT_CELSIUS, 0, temp);
        Serial.print("Temperature: ");
        Serial.print(temp);
        uploadSensorVal(TYPE_HUMID,UNIT_PERCENT, 0, hum);
        Serial.print(" °C\tHumidity: ");
        Serial.print(hum);
        Serial.println(" %");
        return true;
}
void loop() {
  
  if (ESPAutoWiFiConfigLoop()) {  // handle WiFi config webpages
    return;  // skip the rest of the loop until config finished
  }
  readSCD41();
  if (readDHT11==true) {
    int result = dht11.readTemperatureHumidity(dht11Temperature, dht11Humidity);
    if (result != 0) {
        Serial.println(DHT11::getErrorString(result));
    } else { 
      doReadDHT11(dht11Temperature, dht11Humidity);
    }
    readDHT11=!readDHT11;
  }
  //yield();
  timer.tick(); 

}
 

void listServices() { 
  Serial.print(services);
        Serial.println(" service(s) found");
        for (int i = 0; i < services; ++i) {
            // Print details for each service found
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(MDNS.hostname(i));
            Serial.print(" (");
            Serial.print(MDNS.IP(i));
            Serial.print(":");
            Serial.print(MDNS.port(i));
            Serial.println(")");
        }
}

void browseService(const char * service, const char * proto){
    Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
        Serial.println("no services found");
    } else {
        Serial.print(n);
        Serial.println(" service(s) found");
        for (int i = 0; i < n; ++i) {
            // Print details for each service found
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(MDNS.hostname(i));
            Serial.print(" (");
            Serial.print(MDNS.IP(i));
            Serial.print(":");
            Serial.print(MDNS.port(i));
            Serial.println(")");
        }
    }
    Serial.println();
}
