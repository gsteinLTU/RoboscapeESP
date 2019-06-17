// Partially based off of public domain UDPSendReceive example by Michael Margolis

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <WiFiUdp.h>

// Define Wi-Fi connection settings
#ifndef STASSID
#define STASSID "vummiv"
#define STAPSK  ""
#endif

unsigned int localPort = 8888;      // local port to listen on

#ifdef ARDUINO_ARCH_ESP32
#define UDP_TX_PACKET_MAX_SIZE 128
#endif

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

WiFiUDP Udp;

// MAC address for identification
byte mac[6];

// NetsBlox/RoboScape Server
IPAddress serverIP(52, 73, 65, 98 );
unsigned int serverPort = 1973;

// Sends a RoboScape formatted message
void roboscape_send(const char * msg, int len)
{
  unsigned long time = millis();
  Udp.beginPacket(serverIP, serverPort);
  Udp.write(mac, 6);
  Udp.write((byte *) &time, 4);
  Udp.write((byte *)msg, len);
  Udp.endPacket();
}

// Motor pins
#ifdef ARDUINO_ARCH_ESP32
const int m1a = 33;
const int m1b = 25;
const int m2a = 18;
const int m2b = 5;
#else
const int m1a = 4;
const int m1b = 5;
const int m2a = 14;
const int m2b = 12;
#endif

#ifdef ARDUINO_ARCH_ESP32
#define NUM_CHANNELS 4
const int channels[NUM_CHANNELS] = { m1a, m1b, m2a, m2b };

void analogWrite(const uint8_t pin, const int amount){
  uint8_t channel = NUM_CHANNELS + 1;

  for(int i = 0; i < NUM_CHANNELS; i++){
    if(channels[i] == pin){
      channel = i;
    }
  }

  // Only valid channels
  if(channel > NUM_CHANNELS){
    return;
  }

  ledcWrite(channel, amount);
}

#endif

void connect() {
  if(STAPSK[0] != '\0'){
    Serial.println("Connecting with password");
    WiFi.begin(STASSID, STAPSK);
  } else {
    Serial.println("Connecting without password");
    WiFi.begin(STASSID);
  }
}


int sign(int n){
  if(n == 0)
    return 0;
  return n / abs(n);
};

void setup() {
  
  // Setup LED pin
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Setup motor pins
  pinMode(m1a, OUTPUT);
  pinMode(m1b, OUTPUT);
  pinMode(m2a, OUTPUT);
  pinMode(m2b, OUTPUT);

  // Start with motors off
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, LOW);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, LOW);
  
  Serial.begin(115200);
  
#ifndef ARDUINO_ARCH_ESP32
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
#endif

  Serial.println();
  delay(1000);
  
  // Connect to Wi-Fi
  connect();
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(WiFi.status());
    Serial.print(WiFi.RSSI());
    Serial.println();
    delay(500);
  }
  
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("UDP server on port %d\n", localPort);
  Udp.begin(localPort);

  // Get MAC address for RoboScape ID
  WiFi.macAddress(mac);

  // Send heartbeat
  roboscape_send("I", 1);

  // Setup PWM for ESP32
#ifdef ARDUINO_ARCH_ESP32
  for(int i = 0; i < NUM_CHANNELS; i++){
    ledcAttachPin(channels[i], i);
    ledcSetup(i, 5000, 8); 
  }
#endif

}

void loop() {
  
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i = 0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");

    for(int i = 0; i < packetSize; i++){
      Serial.print((unsigned byte)packetBuffer[i]);
      Serial.print(' ');
    }
    Serial.println();
    Serial.println(packetBuffer);

    // LED command
    if(packetBuffer[0] == 'L' && packetSize >= 3)
    {    
      Serial.println("set led command");
      if(packetBuffer[2] == 1){
        digitalWrite(LED_BUILTIN, LOW);
      } else {
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }

    // Set speed
    if(packetBuffer[0] == 'S' && packetSize >= 5)
    {
      Serial.println("set speed command");
      
      int left = *(short*)(packetBuffer + 1) * 4;
      left += 25 * sign(left);
      int right = *(short*)(packetBuffer + 3) * 4;
      right += 25 * sign(right);
      
      // Only set pin for direction we wish to travel
      if(left >= 0){
        analogWrite(m1a, left);
        analogWrite(m1b, 0);
      } else {
        analogWrite(m1a, 0);
        analogWrite(m1b, abs(left));
      }

      // Only set pin for direction we wish to travel
      if(right >= 0){
        analogWrite(m2a, right);
        analogWrite(m2b, 0);
      } else {
        analogWrite(m2a, 0);
        analogWrite(m2b, abs(right));
      }
    }

    // send a reply
    roboscape_send(packetBuffer, packetSize);
    
  }

  // Attempt to reconnect
  if(WiFi.status() != WL_CONNECTED){
    
    analogWrite(m1a, 0);
    analogWrite(m1b, 0);
    analogWrite(m2a, 0);
    analogWrite(m2b, 0);
    WiFi.disconnect();
    connect();
    
    // Wait a while before trying again
    if(WiFi.status() != WL_CONNECTED){
      delay(500);
    }
  }
  
  delay(10);
}
