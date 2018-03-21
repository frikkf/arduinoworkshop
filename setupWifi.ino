// Include SoftwareSerial and Wifi library
#include <SoftwareSerial.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include <WiFiEspClient.h>
#include <WiFiEspServer.h>
#include <ArduinoJson.h>

// Create WiFi module object on GPIO pin 6 (RX) and 7 (TX)
SoftwareSerial Serial1(2, 3);

// Declare and initialise global arrays for WiFi settings
/*char ssid[] = "Frikk-4G";
char pass[] = "FjiA2018";*/
char ssid[] = "Herman-3G";
char pass[] = "hermansinvilla";
long sslPort = 443;
const char thingspeakAPIKey[] = "Z8XM3T4MTC2GUXOJ";
long postingInterval = 30000;

// Declare global variable for timing
long lastConnectionTime;

// Declare and initialise data variable
long myData = 0;

// Declare and initialise variable for radio status 
int status = WL_IDLE_STATUS;

// Initialize the Ethernet client object
WiFiEspClient client;

char server[] = "api.thingspeak.com";

//booting up
void setup() {
  
  initSerials();
  
  connectToWifi();

  Serial.println("You're connected to the network");
  printWifiStatus();
  Serial.println("Startup completed");
}

void loop() {
  
  logESPOutput();
  postToServerInterval();
   
}

void logESPOutput() {
// Get connection info in Serial monitor
  while(client.available()){
    char c = client.read();
    Serial.write(c);
  }
}

void initSerials() {
  // Initialize serial for debugging
  Serial.begin(115200);
  
  // Initialize serial for ESP module
  Serial1.begin(9600);
  
  // Initialize ESP module
  WiFi.init(&Serial1);
}

void connectToWifi() {
  // Attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
}

void postToServerInterval() {
  if (millis() - lastConnectionTime > postingInterval) {
    myData = myData + random(-10, 10);
    sendThingspeak(myData);
    lastConnectionTime = millis();
  }
}

void sendThingspeak(long value){
  if (client.connectSSL(server, sslPort)) {
    Serial.println("Connected to server");
    client.println("GET /update?api_key=" + String(thingspeakAPIKey) + 
    "&field1=" + String(value) + " HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println();
    Serial.println("Sent to server");
  }  
}

void printWifiStatus() {
  
   // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}