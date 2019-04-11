#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "vummiv"
#define STAPSK  ""
#endif

unsigned int localPort = 8888;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged\r\n";       // a string to send back

WiFiUDP Udp;

// MAC address for identification
byte mac[6];

// NetsBlox Server
IPAddress serverIP(52, 73, 65, 98 );
unsigned int serverPort = 1973;

// Sends a RoboScape formatted message
void roboscape_send(const char * msg, int len)
{
  unsigned long time = millis();
  Udp.beginPacket(serverIP, serverPort);
  Udp.write(mac, 6);
  Udp.write((byte *) &time, 4);
  Udp.write(msg, len);
  Udp.endPacket();
}


// Motor pins
const int m1a = 4;
const int m1b = 5;
const int m2a = 14;
const int m2b = 12;

void setup() {
  // Setup LED pin
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Setup motor pins
  pinMode(m1a, OUTPUT);
  pinMode(m1b, OUTPUT);
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, LOW);
  pinMode(m2a, OUTPUT);
  pinMode(m2b, OUTPUT);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, LOW);
  
  Serial.begin(115200);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  
  Serial.println();
  delay(1000);
  
  if(STAPSK[0] != '\0'){
    Serial.println("Connecting with password");
    WiFi.begin(STASSID, STAPSK);
  } else {
    Serial.println("Connecting without password");
    WiFi.begin(STASSID);
  }
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

  WiFi.macAddress(mac);

  roboscape_send("I", 1);
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
      int right = *(short*)(packetBuffer + 3) * 4;


      if(left >= 0){
        analogWrite(m1a, left);
        analogWrite(m1b, 0);
      } else {
        analogWrite(m1a, 0);
        analogWrite(m1b, abs(left));
      }

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
  delay(1);
}
