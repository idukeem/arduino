/*
 *  Json parametric GET REST response with ArduinoJSON library
  *  by Mischianti Renzo <https://www.mischianti.org>
 *
 *  https://www.mischianti.org/
 *
 */
 
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FS.h>

//NEOPixel:
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
 
const char* ssid = "Marcin-Privat-5G";
const char* password = "Krieg$str.37-5G";

//NeoPixel LED controll:
#define LED_PIN    5
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 17
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
 
ESP8266WebServer server(80);
File fsUploadFile;

class PinData {
  private:
    
  public:
    String functions[8] = {"off", ""};
    int pin;
    PinData(int pin) {
      this->pin = pin;
      init();
    }

    void init() {
    }
  
};

  PinData myPin(0);
  PinData pinData2(1);
  PinData myPinData[9] = {PinData(1), PinData(2), PinData(3), PinData(4), PinData(5), PinData(6), PinData(7), PinData(8), PinData(9)};

  void setupAPI() {
    PinData neoPin = myPinData[LED_PIN-1];
    neoPin.functions[0] = "red";
    neoPin.functions[1] = "green";
    myPinData[LED_PIN-1] = neoPin;
  }
  
void setCrossOrigin(){
    server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    server.sendHeader(F("Access-Control-Max-Age"), F("600"));
    server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

// API
void getPinData() {
  setCrossOrigin();
  if(!server.hasArg("pin")) {
    server.send(500, "text/plain", "BAD ARGS"); 
    return;
  }
 
  int pinNumber = server.arg("pin").toInt();
  PinData pinData = myPinData[pinNumber-1];
  DynamicJsonDocument doc(512);
  doc["pin"] = pinData.pin;
  JsonArray functions = doc.createNestedArray("functions");
  for (byte idx = 0; idx < sizeof(pinData.functions) / sizeof(pinData.functions[0]); idx++) {
     functions.add(pinData.functions[idx]);
  }
  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}

void changePinData() {
  setCrossOrigin();
  String postBody = server.arg("plain");
  Serial.println(postBody);
 
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);
  if (error) {
      // if the file didn't open, print an error:
      Serial.print(F("Error parsing JSON "));
      Serial.println(error.c_str());
      String msg = error.c_str();
      server.send(400, F("text/html"), "Error in parsin json body! <br>"+msg);
  }else{
      JsonObject postObj = doc.as<JsonObject>();
      int pinNumber = server.arg("pin").toInt();
      PinData pinData = myPinData[pinNumber-1];
      for (byte idx = 0; idx < sizeof(pinData.functions) / sizeof(pinData.functions[0]); idx++) {
        pinData.functions[idx] = "";
      }
      if ((postObj.containsKey("functions"))) {
        JsonArray functions = postObj["functions"];
        int i = 0;
        for(JsonVariant v : functions) {
          Serial.print("functions: ");
          Serial.println(v.as<String>());
          pinData.functions[i] = v.as<String>();
          i = i + 1;
        }
        myPinData[pinNumber-1] = pinData;
        reactOnPinData(pinData);
        server.send(201, "application/json", postBody);
      }
      else {
         server.send(204, F("text/html"), F("No data found, or incorrect!"));
      }
    }
}


void reactOnPinData(PinData pinData) {
  for (byte idx = 0; idx < sizeof(pinData.functions) / sizeof(pinData.functions[0]); idx++) {
     if (pinData.functions[idx] == "off") {
      digitalWrite(pinData.pin, HIGH);
     }
     if (pinData.functions[idx] == "on") {
      digitalWrite(pinData.pin, LOW);
     }
  }
}

void runNeoPixel() {
  PinData neoPin = myPinData[LED_PIN-1];
  for (byte idx = 0; idx < sizeof(neoPin.functions) / sizeof(neoPin.functions[0]); idx++) {
    if (neoPin.functions[idx] == "red") {
      colorWipe(strip.Color(255,   0,   0), 50); // Red
    }
    if (neoPin.functions[idx] == "green") {
      colorWipe(strip.Color(  0, 255,   0), 50); // Green
    }
    if (neoPin.functions[idx] == "blue") {
      colorWipe(strip.Color(  0,   0, 255), 50); // Blue
    }
    if (neoPin.functions[idx] == "cinemaWhite") {
      theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness
    }
    if (neoPin.functions[idx] == "cinemaRed") {
      theaterChase(strip.Color(127,   0,   0), 50); // Red, half brightness
    }
    if (neoPin.functions[idx] == "cinemaBlue") {
      theaterChase(strip.Color(  0,   0, 127), 50); // Blue, half brightness
    }
    if (neoPin.functions[idx] == "rainbow") {
      rainbow(10);             // Flowing rainbow cycle along the whole strip
    }
    if (neoPin.functions[idx] == "cinemaRainbow") {
      theaterChaseRainbow(50); // Rainbow-enhanced theaterChase variant
    }
  }
}

// Serving Hello world
void getHelloWord() {
      DynamicJsonDocument doc(512);
      doc["name"] = "Hello world";
 
      Serial.print(F("Stream..."));
      String buf;
      serializeJson(doc, buf);
      server.send(200, "application/json", buf);
      Serial.print(F("done."));
}
 
// Serving Hello world
void getSettings() {
    setCrossOrigin();
//
      // Allocate a temporary JsonDocument
      // Don't forget to change the capacity to match your requirements.
      // Use arduinojson.org/v6/assistant to compute the capacity.
    //  StaticJsonDocument<512> doc;
      // You can use DynamicJsonDocument as well
      DynamicJsonDocument doc(512);
 
      doc["ip"] = WiFi.localIP().toString();
      doc["gw"] = WiFi.gatewayIP().toString();
      doc["nm"] = WiFi.subnetMask().toString();
 
      if (server.arg("signalStrength")== "true"){
          doc["signalStrengh"] = WiFi.RSSI();
      }
 
      if (server.arg("chipInfo")== "true"){
          doc["chipId"] = ESP.getChipId();
          doc["flashChipId"] = ESP.getFlashChipId();
          doc["flashChipSize"] = ESP.getFlashChipSize();
          doc["flashChipRealSize"] = ESP.getFlashChipRealSize();
      }
      if (server.arg("freeHeap")== "true"){
          doc["freeHeap"] = ESP.getFreeHeap();
      }
 
      Serial.print(F("Stream..."));
      String buf;
      serializeJson(doc, buf);
      server.send(200, F("application/json"), buf);
      Serial.print(F("done."));
}
 
void setSettings() {
    // expected
    // {"ip":"192.168.1.129","gw":"192.168.1.1","nm":"255.255.255.0"}
    Serial.println(F("postConfigFile"));
 
    setCrossOrigin();
 
    String postBody = server.arg("plain");
    Serial.println(postBody);
 
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        // if the file didn't open, print an error:
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());
 
        String msg = error.c_str();
 
        server.send(400, F("text/html"), "Error in parsin json body! <br>"+msg);
 
    }else{
        JsonObject postObj = doc.as<JsonObject>();
 
        Serial.print(F("HTTP Method: "));
        Serial.println(server.method());
 
        if (server.method() == HTTP_POST) {
            if ((postObj.containsKey("ip"))) {
 
                Serial.print(F("Open config file..."));
                // Here you can open your file to store your config
                // Now I simulate It and set configFile a true
                bool configFile = true;
                if (!configFile) {
                    Serial.println(F("fail."));
                    server.send(304, F("text/html"), F("Fail to store data, can't open file!"));
                }else{
                    Serial.println(F("done."));
                    const char* address = postObj[F("ip")];
                    const char* gatway = postObj[F("gw")];
                    const char* netMask = postObj[F("nm")];
 
                    Serial.print("ip: ");
                    Serial.println(address);
                    Serial.print("gw: ");
                    Serial.println(gatway);
                    Serial.print("nm: ");
                    Serial.println(netMask);
 
//                  server.sendHeader("Content-Length", String(postBody.length()));
                    server.send(201, F("application/json"), postBody);
 
//                  Serial.println(F("Sent reset page"));
//                    delay(5000);
//                    ESP.restart();
//                    delay(2000);
                }
            }
            else {
                server.send(204, F("text/html"), F("No data found, or incorrect!"));
            }
        }
    }
}
 
 
void sendCrossOriginHeader(){
    Serial.println(F("sendCORSHeader"));
 
    server.sendHeader(F("access-control-allow-credentials"), F("false"));
 
    setCrossOrigin();
 
    server.send(204);
}

void testServerRouting() {
  server.on("/control", HTTP_GET, handleControl);

  server.on("/all", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
}

void apiServerRouting() {
  server.on("/api/control", HTTP_GET, getPinData);
  server.on("/api/control", HTTP_PUT, changePinData);
  server.on("/api/control", HTTP_OPTIONS, sendCrossOriginHeader);
}

// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    server.on(F("/helloWorld"), HTTP_GET, getHelloWord);
    server.on(F("/settings"), HTTP_OPTIONS, sendCrossOriginHeader);
    server.on(F("/settings"), HTTP_GET, getSettings);
    server.on(F("/settings"), HTTP_POST, setSettings);
}

void fileServerRouting() {
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
}
 
// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleControl() {
  //http://192.168.1.3/control?pin=2&state=0
  if(!server.hasArg("pin") || !server.hasArg("state")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String pinString = server.arg("pin");
  String stateString = server.arg("state");
  int state = LOW;
  if(stateString == "0" || stateString == "on")
    state = HIGH;
  Serial.println("Switching pin: "+pinString +" to state: " + state);
  //digitalWrite(LED_BUILTIN, state);
  long pin = pinString.toInt();
  digitalWrite(pin, state);
  server.send(200, "text/json", "{\"response\":\"OK\"}");
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  Serial.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}

void setupOutputs(){
/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(D1, OUTPUT);
  digitalWrite(D1, LOW);
}
 
void setup(void) {
  Serial.begin(115200);
  //setupOutputs();
  setupAPI();
  setupNeoPixel();

// Setup Files
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
 
  // Set server routing
  //restServerRouting();
  fileServerRouting();
  //testServerRouting();
  apiServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop(void) {
  server.handleClient();
  runNeoPixel();
}

void delayAndAcceptHttp(int wait){
  server.handleClient();
  delay(wait);
  server.handleClient();
}


// ------------------------------ NEOPixel ---------------------------------
void setupNeoPixel() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}

// Some functions of our own for creating animated effects -----------------

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delayAndAcceptHttp(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delayAndAcceptHttp(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delayAndAcceptHttp(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delayAndAcceptHttp(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
