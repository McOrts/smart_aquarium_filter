/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WIFI values
const char* ssid = "XXXXXX";
const char* password = "XXXXXXX";
WiFiClient espClient;
PubSubClient client(espClient);

// CONTROL parameters
const int PinReleFilter = 2;
const int PinReleCooler = 0;

// MQTT configuration
const char* mqtt_server = "192.168.1.114";
String mqttcommand = String(14);

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
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  mqttcommand = "";
  for (int i = 0; i < length; i++) {
    mqttcommand += (char)payload[i];
  }
  Serial.print(mqttcommand);
  Serial.println();
 
  // Switch on the LED if an 1 was received as first character
  if (mqttcommand == "FilterOn") {
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
    digitalWrite(PinReleFilter, LOW);
    Serial.println("FilterOn");
  } else if (mqttcommand == "FilterOff"){
    digitalWrite(PinReleFilter, HIGH);
    Serial.println("FilterOff");
  } else if (mqttcommand == "CoolerOn"){
    digitalWrite(PinReleCooler, LOW);
    Serial.println("coolerOn");
  } else if (mqttcommand == "CoolerOff"){
    digitalWrite(PinReleCooler, HIGH);
    Serial.println("CoolerOff");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/home/aquarium", "starting");
      // ... and resubscribe
      client.subscribe("/home/aquarium/operation");
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
  pinMode(PinReleCooler, OUTPUT);  
  digitalWrite(PinReleFilter, HIGH);
  digitalWrite(PinReleCooler, HIGH);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  getTemperature();
  Serial.println("Publish temperature: ");
  Serial.println(temperatureCString);
  client.publish("/home/aquarium/temperature", temperatureCString);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 3600000) {
    lastMsg = now;
    getTemperature();
    Serial.println("Publish temperature: ");
    Serial.println(temperatureCString);
    client.publish("/home/aquarium/temperature", temperatureCString);
  }
}
