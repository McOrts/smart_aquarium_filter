#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <PCF8574.h>

#include "settings.h"

// Define custom I2C pins for communication
#define SDA_PIN 4    // Set SDA pin for I2C communication
#define SCL_PIN 15   // Set SCL pin for I2C communication

// Communication variables
WiFiClient espClient;
PubSubClient clientMqtt(espClient);
String mqttcommand = String(14);
long lastMsg = 0;
char msg[50];
int value = 0;

// Define I2C addresses for the inPCF8574 pcf8574_output(0x24); // I2C address for the output expander (change according to your setup)
PCF8574 pcf8574_input(0x22);  // I2C address for the input expander (change according to your setup)
PCF8574 pcf8574_output(0x24); // I2C address for the output expander (change according to your setup)

// TEMPERATURE SENSOR DS18B20
// KC868-A6 SENS connector:
// HT1 = GPIO32
// HT2 = GPIO33
#define ONE_WIRE_BUS 32
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS18B20(&oneWire);
char temperatureCString[7];

// Servo feeder
Servo ServoFeeder;
const int FEEDER_HOME_ANGLE = 10;
const int FEEDER_FEED_ANGLE = 160;

void getTemperature() {
  float tempC;
//  do {
    DS18B20.requestTemperatures(); 
    tempC = DS18B20.getTempCByIndex(0);
    dtostrf(tempC, 2, 2, temperatureCString);
    delay(100);
//  }
}

void moveFeederServo(int targetAngle) {
  // Attach only while the servo needs to move.
  // Safer pulse-width range than 500–2500 us.
  ServoFeeder.attach(PinFeeder, 1000, 2000);
  ServoFeeder.write(targetAngle);
  // Give the servo enough time to reach the position.
  delay(700);
  // Stop PWM pulses so the servo does not continuously hold torque,
  // overheat, or consume power after the movement.
  ServoFeeder.detach();
}

void setup() {
  Serial.begin(115200);

  // Wifi
  setup_wifi();
  clientMqtt.setServer(mqtt_server, mqtt_port);
  clientMqtt.setCallback(callback);

  // Initialize I2C and set custom SDA and SCL pins
  Wire.begin(SDA_PIN, SCL_PIN);
  // Ensure all relay outputs are set to HIGH (deactivated) before initializing the expander
  // This step is to avoid any relay from briefly turning on during setup
  for (int i = 0; i < 6; i++) {
    pcf8574_output.digitalWrite(i, HIGH);
  }
  // Initialize input pins as INPUT
  pcf8574_input.pinMode(P0, INPUT);
  pcf8574_input.pinMode(P1, INPUT);
  pcf8574_input.pinMode(P2, INPUT);
  pcf8574_input.pinMode(P3, INPUT);
  pcf8574_input.pinMode(P4, INPUT);
  pcf8574_input.pinMode(P5, INPUT);
  // Initialize output pins as OUTPUT (these control the relays)
  pcf8574_output.pinMode(P0, OUTPUT);
  pcf8574_output.pinMode(P1, OUTPUT);
  pcf8574_output.pinMode(P2, OUTPUT);
  pcf8574_output.pinMode(P3, OUTPUT);
  pcf8574_output.pinMode(P4, OUTPUT);
  pcf8574_output.pinMode(P5, OUTPUT);
  // Ensure all outputs (relays) are deactivated at the start (set to HIGH)
  for (int i = 0; i < 6; i++) {
    pcf8574_output.digitalWrite(i, HIGH);
  }
  // Initialize the PCF8574 modules and check for errors
  Serial.print("Init pcf8574_input...");
  if (pcf8574_input.begin()) {
    Serial.println("OK");  // Input expander initialized successfully
  } else {
    Serial.println("KO");  // Input expander initialization failed
  }
  Serial.print("Init pcf8574_output...");
  if (pcf8574_output.begin()) {
    Serial.println("OK");  // Output expander initialized successfully
  } else {
    Serial.println("KO");  // Output expander initialization failed
  }

  // Feeder servo configuration
  ServoFeeder.setPeriodHertz(50);
  const int FEEDER_HOME_ANGLE = 10;
  const int FEEDER_FEED_ANGLE = 160;
  Serial.println("KC868-A6 feeder servo ready.");

  // Temperature sensor
  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  getTemperature();
  delay(2000);
  Serial.print("Publish start-up temperature: ");
  Serial.println(temperatureCString);
  clientMqtt.publish(mqtt_pub_topic_temperature, temperatureCString);
}

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

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT]Incomming message (");
  Serial.print(topic);
  Serial.print(") ");
  mqttcommand = "";
  for (int i = 0; i < length; i++) {
    mqttcommand += (char)payload[i];
  }
  Serial.print(mqttcommand);
  Serial.println(mqttcommand.substring (0,6));
  Serial.println(atoi(mqttcommand.substring (6,7).c_str()));
 
  // MQTT Command control  
  if (mqttcommand == "FilterOn") {
    pcf8574_output.digitalWrite(0, LOW);
    Serial.println("Filter On");
    clientMqtt.publish(mqtt_sub_topic_healthcheck, "ACK Filter On");
  } else if (mqttcommand == "FilterOff"){
    pcf8574_output.digitalWrite(0, HIGH);
    Serial.println("Filter Off");
    clientMqtt.publish(mqtt_sub_topic_healthcheck, "ACK Filter Off");
  } else if (mqttcommand == "CleanerOn"){
    pcf8574_output.digitalWrite(1, LOW);
    Serial.println("Cleaner On");
    clientMqtt.publish(mqtt_sub_topic_healthcheck, "ACK Cleaner On");
  } else if (mqttcommand == "CleanerOff"){
    pcf8574_output.digitalWrite(1, HIGH);
    Serial.println("Cleaner Off");
    clientMqtt.publish(mqtt_sub_topic_healthcheck, "ACK Cleaner Off");
} else if (mqttcommand.substring(0, 6) == "feeder") {
    int turns = atoi(mqttcommand.substring(6).c_str());
    // Protect against invalid commands or excessive repeated movements.
    if (turns < 1 || turns > 20) {
      Serial.println("Invalid feeder command.");
      clientMqtt.publish(
        mqtt_sub_topic_healthcheck,
        "ERROR: feeder command must be between 1 and 20"
      );
      return;
    }
    for (int i = 0; i < turns; i++) {
      Serial.print("Feeder cycle ");
      Serial.print(i + 1);
      Serial.print(" of ");
      Serial.println(turns);
      // Move to the dispensing position.
      moveFeederServo(FEEDER_FEED_ANGLE);
      delay(300);
      // Return to the home position.
      moveFeederServo(FEEDER_HOME_ANGLE);
      delay(300);
    }
    clientMqtt.publish(mqtt_sub_topic_healthcheck, "ACK Fed");
  }
}

void reconnect() {
  Serial.print("[MQTT] Starting connection MQTT... ");
  // Loop until we're reconnected
  while (!clientMqtt.connected()) {
    Serial.print(".");
    // Attempt to connect
    if (clientMqtt.connect(mqtt_id)) { // Ojo, para más de un dispositivo cambiar el nombre para evitar conflicto
      Serial.println("");
      Serial.println("[MQTT] connecting server...");
      // Once connected, publish an announcement...
      clientMqtt.publish(mqtt_sub_topic_healthcheck, "starting");
      // ... and subscribe
      clientMqtt.subscribe(mqtt_sub_topic_operation);
      Serial.println("[MQTT] connected !");
    } else {
      Serial.print("[MQTT]Error, rc=");
      Serial.print(clientMqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!clientMqtt.connected()) {
    reconnect();
  }
  clientMqtt.loop();
  
  long now = millis();
  if (now - lastMsg > update_time) {
    lastMsg = now;
    
    // Water temperature sensor
    getTemperature();
    Serial.print("Publish temperature: ");
    Serial.println(temperatureCString);
    clientMqtt.publish(mqtt_pub_topic_temperature, temperatureCString);
    
    // Water level sensor
    if (pcf8574_input.digitalRead(PinLevelSensor) == LOW) {
      Serial.println("Water lever Ok");
      clientMqtt.publish(mqtt_sub_topic_waterlevel, "ok");
    } else {
      Serial.println("Water lever LOW");
      clientMqtt.publish(mqtt_sub_topic_waterlevel, "low");
    }

  }
}
