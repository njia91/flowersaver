#include <WiFi101.h>
#include <WiFiSSLClient.h>
#include "ArduinoLowPower.h"
#include <string.h>
#include "secrets.h"

#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>

// The name of the device. This MUST match up with the name defined in the AWS console
#define DEVICE_NAME "FlowerSaver101"

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

int moisturePin = A2; 
// Set this threeshold accordingly to the resistance you used 
// The easiest way to calibrate this value is to test the sensor in both dry and wet soil 
int threeshold = 1000; 
unsigned long lastMillis = 0;

const unsigned long SECOND = 1000;
const unsigned long HOUR = 3600*SECOND;

const int AirValue = 890;   //you need to replace this value with Value_1
const int WaterValue = 660;  //you need to replace this value with Value_2
int intervals = (AirValue - WaterValue)/3;
int moistureLevel = 0;

#define SensorPin A0

boolean isEmailSent = false;
WiFiSSLClient client; 

void setup() { 
  Serial.begin(9600); 
  delay(2000); 
  setupWifiConnection();

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, AWS_CERTIFICATE);

  // Optional, set the client id used for MQTT,
  // each device that is connected to the 

  // must have a unique client id. The MQTTClient will generate
  // a client id for you based on the millis() value if not set
  //
  mqttClient.setId(AWS_THING_ARN);

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);

} 
bool isConnectedOnce = false;
void loop() { 

  
  if (!mqttClient.connected()) {
    //MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  //mqttClient.poll();

  moistureLevel = analogRead(A0);
  Serial.println(moistureLevel);
  if (isDry(moistureLevel)){
    Serial.println("I AM DRY!!");
  }
 
  //Serial.print("Publishing Moisture level to MQTT AWS IOT: ");
  Serial.println(moistureLevel);
  lastMillis = millis();
  publishMessage();

  mqttClient.stop();
  //delay(2000);
  delay(HOUR);
} 

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.println(" ");

  while (!mqttClient.connect(AWS_IOT_ENDPOINT, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(10000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  //mqttClient.subscribe("arduino/incoming");
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing");
  String moistureLevelStr = String(moistureLevel);
  String jsonFormattedMessage= "{\"id\":\"Kribb-Pink-Flower\",\"moistureLevel\":" + moistureLevelStr + "}";
  mqttClient.print(jsonFormattedMessage.c_str());
  mqttClient.endMessage();
}

void turnOnLed(){
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level) 
}

void turnOffLed(){
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level) 
}

bool setupWifiConnection(){
  Serial.print("Connecting Wifi: "); 
  Serial.println(ssid); 
  while (WiFi.begin(ssid, password) != WL_CONNECTED) { 
    Serial.print("."); 
    delay(500); 
  } 
  Serial.println(""); 
  Serial.println("WiFi connected"); 
  return true;
}

void disconnectWifi(){
  WiFi.end();
  Serial.println("Wifi Disconnected");
}



unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
}

boolean isDry(int moistureValue){
  return ( moistureLevel > (AirValue - intervals));
}
