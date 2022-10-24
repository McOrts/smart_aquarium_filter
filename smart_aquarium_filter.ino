/*
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h> 
#include "settings.h"

// WIFI values
WiFiClient espClient;
PubSubClient client(espClient);

// CONTROL parameters
const int PinReleTmpCtrl = 2;
const int PinFeeder = 14;
const int PinReleFilter = 12;
const int PinLevelSensor = 4;

/* NEOPIXEL configuration*/
#define PIN_STRIP_1 13
#define NUMPIXELS_STRIP_1 9
Adafruit_NeoPixel pixels_STRIP_1  = Adafruit_NeoPixel(NUMPIXELS_STRIP_1, PIN_STRIP_1, NEO_GRB + NEO_KHZ800);

// MQTT configuration
String mqttcommand = String(14);

// Create a servo object 
Servo ServoFeeder; 

// TEMPERATURE SENSOR DS18B20
// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5
#define ONE_WIRE_BUS 5
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS18B20(&oneWire);
char temperatureCString[7];
char temperatureFString[7];

// Other params
long lastMsg = 0;
char msg[50];
int value = 0;
int SensorState = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

   // We need to attach the servo to the used pin number 
  ServoFeeder.attach(PinFeeder); 
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  mqttcommand = "";
  for (int i = 0; i < length; i++) {
    mqttcommand += (char)payload[i];
  }
  Serial.println(mqttcommand);
  Serial.println(mqttcommand.substring (0,6));
  Serial.println(atoi(mqttcommand.substring (6,7).c_str()));
 
  // MQTT Command control  
  if (mqttcommand == "FilterOn") {
    digitalWrite(PinReleFilter, LOW);
    Serial.println("Filter On");
  } else if (mqttcommand == "FilterOff"){
    digitalWrite(PinReleFilter, HIGH);
    Serial.println("Filter Off");
  } else if (mqttcommand == "TmpCtrlOn"){
    digitalWrite(PinReleTmpCtrl, LOW);
    Serial.println("Heater On");
  } else if (mqttcommand == "TmpCtrlOff"){
    digitalWrite(PinReleTmpCtrl, HIGH);
    Serial.println("Heater Off");
  } else if (mqttcommand.substring (0,6) == "feeder"){
    for (int i=0; i<atoi(mqttcommand.substring (6,7).c_str()); i++) {
      Serial.println(i);
      Serial.println("Feeder turn");
      ServoFeeder.write(0);
      delay (1000);
      ServoFeeder.write(180);
      delay (1000);
    } 
  } else if (mqttcommand == "LightOn") {
      for(int i=0;i<NUMPIXELS_STRIP_1;i++){
        pixels_STRIP_1.setPixelColor(i, pixels_STRIP_1.Color(255,255,255)); // high bright white color.
        pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
      }
    Serial.println("Lights On");
  } else if (mqttcommand == "LightBlue") {
      for(int i=0;i<NUMPIXELS_STRIP_1;i++){
        pixels_STRIP_1.setPixelColor(i, pixels_STRIP_1.Color(0,0,255)); // high bright white color.
        pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
      }
    Serial.println("Lights On");
  } else if (mqttcommand == "LightOff") {
      for(int i=0;i<NUMPIXELS_STRIP_1;i++){
        pixels_STRIP_1.setPixelColor(i, pixels_STRIP_1.Color(0,0,0)); // black color.
        pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
      }
    Serial.println("Lights Off");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_id)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_sub_topic_healthcheck, "starting");
      // ... and resubscribe
      client.subscribe(mqtt_sub_topic_operation);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void getTemperature() {
  float tempC;
  float tempF;
//  do {
    DS18B20.requestTemperatures(); 
    tempC = DS18B20.getTempCByIndex(0);
    dtostrf(tempC, 2, 2, temperatureCString);
    tempF = DS18B20.getTempFByIndex(0);
    dtostrf(tempF, 3, 2, temperatureFString);
    delay(100);
//  } while (tempC == 85.0 || tempC == (-127.0));
}


void setup() {
  pinMode(PinReleFilter, OUTPUT);
  pinMode(PinReleTmpCtrl, OUTPUT);
  pinMode(PinLevelSensor, INPUT);
  digitalWrite(PinReleFilter, HIGH);
  digitalWrite(PinReleTmpCtrl , HIGH);
  digitalWrite(PinLevelSensor, LOW);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  getTemperature();
  Serial.println("Publish start-up temperature: ");
  Serial.println(temperatureCString);
  client.publish(mqtt_pub_topic_temperature, temperatureCString);

  /* NEOPIXEL Initialization*/
  pixels_STRIP_1.begin(); // This initializes the NeoPixel library. 
  for(int i=0;i<NUMPIXELS_STRIP_1;i++){
     pixels_STRIP_1.setPixelColor(i, pixels_STRIP_1.Color(0,0,0)); // black color.
     pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > update_time) {
    lastMsg = now;
    
    // Water temperature sensor
    getTemperature();
    Serial.println("Publish temperature: ");
    Serial.println(temperatureCString);
    client.publish(mqtt_pub_topic_temperature, temperatureCString);

    // Water level sensor
    SensorState = digitalRead(PinLevelSensor);
    if (SensorState == HIGH) {
      Serial.println("Water lever Ok");
      client.publish(mqtt_sub_topic_waterlevel, "ok");
    } else {
      Serial.println("Water lever LOW");
      client.publish(mqtt_sub_topic_waterlevel, "low");
    }
    
  }
}
