#include <LinkedList.h>

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
long lastLightSensorLogTime;
long lastTempSensorLogTime;
// Declare and initialise data variable
long myData = 0;

// Declare and initialise variable for radio status 
int status = WL_IDLE_STATUS;

// Initialize the Wifi client object
WiFiEspClient wifiClient;

// Initialize the Wifi server object
WiFiEspServer wifiServer(80);
char server[] = "api.thingspeak.com";

boolean reading = false;
String myStr;
const int redPin = 5;
const int greenPin = 6;
const int bluePin = 7;
const int lightPin = A0;
const int tempPin = A1;
int redVal = 250, greenVal = 100, blueVal = 50;
// Instantiate a LinkedList that will hold 'integer'
LinkedList<String> responseArray = LinkedList<String>();
LinkedList<String> parseArray = LinkedList<String>();
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
  setLedColor();
}

void loop() {
  //logESPOutput();
  //postToServerInterval();
  listenToWifiClients();

  logLightSensorData();
  logTempSensorData();
}

void logLightSensorData() {
  if (millis() - lastLightSensorLogTime > 30000) {
    int sensorData = getLightSensorData();
    Serial.print("Raw light sensor value: ");
    Serial.print(sensorData);
    Serial.print("\n");
    //sendThingspeak(String(sensorData), "field2");
    lastLightSensorLogTime = millis();
  }
}

int getLightSensorData() {
  return analogRead(lightPin);
}

void logTempSensorData() {
  if (millis() - lastTempSensorLogTime > 30000) {
    int sensorData = analogRead(tempPin);
    Serial.print("Raw temp sensor value: ");
    Serial.print(sensorData);
    Serial.print("\t voltage: ");
    float voltage = (sensorData/1024.0) * 5.0;
    Serial.print(voltage);
    Serial.print("\t degrees C: ");
    float temperature = (voltage - .5) * 100;
    Serial.print(temperature);
    Serial.print("\n");
    sendThingspeak(String(temperature));
    lastTempSensorLogTime = millis();
  }
}

float getTempSensorTemperatur() {
  int sensorData = analogRead(tempPin);
  float voltage = (sensorData/1024.0) * 5.0;
  float temperature = (voltage - .5) * 100;
  return temperature;
}

void setLedColor() {
  Serial.println("r: "+String(redVal)+ " g:"+String(greenVal)+" b:"+String(blueVal));
  analogWrite(redPin, redVal);
  analogWrite(greenPin, greenVal);
  analogWrite(bluePin, blueVal);
}

/*
 * Below comes all the implementation
 */

void requestHandler() {
  checkThingSpeakQuery();
}

void checkThingSpeakQuery() {
  for (int i=0; i <= parseArray.size() - 2; i = i + 2){
      if(parseArray.get(i).indexOf("thingspeak") != -1) {
        responseArray.add("SentValue");
        String value = parseArray.get(i+1);
        responseArray.add(value);
        sendThingspeak(value);
      }
  }
}

void parseQueryParams(String str) {//sets the parseArray to the queryParams
  parseArray.clear();
  String currentStr = str;
  while( currentStr.indexOf("&") != -1) {
    int endIndex = currentStr.indexOf("&");
    String param = currentStr.substring(0, endIndex);
    String key = param.substring(0, param.indexOf("="));
    parseArray.add(key);
    String value = param.substring(param.indexOf("=")+1, param.length());
    parseArray.add(value);
    currentStr = currentStr.substring(endIndex+1, currentStr.length());
  }
  //last piece
  String lastParam = currentStr;
  String key = currentStr.substring(0, currentStr.indexOf("="));
  String value = currentStr.substring(currentStr.indexOf("=")+1, currentStr.length());
  parseArray.add(key);
  parseArray.add(value);
}

String getUrlQueryParamsAsString(WiFiEspClient client) {
  boolean currentLineIsBlank = true;
  String str = "";
  while( client.connected() ) {
    if( client.available() ) {
      char c = client.read();
      if(reading && c == ' ') reading = false; //stop reading
      if(c == '?') reading = true; //found the ?, begin reading the queryParams
      if(reading && c != '?') str += c; //append the next character to str
      if (c == '\n' && currentLineIsBlank)  break;//stop reading 
      if (c == '\n') {
        currentLineIsBlank = true;
      }else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }//end if
  }
  return str;
}

void listenToWifiClients() {
  // listen for incoming clients. Give them simple response
  WiFiEspClient client = wifiServer.available();
  if(client) {
    Serial.println("\n\n\n");
    Serial.println("New Client connected to server");
    String str = getUrlQueryParamsAsString(client);
    parseQueryParams(str);
    requestHandler();
    sendHttpResponseJSON(client);

    // give the web browser time to receive the data
    delay(10);

    //close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}

void sendHttpResponseHTML(WiFiEspClient client) {
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

void sendHttpResponseJSON(WiFiEspClient client){
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
  for (int i=0; i <= responseArray.size() - 2; i = i + 2){
      object[responseArray.get(i)] = responseArray.get(i+1);
  }
  object.prettyPrintTo(client);
  responseArray.clear();
}

void initWifiServer() {
  // start the web server on port 80
  wifiServer.begin();
}
/*
void logESPOutput() {
  // Get connection info in Serial monitor
  while(wifiClient.available()){
    char c = wifiClient.read();
    
    // Check HTTP status
    char status[32] = {0};
    wifiClient.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      return;
    }

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!wifiClient.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
    }

    // Allocate JsonBuffer
    // Use arduinojson.org/assistant to compute the capacity.
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonBuffer jsonBuffer(capacity);

    // Parse JSON object
    JsonObject& root = jsonBuffer.parseObject(wifiClient);
    if (!root.success()) {
      Serial.println(F("Parsing failed!"));
      return;
    }
    Serial.write(c);
  }
}*/

void initSerials() {
  // Initialize serial for debugging
  Serial.begin(115200);
  
  // Initialize serial for ESP module
  Serial1.begin(9600);
  
  // Initialize ESP module
  WiFi.init(&Serial1);

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
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
    sendThingspeak(String(myData));
    lastConnectionTime = millis();
  }
}

void sendThingspeak(String value){
  if (wifiClient.connectSSL(server, sslPort)) {
    Serial.println("Connected to server");
    wifiClient.println("GET /update?api_key=" + String(thingspeakAPIKey) + 
    "&field1=" + value + " HTTP/1.1");
    wifiClient.println("Host: api.thingspeak.com");
    wifiClient.println("Connection: close");
    wifiClient.println();
    Serial.println("Sent to server");
  }  
}


void uploadToThingSpeak(String value, String fieldName) {
  String relativeUrl = "/update?api_key=" + String(thingspeakAPIKey) + "&"+fieldName+"=" + value;
  GET("api.thingspeak.com", relativeUrl);
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

void GET(char domain[], String relativeUrl) {
  if (wifiClient.connectSSL(domain, 443)) {
    Serial.println("Connected to endpoint");
    String connect = "GET " + relativeUrl + " HTTP/1.1";
    wifiClient.println(connect);
    wifiClient.println("Host: "+ String(domain));
    wifiClient.println("Connection: close");
    Serial.println("Sent to server");
  }
}