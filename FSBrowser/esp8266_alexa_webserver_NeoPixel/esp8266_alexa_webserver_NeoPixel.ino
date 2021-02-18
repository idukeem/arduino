/*
  This sketch demonstrates how to control your WS2812FX lights with your voice using
  an Amazon Echo and Amazon's digital assistant, Alexa. 

  The sketch uses Aircoookie's excellent Espalexa library, which you can download here:
  https://github.com/Aircoookie/Espalexa. After downloading it, install it using the
  Arduino IDE Library Manager's "Add .ZIP library" feature.

  *** HACK ALERT ***
  There's an issue with the ESP8266WebServer code shipped with recent versions of the
  ESP8266 Arduino Core package (versions >= v2.4.0). The bug affects Alexa's ability to
  discover compatible devices on the network and/or your ESP's ability to respond to
  Alexa's commands. There's two workarounds, pick whichever one suits you:
    1) drop back to using ESP8266 Core v2.3.0, which doesn't seem to have the issue.
    2) patch the ESP8266WebServer Parsing.cpp file as described in this thread:
       https://github.com/Aircoookie/Espalexa/issues/6#issuecomment-366533897

  I opted for workaround #2 and my 2nd Gen Echo Dot (fireware version 611498620) worked
  fine. Note, on my Mac setup the file is located at
  /Users/{userid}/Library/Arduino15/packages/esp8266/hardware/esp8266/2.4.1/libraries/ESP8266WebServer/src/Parsing.cpp
  and the patch needs to be applied to line 198.

  Hopefully this will be fixed in the ESP8266 Core v2.5.0 release or perhaps it
  will be fixed with a future Amazon Echo firmware update.
  *** END HACK ALERT ***

  After uploading the sketch to your ESP, you'll need to ask Alexa to discover the
  device on your network by saying "Alexa, discover devices". After a few seconds,
  hopefully, Alexa will respond that it found the "devices" defined in the sketch.
  If Alexa responds with something about making sure you've enabled the SmartHome
  skill appropriate for your device, ignore it. A skill is not needed for the
  Espalexa library to do its thing.

  The sketch defines a set of virtual devices and WS2812FX presets which will be
  activated when Alexa hears the device's name in a verbal command. After Alexa
  has found your devices on the network, you can say things like
  "Alexa, turn on christmas lights"
  "Alexa, turn on christmas lights 30 percent" (sets the brightness to 30%)
  "Alexa, turn off christmas lights"

  You can also control your lights with the Alexa app on your smartphone. Look for your
  virtual devices in the Smart Home section.

  See the Espalexa documentation for more details.
  
  Keith Lord - 2018

  LICENSE

  The MIT License (MIT)

  Copyright (c) 2018  Keith Lord 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  
  CHANGELOG
  2018-06-10 initial version
*/

#include <Espalexa.h>
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <WebServer.h> //if you get an error here please update to ESP32 arduino core 1.0.0
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif
#include <WS2812FX.h>

// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define KINO_NUMLEDS 55
#define KINO_DATAPIN  5 // D1
#define KINOFILM_DATAPIN 4 // D2
#define KINOFILM_NUMLEDS 1
#define KINOLIGHT_PIN 16 //D0

#define WIFI_SSID     "Marcin-Privat-5G" // WiFi network
#define WIFI_PASSWORD "Krieg$str.37-5G" // WiFi network password
#define WIFI_TIMEOUT 30000

#define DEFAULT_COLOR 0xFF5900 //20F9FF
#define DEFAULT_BRIGHTNESS 128
#define DEFAULT_SPEED 4918 //1000
#define DEFAULT_MODE FX_MODE_THEATER_CHASE

extern const char index_html[];
extern const char main_js[];

/* 
 *  define your ws2812fx presets
 */
WS2812FX::segment color_preset[MAX_NUM_SEGMENTS] = {
  // { first, last, speed, mode, options, colors[] }
  {0, KINO_NUMLEDS-1, 5000, FX_MODE_STATIC, NO_OPTIONS, {RED, GREEN, BLUE}}
};

WS2812FX::segment christmas_preset[MAX_NUM_SEGMENTS] = {
  // { first, last, speed, mode, options, colors[] }
  {0, KINO_NUMLEDS-1, 5000, FX_MODE_MERRY_CHRISTMAS, NO_OPTIONS, {RED, GREEN, BLACK}}
};

WS2812FX::segment team_preset[MAX_NUM_SEGMENTS] = {
  // { first, last, speed, mode, options, colors[] }
  {0, KINO_NUMLEDS-1, 5000, FX_MODE_TRICOLOR_CHASE, NO_OPTIONS, {0x805000, 0x805000, 0x000040}} // blue and gold, GO ND!
};

WS2812FX::segment flag_preset[MAX_NUM_SEGMENTS] = {
  // { first, last, speed, mode, options, colors[] }
  {0,             (KINO_NUMLEDS/6)*1-1,  500, FX_MODE_FLASH_SPARKLE, NO_OPTIONS, {BLUE,  BLACK, BLACK}},
  {(KINO_NUMLEDS/6)*1, (KINO_NUMLEDS/6)*2-1,  500, FX_MODE_FLASH_SPARKLE, NO_OPTIONS, {BLUE,  BLACK, BLACK}},
  {(KINO_NUMLEDS/6)*2, (KINO_NUMLEDS/6)*3-1, 2000, FX_MODE_COLOR_WIPE,    NO_OPTIONS, {RED,   WHITE, BLACK}},
  {(KINO_NUMLEDS/6)*3, (KINO_NUMLEDS/6)*4-1, 2000, FX_MODE_COLOR_WIPE,    NO_OPTIONS, {WHITE, RED,   BLACK}},
  {(KINO_NUMLEDS/6)*4, (KINO_NUMLEDS/6)*5-1, 2000, FX_MODE_COLOR_WIPE,    NO_OPTIONS, {RED,   WHITE, BLACK}},
  {(KINO_NUMLEDS/6)*5, KINO_NUMLEDS-1,       2000, FX_MODE_COLOR_WIPE,    NO_OPTIONS, {WHITE, RED,   BLACK}}
};

/*
 * define the ws2812fx and espalexa objects
 */
Espalexa espalexa;
WS2812FX kino_ws2812fx = WS2812FX(KINO_NUMLEDS, KINO_DATAPIN, NEO_GRB + NEO_KHZ800);
WS2812FX kinoFilm_ws2812fx = WS2812FX(KINOFILM_NUMLEDS, KINOFILM_DATAPIN, NEO_GRB + NEO_KHZ800);
#ifdef ARDUINO_ARCH_ESP32
WebServer server(80);
#else
ESP8266WebServer server(80);
#endif

unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;
String modes = "";
uint8_t myModes[] = {}; // *** optionally create a custom list of effect/mode numbers
boolean auto_cycle = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n");

  wifi_setup();

  modes.reserve(5000);
  modes_setup();
  
  setupWebserver();
  // add your alexa virtual devices giving them a name and associated callback
  espalexa.addDevice("Kino", kinoCallback);
  espalexa.addDevice("Kinofilm", kinoFilmCallback);


  //espalexa.begin();
  espalexa.begin(&server);
  setup_ws2812fx();
  #ifdef KINOLIGHT_PIN
  pinMode(KINOLIGHT_PIN, OUTPUT);
  #endif
}

void setup_ws2812fx() {
  kino_ws2812fx.init();
  kino_ws2812fx.setMode(DEFAULT_MODE);
  kino_ws2812fx.setColor(DEFAULT_COLOR);
  kino_ws2812fx.setSpeed(DEFAULT_SPEED);
  kino_ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  kino_ws2812fx.stop(); // ws2812fx is stopped until it receives a command from ALexa

  kinoFilm_ws2812fx.init();
  kinoFilm_ws2812fx.setMode(FX_MODE_RAINBOW);
  kinoFilm_ws2812fx.setColor(DEFAULT_COLOR);
  kinoFilm_ws2812fx.setSpeed(DEFAULT_SPEED);
  kinoFilm_ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  kinoFilm_ws2812fx.stop(); // ws2812fx is stopped until it receives a command from ALexa
}

void wifi_setup() {
  // init WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to " WIFI_SSID);
  unsigned long connect_start = millis();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if(millis() - connect_start > WIFI_TIMEOUT) {
      Serial.println();
      Serial.print("Tried ");
      Serial.print(WIFI_TIMEOUT);
      Serial.print("ms. Resetting ESP now.");
      ESP.reset();
    }
  }
  Serial.print("\nServer IP is ");
  Serial.println(WiFi.localIP());
}

void setupWebserver() {
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/set", srv_handle_set);
  server.onNotFound(srv_handle_not_found);
}
 
void loop() {
   espalexa.loop();
   kino_ws2812fx.service();
   kinoFilm_ws2812fx.service();
   delay(1);
   checkWiFiConnection();
}

void checkWiFiConnection(){
  unsigned long now = millis();
   if(now - last_wifi_check_time > WIFI_TIMEOUT) {
    Serial.print("Checking WiFi... ");
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Reconnecting...");
      wifi_setup();
    } else {
      Serial.println("OK");
    }
    last_wifi_check_time = now;
  }
}

/*
 * our callback functions
 */
void kinoCallback(uint8_t brightness) { // used for on, off or adjusting brightness without changing the active preset
  Serial.print("Kino ");Serial.println(brightness);
  kino_lights(brightness, NULL);
}

void kinoFilmCallback(uint8_t brightness) { // used for on, off or adjusting brightness without changing the active preset
  Serial.print("Kinofilm ");Serial.println(brightness);
  kinoFilm_ws2812fx.stop();
  if(brightness == 0) return;
  kinoFilm_ws2812fx.setBrightness(brightness);
  kinoFilm_ws2812fx.start();
}

void kino_lights_on() {
  kino_lights(kino_ws2812fx.getBrightness(), NULL);
}

void kino_lights_off() {
  kino_lights(0, NULL);
}

void kino_lights(uint8_t brightness, WS2812FX::segment segments[]) {
  kino_ws2812fx.stop();
  if(brightness == 0) {
    #ifdef KINOLIGHT_PIN
    digitalWrite(KINOLIGHT_PIN, LOW);
    #endif
    return;
  }

  if(segments != NULL) {
    kino_ws2812fx.resetSegments();
    for(int i=0; i<MAX_NUM_SEGMENTS; i++) {
      WS2812FX::segment seg = segments[i];
      if(i != 0 && seg.start == 0) break;
      kino_ws2812fx.setSegment(i, seg.start, seg.stop, seg.mode, seg.colors, seg.speed, seg.options);
    }
  }

  #ifdef KINOLIGHT_PIN
  digitalWrite(KINOLIGHT_PIN, HIGH);
  #endif
  kino_ws2812fx.setBrightness(brightness);
  kino_ws2812fx.start();
}

/*
 * Build <li> string for all modes.
 */
void modes_setup() {
  modes = "";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : kino_ws2812fx.getModeCount();
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    modes += "<li><a href='#'>";
    modes += kino_ws2812fx.getModeName(m);
    modes += "</a></li>";
  }
}

/* #####################################################
#  Webserver Functions
##################################################### */

void srv_handle_not_found() {
  if (!espalexa.handleAlexaApiCall(server.uri(),server.arg(0))) //if you don't know the URI, ask espalexa whether it is an Alexa control request
  {
        //whatever you want to do with 404s
        server.send(404, "text/plain", "Not found");
  }
}

void srv_handle_index_html() {
  server.send_P(200,"text/html", index_html);
}

void srv_handle_main_js() {
  server.send_P(200,"application/javascript", main_js);
}

void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}

void srv_handle_set() {
  for (uint8_t i=0; i < server.args(); i++){
    if(server.argName(i) == "c") {
      uint32_t tmp = (uint32_t) strtol(server.arg(i).c_str(), NULL, 10);
      if(tmp >= 0x000000 && tmp <= 0xFFFFFF) {
        kino_ws2812fx.setColor(tmp);
      }
      Serial.print("color is "); Serial.println(kino_ws2812fx.getColor());
    }

    if(server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
      kino_ws2812fx.setMode(tmp % kino_ws2812fx.getModeCount());
      Serial.print("mode is "); Serial.println(kino_ws2812fx.getModeName(kino_ws2812fx.getMode()));
    }

    if(server.argName(i) == "p") {
      if(server.arg(i)[0] == '-') {
        Serial.print("power is off");
        kino_lights_off();
      } else {
        Serial.print("power is on");
        kino_lights_on();
      }
      
    }
    
    if(server.argName(i) == "b") {
      if(server.arg(i)[0] == '-') {
        kino_ws2812fx.setBrightness(kino_ws2812fx.getBrightness() * 0.8);
      } else if(server.arg(i)[0] == ' ') {
        kino_ws2812fx.setBrightness(min(max(kino_ws2812fx.getBrightness(), 5) * 1.2, 255));
      } else { // set brightness directly
        uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
        kino_ws2812fx.setBrightness(tmp);
      }
      Serial.print("brightness is "); Serial.println(kino_ws2812fx.getBrightness());
    }

    if(server.argName(i) == "s") {
      if(server.arg(i)[0] == '-') {
        kino_ws2812fx.setSpeed(max(kino_ws2812fx.getSpeed(), 5) * 1.2);
      } else if(server.arg(i)[0] == ' ') {
        kino_ws2812fx.setSpeed(kino_ws2812fx.getSpeed() * 0.8);
      } else {
        uint16_t tmp = (uint16_t) strtol(server.arg(i).c_str(), NULL, 10);
        kino_ws2812fx.setSpeed(tmp);
      }
      Serial.print("speed is "); Serial.println(kino_ws2812fx.getSpeed());
    }

    if(server.argName(i) == "a") {
      if(server.arg(i)[0] == '-') {
        auto_cycle = false;
      } else {
        auto_cycle = true;
        auto_last_change = 0;
      }
    }
  }
  server.send(200, "text/plain", "OK");
}
