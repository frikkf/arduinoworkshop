// Include SoftwareSerial and Wifi library
#include <SoftwareSerial.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include <WiFiEspClient.h>
#include <WiFiEspServer.h>
#include <ArduinoJson.h>

/*
 * global variables
 */

// Create WiFi module object on GPIO pin 2 (RX) and 3 (TX)
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

// Initialize the Wifi client object
WiFiEspClient wifiClient;

// Initialize the Wifi server object
WiFiEspServer wifiServer(80);
char server[] = "api.thingspeak.com";

/*
 * booting controll mechanism
 */

void setup() {
  initSerials();
  connectToWifi();
  Serial.println("You're connected to the network");
  printWifiStatus();
  initWifiServer();
  Serial.println("Startup completed");
}

void loop() {
  logESPOutput();
  postToServerInterval();
  listenToWifiClients();
}

/*
 * Below comes all the implementation
 */

void listenToWifiClients() {
  // listen for incoming clients. Give them simple response
  WiFiEspClient client = server.available();
  if(client) {
    Serial.println("New Client connected to server");
    //an http request ends with a blank line
    boolean currentLineIsBlank = true;

    while( client.connected() ) {
      if( client.available() ) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if( c == '\n' && currentLineIsBlank ) {
          Serial.println("Sending response");
          sendHttpResponse(client);
          break;
        }
        if( c == '\n') {
          // you are starting a new line
          currentLineIsBlank = true;
        }
        else if( c != '\r') {
          // you have gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(10);

    //close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}

void sendHttpResponse(WiFiEspClient client)
{
  //todo how to configure this to respond with json?
  //send a standard response http header
  // use \r\n instaed of many println statemenets to speedup data send
  client.print(
    "Http/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "Refresh: 20\r\n"
    "\r\n"
  );
  client.print("<!DOCTYPE HTML>\r\n");
  client.print("<html>\r\n");
  client.print("<h1>Hello Arduino Friends!</h1>\r\n");
  client.print("<br>\r\n");
  client.print("</html>\r\n");
}

void sendHttpResponseJSON(WiFiEspClient client)
{
  //todo how to configure this to respond with json?
  //send a standard response http header
  // use \r\n instaed of many println statemenets to speedup data send
  client.print(
    "Http/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Connection: close\r\n"
    "Refresh: 20\r\n"
    "\r\n"
  );
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& object = jsonBuffer.createObject();
  object["hello"] = "world";
  object.prettyPrintTo(client);
  char json[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";
  client.println(json);
  
}

void initWifiServer() {
  // start the web server on port 80
  wifiServer.begin();
}

void logESPOutput() {
// Get connection info in Serial monitor
  while(wifiClient.available()){
    char c = wifiClient.read();
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
  if (wifiClient.connectSSL(server, sslPort)) {
    Serial.println("Connected to server");
    wifiClient.println("GET /update?api_key=" + String(thingspeakAPIKey) + 
    "&field1=" + String(value) + " HTTP/1.1");
    wifiClient.println("Host: api.thingspeak.com");
    wifiClient.println("Connection: close");
    wifiClient.println();
    Serial.println("Sent to server");
  }  
}

void uploadToThingSpeak(long value) {
  GET("api.thingspeak.com", "/update?api_key=" + String(thingspeakAPIKey) + "&field1=" + String(value), null)
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

void GET(char domain[], char relativeUrl[], headers ) {
  if (wifiClient.connectSSL(domain, 443)) {
    Serial.println("Connected to endpoint");
    string connect = "GET " + String(relativeUrl) + " HTTP/1.1";
    wifiClient.println(connect)
    wifiClient.println("Host: "+ String(domain))
    wifiClient.println("Connection: close");
    Serial.println("Sent to server");
  }
}