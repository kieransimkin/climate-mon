

#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoUniqueID.h>
#include <Wire.h>
#include "SparkFun_SCD4x_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD4x
#include "liblongtail/ctypes.h"
#include <Adafruit_SSD1306.h>
// Include the DHT11 library for interfacing with the sensor.
#include <DHT11.h>

#include <ESPAutoWiFiConfig.h>
#include <ESPmDNS.h>
#include <Ed25519.h>
#include <WebServer.h>
#include <arduino-timer.h>
#include <blst.hpp>
#include <RNG.h>
#include <Crypto.h>
#include <BLAKE2b.h>
#include <Preferences.h>
#include <esp_smartconfig.h>
#include <esp_netif_types.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <string>
#include <tuple>
#include <vector>
#include <stdint.h>
#include "types.h"


using namespace blst;
Preferences preferences;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
uint8_t mac[6];
    uint8_t privateKey[32];
    uint8_t publicKey[32];
    unsigned char publicKeyHash[29];
SCD4x mySensor;
DHT11 dht11(4);
int dht11Temperature = 0;
int dht11Humidity = 0;
int readDHT11 = 0;
int services = 0;
auto timer = timer_create_default();
void scanNetworks() { 
      int n = WiFi.scanNetworks();
    Serial.println("Scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.printf("%2d",i + 1);
            Serial.print(" | ");
            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            Serial.print(" | ");
            Serial.printf("%4d", WiFi.RSSI(i));
            Serial.print(" | ");
            Serial.printf("%2d", WiFi.channel(i));
            Serial.print(" | ");
            switch (WiFi.encryptionType(i))
            {
            case WIFI_AUTH_OPEN:
                Serial.print("open");
                break;
            case WIFI_AUTH_WEP:
                Serial.print("WEP");
                break;
            case WIFI_AUTH_WPA_PSK:
                Serial.print("WPA");
                break;
            case WIFI_AUTH_WPA2_PSK:
                Serial.print("WPA2");
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                Serial.print("WPA+WPA2");
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                Serial.print("WPA2-EAP");
                break;
            case WIFI_AUTH_WPA3_PSK:
                Serial.print("WPA3");
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                Serial.print("WPA2+WPA3");
                break;
            case WIFI_AUTH_WAPI_PSK:
                Serial.print("WAPI");
                break;
            default:
                Serial.print("unknown");
            }
            Serial.println();
            delay(10);
        }
    }
    Serial.println("");

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();
};
static uint32_t bech32_polymod_step(uint32_t pre) {
    uint8_t b = pre >> 25;
    return ((pre & 0x1FFFFFF) << 5) ^
        (-((b >> 0) & 1) & 0x3b6a57b2UL) ^
        (-((b >> 1) & 1) & 0x26508e6dUL) ^
        (-((b >> 2) & 1) & 0x1ea119faUL) ^
        (-((b >> 3) & 1) & 0x3d4233ddUL) ^
        (-((b >> 4) & 1) & 0x2a1462b3UL);
}

static uint32_t bech32_final_constant(bech32_encoding enc) {
    if (enc == BECH32_ENCODING_BECH32) return 1;
    if (enc == BECH32_ENCODING_BECH32M) return 0x2bc830a3;
    assert(0);
}

const char bech32_charset[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

const int8_t bech32_charset_rev[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
};
int toBase32(byte* in, long length, byte*& out, boolean usePadding)
{
  char base32StandardAlphabet[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"};
  char standardPaddingChar = '='; 

  int result = 0;
  int count = 0;
  int bufSize = 8;
  int index = 0;
  int size = 0; // size of temporary array
  byte* temp = NULL;

  if (length < 0 || length > 268435456LL) 
  { 
    return 0;
  }

  size = 8 * ceil(length / 4.0); // Calculating size of temporary array. Not very precise.
  temp = (byte*)malloc(size); // Allocating temporary array.

  if (length > 0)
  {
    int buffer = in[0];
    int next = 1;
    int bitsLeft = 8;
    
    while (count < bufSize && (bitsLeft > 0 || next < length))
    {
      if (bitsLeft < 5)
      {
        if (next < length)
        {
          buffer <<= 8;
          buffer |= in[next] & 0xFF;
          next++;
          bitsLeft += 8;
        }
        else
        {
          int pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      index = 0x1F & (buffer >> (bitsLeft -5));

      bitsLeft -= 5;
      temp[result] = (byte)index;
      result++;
    }
  }
  
  if (usePadding)
  {
    int pads = (result % 8);
    if (pads > 0)
    {
      pads = (8 - pads);
      for (int i = 0; i < pads; i++) 
      {
        temp[result] = standardPaddingChar;
        result++;
      }
    }
  }

  out = (byte*)malloc(result);
  memcpy(out, temp, result);
  free(temp);
  
  return result;
}
int bech32_encode(char *output, const char *hrp, const uint8_t *data, size_t data_len, size_t max_output_len, bech32_encoding enc) {
    uint32_t chk = 1;
    size_t i = 0;
    while (hrp[i] != 0) {
        int ch = hrp[i];
        if (ch < 33 || ch > 126) {
            return 0;
        }

        if (ch >= 'A' && ch <= 'Z') return 0;
        chk = bech32_polymod_step(chk) ^ (ch >> 5);
        ++i;
    }
    if (i + 7 + data_len > max_output_len) return 0;
    chk = bech32_polymod_step(chk);
    while (*hrp != 0) {
        chk = bech32_polymod_step(chk) ^ (*hrp & 0x1f);
        *(output++) = *(hrp++);
    }
    *(output++) = '1';
    for (i = 0; i < data_len; ++i) {
        if (*data >> 5) {
          Serial.println("Stuck");
          Serial.println(*data);
          return 0;
        }
        chk = bech32_polymod_step(chk) ^ (*data);
        *(output++) = bech32_charset[*(data++)];
    }
    for (i = 0; i < 6; ++i) {
        chk = bech32_polymod_step(chk);
    }
    chk ^= bech32_final_constant(enc);
    for (i = 0; i < 6; ++i) {
        *(output++) = bech32_charset[(chk >> ((5 - i) * 5)) & 0x1f];
    }
    *output = 0;
    return 1;
}

bech32_encoding bech32_decode(char* hrp, uint8_t *data, size_t *data_len, const char *input, size_t max_input_len) {
    uint32_t chk = 1;
    size_t i;
    size_t input_len = strlen(input);
    size_t hrp_len;
    int have_lower = 0, have_upper = 0;
    if (input_len < 8 || input_len > max_input_len) {
        return BECH32_ENCODING_NONE;
    }
    *data_len = 0;
    while (*data_len < input_len && input[(input_len - 1) - *data_len] != '1') {
        ++(*data_len);
    }
    hrp_len = input_len - (1 + *data_len);
    if (1 + *data_len >= input_len || *data_len < 6) {
        return BECH32_ENCODING_NONE;
    }
    *(data_len) -= 6;
    for (i = 0; i < hrp_len; ++i) {
        int ch = input[i];
        if (ch < 33 || ch > 126) {
            return BECH32_ENCODING_NONE;
        }
        if (ch >= 'a' && ch <= 'z') {
            have_lower = 1;
        } else if (ch >= 'A' && ch <= 'Z') {
            have_upper = 1;
            ch = (ch - 'A') + 'a';
        }
        hrp[i] = ch;
        chk = bech32_polymod_step(chk) ^ (ch >> 5);
    }
    hrp[i] = 0;
    chk = bech32_polymod_step(chk);
    for (i = 0; i < hrp_len; ++i) {
        chk = bech32_polymod_step(chk) ^ (input[i] & 0x1f);
    }
    ++i;
    while (i < input_len) {
        int v = (input[i] & 0x80) ? -1 : bech32_charset_rev[(int)input[i]];
        if (input[i] >= 'a' && input[i] <= 'z') have_lower = 1;
        if (input[i] >= 'A' && input[i] <= 'Z') have_upper = 1;
        if (v == -1) {
            return BECH32_ENCODING_NONE;
        }
        chk = bech32_polymod_step(chk) ^ v;
        if (i + 6 < input_len) {
            data[i - (1 + hrp_len)] = v;
        }
        ++i;
    }
    if (have_lower && have_upper) {
        return BECH32_ENCODING_NONE;
    }
    if (chk == bech32_final_constant(BECH32_ENCODING_BECH32)) {
        return BECH32_ENCODING_BECH32;
    } else if (chk == bech32_final_constant(BECH32_ENCODING_BECH32M)) {
        return BECH32_ENCODING_BECH32M;
    } else {
        return BECH32_ENCODING_NONE;
    }
}

int bech32_convert_bits(uint8_t* out, size_t* outlen, int outbits, const uint8_t* in, size_t inlen, int inbits, int pad) {
    uint32_t val = 0;
    int bits = 0;
    uint32_t maxv = (((uint32_t)1) << outbits) - 1;
    while (inlen--) {
        val = (val << inbits) | *(in++);
        bits += inbits;
        while (bits >= outbits) {
            bits -= outbits;
            out[(*outlen)++] = (val >> bits) & maxv;
        }
    }
    if (pad) {
        if (bits) {
            out[(*outlen)++] = (val << (outbits - bits)) & maxv;
        }
    } else if (((val << (outbits - bits)) & maxv) || bits >= inbits) {
        return 0;
    }
    return 1;
}

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
        Serial.print("Connection failed: "); 
        Serial.print("IP: "); 
        Serial.print(MDNS.IP(c));
        Serial.print(" Port: ");
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

void sendQuery(WiFiClient &client,ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, int8_t exponent=0) { 
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
void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, float val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%f",val);
    client.print("&ctype=float");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, double val, int8_t exponent=0) { 
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
void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, uint8_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint8");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, int8_t val, int8_t exponent=0) { 
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
void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, uint16_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint16");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, int16_t val, int8_t exponent=0) { 
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
void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, uint32_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint32");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, int32_t val, int8_t exponent=0) { 
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
void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, uint64_t val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%u",val);
    client.print("&ctype=uint64");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}

void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, int64_t val, int8_t exponent=0) { 
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
void uploadSensorVal(ReadingType readingType, ReadingUnit readingUnit, uint16_t sensorIndex, char *val, int8_t exponent=0) { 
    WiFiClient client;
    if (!openLogRequest(client)) return fail(client);
    sendQuery(client,readingType,readingUnit,sensorIndex,exponent);
    client.printf("%s",val);
    client.print("&ctype=string");
    addHeaders(client);
    if (!awaitReply(client)) return fail(client);
    client.stop();
}
/** Convert a master seed into a master node (an extended private key), as
  * described by the BIP32 specification.
  * \param master_node The master node will be written here. This must be a
  *                    byte array with space for #NODE_LENGTH bytes.
  * \param seed Input seed, which is an arbitrary array of bytes.
  * \param seed_length Length of input seed in number of bytes.
  */
void bip32SeedToNode(uint8_t *master_node, const uint8_t *seed, const unsigned int seed_length)
{
	hmacSha512(master_node, (const uint8_t *)"Bitcoin seed", 12, seed, seed_length);
}
void setup() {
  
  
    Serial.begin(115200);
    RNG.begin(" Climate Mon 1.0");
Wire.begin();
    Serial.println();
    Serial.println();
    Serial.println("\n");
Serial.flush();
  RNG.stir(UniqueID, UniqueIDsize);
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
  
  scanNetworks();
  setESPAutoWiFiConfigDebugOut(Serial); // turns on debug output for the ESPAutoWiFiConfig code
  
  const uint8_t protocol = WIFI_PROTOCOL_11B| WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR;
  esp_wifi_set_protocol(WIFI_IF_STA, protocol);
  WiFi.setTxPower(WIFI_POWER_19_5dBm );
  if (ESPAutoWiFiConfigSetup(2, true,0)) { // check if we should start access point to configure WiFi settings
    return; // in config mode so skip rest of setup
  }
  
  WiFi.macAddress(mac);
  RNG.stir(mac, sizeof mac);
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println();
  String psk = WiFi.psk();
  RNG.stir((const uint8_t*)psk.c_str(),psk.length());
  Serial.println("Got PSK:");
  Serial.println(psk);

   preferences.begin("climate-mon", false);
  if (preferences.isKey("pkey")) { 
    Serial.println("Got existing key, loading it..");
    preferences.getBytes("pkey", privateKey, sizeof privateKey);
  } else { 
    Serial.println("Generating new key pair...");
    Ed25519::generatePrivateKey(privateKey);
    preferences.putBytes("pkey", privateKey, sizeof privateKey);
  }
    Serial.println("Generating public key...");
  Ed25519::derivePublicKey(publicKey, privateKey);
  
   BLAKE2b blake;
  blake.reset(28);
  blake.update(publicKey, sizeof publicKey);
  blake.finalize(publicKeyHash, 28);
  publicKeyHash[28]='\0';
  byte *buf = 0;
  toBase32(publicKeyHash,28,(byte*&)buf,true);
    
  
  
  char bech[100];
  bech32_encode(bech, "test", buf, 28,90,BECH32_ENCODING_BECH32M);
  //bech32_encode(bech, "", 0, publicKeyHash, 28, 60, true);
  char str[28 + 1];
    memcpy(str, buf, 28);
    str[28] = 0; // Null termination.
    Serial.println(str);
  Serial.println();
  Serial.println(bech[4]);
  Serial.println("Got pubkey bech:");
  Serial.println(bech);
  delay(20000);
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
      uploadSensorVal(ReadingType::REF_OFFSET,ReadingUnit::CELSIUS, 1, to);
    
  
  

  Serial.print(F("Sensor altitude is currently: "));
  uint16_t alt = mySensor.getSensorAltitude();
  Serial.println(alt); // Print the sensor altitude
      uploadSensorVal(ReadingType::REF_ALT,ReadingUnit::METERS, 1, alt);
      
 mySensor.persistSettings(); // Uncomment this line to store the sensor settings in EEPROM
  uploadSensorVal(ReadingType::SENSOR, ReadingUnit::SENSOR, 1, "SCD41");
  uploadSensorVal(ReadingType::SENSOR, ReadingUnit::SENSOR, 0, "DHT11");
  //Just for giggles, while the periodic measurements are stopped, let's read the sensor serial number
  char serialNumber[13]; // The serial number is 48-bits. We need 12 bytes plus one extra to store it as ASCII Hex
  if (mySensor.getSerialNumber(serialNumber) == true)
  {
    Serial.print(F("The sensor's serial number is: 0x"));
    Serial.println(serialNumber);
    uploadSensorVal(ReadingType::SERIALNO, ReadingUnit::SERIALNO, 1, serialNumber);
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
    uploadSensorVal(ReadingType::TEMP,ReadingUnit::CELSIUS, 1, t);
    uploadSensorVal(ReadingType::HUMID,ReadingUnit::PERCENT, 1, h);
    uploadSensorVal(ReadingType::CO2,ReadingUnit::PPM, 1, c);

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
  uploadSensorVal(ReadingType::TEMP,ReadingUnit::CELSIUS, 0, temp);
        Serial.print("Temperature: ");
        Serial.print(temp);
        uploadSensorVal(ReadingType::HUMID,ReadingUnit::PERCENT, 0, hum);
        Serial.print(" °C\tHumidity: ");
        Serial.print(hum);
        Serial.println(" %");
        return true;
}
void loop() {
  RNG.loop();
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
        Serial.println(" service(s) found");z
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
