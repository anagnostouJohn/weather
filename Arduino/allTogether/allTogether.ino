#include "DHT.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>

#define S0 D1                             /* Assign Multiplexer pin S0 connect to pin D0 of NodeMCU */
#define S1 D2                             /* Assign Multiplexer pin S1 connect to pin D1 of NodeMCU */
#define S2 D3                             /* Assign Multiplexer pin S2 connect to pin D2 of NodeMCU */
#define S3 D4                             /* Assign Multiplexer pin S3 connect to pin D3 of NodeMCU */
#define SIG A0                            /* Assign SIG pin as Analog output for all 16 channels of Multiplexer to pin A0 of NodeMCU */



#define LIGHT_SENSOR_PIN A0 // ESP32 pin GIOP36 (ADC0)
#define sensorPin D0
#define DHTPIN D7  
#define DHTTYPE DHT22 


int decimal = 2;                          // Decimal places of the sensor value outputs 
int photo;                            /* Assign the name "sensor0" as analog output value from Channel C0 */
int wind;  


DHT dht(DHTPIN, DHTTYPE);





const int mqtt_port = 8883;  // MQTT port (TLS)
const char *mqtt_broker = "192.168.1.16";  // EMQX broker endpoint
const char *mqtt_topic = "weather";     // MQTT topic
// const char *mqtt_username = "emqx";  // MQTT username for authentication
// const char *mqtt_password = "public";  // MQTT password for authentication

// NTP Server settings
const char *ntp_server = "pool.ntp.org";     // Default NTP server
// const char* ntp_server = "cn.pool.ntp.org"; // Recommended NTP server for users in China
const long gmt_offset_sec = 0;            // GMT offset in seconds (adjust for your time zone)
const int daylight_offset_sec = 0;        // Daylight saving time offset in seconds

// WiFi and MQTT client initialization
BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

// SSL certificate for MQTT broker CA.crt<<<<<<<<<<<<<<<<<<<<<<<
// Load DigiCert Global Root G2, which is used by EMQX Public Broker: broker.emqx.io
static const char ca_cert[]
PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDtzCCAp+gAwIBAgIUIP0L/VPeu+7RXq0RVl3fkCCfsCkwDQYJKoZIhvcNAQEL
BQAwazELMAkGA1UEBhMCU0UxEjAQBgNVBAgMCVN0b2NraG9sbTESMBAGA1UEBwwJ
U3RvY2tob2xtMRAwDgYDVQQKDAdoaW1pbmRzMQswCQYDVQQLDAJDQTEVMBMGA1UE
AwwMMTkyLjE2OC4xLjE2MB4XDTI0MDYyNDA4NTIzN1oXDTI1MDYyNDA4NTIzN1ow
azELMAkGA1UEBhMCU0UxEjAQBgNVBAgMCVN0b2NraG9sbTESMBAGA1UEBwwJU3Rv
Y2tob2xtMRAwDgYDVQQKDAdoaW1pbmRzMQswCQYDVQQLDAJDQTEVMBMGA1UEAwwM
MTkyLjE2OC4xLjE2MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwKUM
1GOcTkk1M5xCuh4IWZ8cf2M5wTH5yaBNX5QUcUDQXlHSe3TuMMw9xP1tSTMQJfCe
NbEzCIBnUeYzItez7dVsQa2Csn0sQB/7cyGfp8iSiVXR5xs8i3KTVApu2heV5L+N
62Pn/8kCCb+UhzqZlTejrlol4K358yR0MLo1+qUGiqX69HzP29Z7Sp4YBPCxTn78
RFFhdoTsZMGT5HgzxtmEOnBtytvjusli6mvEPqRWn1ugUz5hsE73oyyI202m/wqF
Dxm0Gjc6QusD0ZVPl9etSJiPpyDCwiAJPqacYKopyJy70zmlhF1joO1y8lDcrISg
A4ZD8AutSOpX30RdwwIDAQABo1MwUTAdBgNVHQ4EFgQUgVrUj4nSs+Z3TTg4P9cq
5+UgDLIwHwYDVR0jBBgwFoAUgVrUj4nSs+Z3TTg4P9cq5+UgDLIwDwYDVR0TAQH/
BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAMbATV9YyaOcF5Lud7Z71Td59U4xV
Aqts4e6XDx4Gh7OuAxvu0HXdj1wfBBer1Ba0fIid67dEPFLp/rXJOe1RaZA8e7e1
Qgzli4V3H/y7ObNYBvcrrG2vhMN8hAUM7WmxiLfOoeOD0ppx/uAlO1z+T/Sf8ZBl
GwxaPpczir5dYRr5Xjto9qBGDh1t1urywCTljigXW5Nf5JzcwLNX3YTDbWyNhaAC
Jz2dMAZ4cO2Zjy5IYjt2d34BC1osOovD6r78+N9sy7FMln9TBOlkAepEIGYTPWnw
5B5VEl57bMXn//R1e8xnzu8IjydbtK6RxMQ6d3H4e8gVXlLEDrRi3XKQzQ==
-----END CERTIFICATE-----
)EOF";

// Load DigiCert Global Root CA ca_cert, which is used by EMQX Cloud Serverless Deployment
/*
static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";
*/


// Function declarations

void connectToMQTT();

void syncTime();

void mqttCallback(char *topic, byte *payload, unsigned int length);




/////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
    // Serial.begin(115200);
  // initialize serial communication at 9600 bits per second:
    pinMode(S0,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin SO */        
    pinMode(S1,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin S1 */  
    pinMode(S2,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin S2 */ 
    pinMode(S3,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin S3 */  
    pinMode(SIG, INPUT);  
    Serial.begin(9600);
    Serial.println(F("DHTxx test!"));
    dht.begin();
    WiFiManager wm;
    bool res;
    res = wm.autoConnect("AutoConnectAP","password"); 
    if(!res) {
      Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
        syncTime();  // X.509 validation requires synchronization time
        mqtt_client.setServer(mqtt_broker, mqtt_port);
        mqtt_client.setCallback(mqttCallback);
        connectToMQTT();
    }
}


void syncTime() {
    configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
    Serial.print("Waiting for NTP time sync: ");
    while (time(nullptr) < 8 * 3600 * 2) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Time synchronized");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.print("Current time: ");
        Serial.println(asctime(&timeinfo));
    } else {
        Serial.println("Failed to obtain local time");
    }
}

void connectToMQTT() {
    BearSSL::X509List serverTrustedCA(ca_cert);
    espClient.setTrustAnchors(&serverTrustedCA);
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str())) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            // Publish message upon successful connection
            mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^");
        } else {
            char err_buf[128];
            espClient.getLastSSLError(err_buf, sizeof(err_buf));
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.println(mqtt_client.state());
            Serial.print("SSL error: ");
            Serial.println(err_buf);
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
    StaticJsonDocument<200> doc;

  if (!mqtt_client.connected()) {
    connectToMQTT();
  }
     mqtt_client.loop();
  // reads the input on analog pin (value between 0 and 4095)
  // int analogValue = analogRead(LIGHT_SENSOR_PIN);

  // Serial.print("Analog Value Photoresistor = ");
  // Serial.print(analogValue);   // the raw analog reading
      digitalWrite(S0,LOW); digitalWrite(S1,LOW); digitalWrite(S2,LOW); digitalWrite(S3,LOW);
    photo = analogRead(SIG);

    digitalWrite(S0,HIGH); digitalWrite(S1,LOW); digitalWrite(S2,LOW); digitalWrite(S3,LOW);
    wind = analogRead(SIG);

    doc["photo"] = photo;
    
    // doc["value"] = 42;
	// Serial.print("Digital Output RainDrop: ");
	
	int val = digitalRead(sensorPin);	// Read the sensor output
  doc["rain"] = val;
	// Serial.println(val);
  // We'll have a few threshholds, qualitatively determined
  // if (analogValue < 40) {
  //   Serial.println(" => Dark");
  // } else if (analogValue < 800) {
  //   Serial.println(" => Dim");
  // } else if (analogValue < 2000) {
  //   Serial.println(" => Light");
  // } else if (analogValue < 3200) {
  //   Serial.println(" => Bright");
  // } else {
  //   Serial.println(" => Very bright");
  // }

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  // Serial.print(F("Humidity: "));
  doc["humidity"] = h;
  // Serial.print(h);
  // Serial.print(F("%  Temperature: "));
  doc["temperature"] = t;
  // Serial.print(t);
  // Serial.print(F("°C "));
  // Serial.print(f);
  // Serial.print(F("°F  Heat index: "));
  // Serial.print(hic);
   doc["heatIndex"] = hic;
  // Serial.print(F("°C "));
  // Serial.print(hif);
  // Serial.println(F("°F"));
   doc["windSpeed"] = wind;

    // char payload[80];  
    // snprintf(payload, sizeof(payload), "%d-------%.2f", 125,12.3 );
    // mqtt_client.publish(mqtt_topic, String(Signal) + "-------" + String(a.acceleration.y));


    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    // Serial.println(payload);
    mqtt_client.publish(mqtt_topic, buffer, n);
    delay(10);    

  delay(500);
}