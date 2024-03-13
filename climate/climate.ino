#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>

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


// OLED FeatherWing buttons map to different pins depending on board.
// The I2C (Wire) bus may also be different.
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
  #define WIRE Wire
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
  #define WIRE Wire
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
  #define WIRE Wire
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
  #define WIRE Wire
#elif defined(ARDUINO_FEATHER52832)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
  #define WIRE Wire
#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  #define BUTTON_A  9
  #define BUTTON_B  8
  #define BUTTON_C  7
  #define WIRE Wire1
#else // 32u4, M0, M4, nrf52840 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
  #define WIRE Wire
#endif

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &WIRE);

SCD4x mySensor;
DHT11 dht11(4);
int dht11Temperature = 0;
int dht11Humidity = 0;
int readDHT11 = 0;
int services = 0;
auto timer = timer_create_default();
void setup() {
  
  
    Serial.begin(115200);
Wire.begin();
    Serial.println();
    Serial.println();
    Serial.println();

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }
    

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
  Serial.println(mySensor.getTemperatureOffset(), 2); // Print the temperature offset with two decimal places
  mySensor.setTemperatureOffset(10); // Set the temperature offset to 5C
  Serial.print(F("Temperature offset is now: "));
  Serial.println(mySensor.getTemperatureOffset(), 2); // Print the temperature offset with two decimal places

  Serial.print(F("Sensor altitude is currently: "));
  Serial.println(mySensor.getSensorAltitude()); // Print the sensor altitude
  mySensor.setSensorAltitude(40); // Set the sensor altitude to 1000m
  Serial.print(F("Sensor altitude is now: "));
  Serial.println(mySensor.getSensorAltitude()); // Print the sensor altitude
 mySensor.persistSettings(); // Uncomment this line to store the sensor settings in EEPROM

  //Just for giggles, while the periodic measurements are stopped, let's read the sensor serial number
  char serialNumber[13]; // The serial number is 48-bits. We need 12 bytes plus one extra to store it as ASCII Hex
  if (mySensor.getSerialNumber(serialNumber) == true)
  {
    Serial.print(F("The sensor's serial number is: 0x"));
    Serial.println(serialNumber);
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

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

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
    renderDisplay(mySensor.getHumidity(), mySensor.getCO2(), mySensor.getTemperature());
    Serial.println();

    Serial.print(F("CO2(ppm):"));
    Serial.print(mySensor.getCO2());

    Serial.print(F("\tTemperature(C):"));
    Serial.print(mySensor.getTemperature(), 1);

    Serial.print(F("\tHumidity(%RH):"));
    Serial.print(mySensor.getHumidity(), 1);
    Serial.println();
  }
}
bool doReadDHT11(int temp, int hum) { 
  uploadSensorVal(0,1.0);
        Serial.print("Temperature: ");
        Serial.print(temp);
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
void uploadSensorVal(int type, float val) { 
    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(MDNS.IP(0), MDNS.port(0))) {
        Serial.println("Connection failed.");     
        return;
    }
    yield();
    // This will send a request to the server
    //uncomment this line to send an arbitrary string to the server
    //client.print("Send this data to the server");
    //uncomment this line to send a basic document request to the server
    client.print("GET /log?type=1&val=20 HTTP/1.0\r\n\r\n");

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
     String line = client.readStringUntil('\r');
     yield();
    Serial.println(line);
  }
  else
  {
    Serial.println("client.available() timed out ");
  }

    client.stop();

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
