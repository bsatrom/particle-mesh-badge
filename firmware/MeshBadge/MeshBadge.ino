// OLED Includes
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Client request libraries
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wifi Libraries
#include <WiFi.h>
#include <WiFiMulti.h>

// NeoPixel Includes
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define BUTTON_A 15
#define BUTTON_C 14
#define LED      13
#define NEOPIXEL_PIN 32
#define LIPO_PIN A13

// Sketch values
#define TEXTARRAYSIZE 6
// All in seconds
#define TEXTUPDATEINTERVAL 10
#define BATTERYCHECKINTERVAL 60
#define TWITTERCHECKINTERVAL 360
#define LOWVOLTAGETHRESHOLD 3.3

// Wifi values
WiFiMulti wifiMulti;

const char* ssid     = "5at70m";
const char* password = "Sarah0812!";

// Rest Interface constants
const char* tweetHost = "https://sheets.googleapis.com"; // Tweets are stored in a Google Sheet via IFTTT
const char* apiKey = "AIzaSyBzcsBpKziCrhy68nfm6h1Wnw1G6C9S53M";
const char* sheetId = "1pMINVucqHspaUVzp_kydLzkL9WYeM9D3TnOeb8l1ouw";
StaticJsonBuffer<200> jsonBuffer;

long lastDisplayChange = 0;
long lastBatteryCheck = 0;
int textIndex = 0;
bool updateDisplayRotator = true;
String textStrings[TEXTARRAYSIZE] = { "Brandon", "Satrom", "DevRel @", "Particle", "@Brandon", "Satrom" };

//LiPo Management variables
float startVoltage = 0;
float currentVoltage = 0;
float maxVoltage = 4.2;

Adafruit_SSD1306 display = Adafruit_SSD1306();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(32, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  // Configure Hardware Input Buttons
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // Initialize with the I2C addr 0x3C (for the 128x32)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();
  paintScreen();

  //Connnect to Wifi
  wifiMulti.addAP(ssid, password);
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
  }
  scrollText("WiFi", "Connected!", 2);

  // Check the Voltage of the Power Supply
  checkBattery();

  // Init NeoPixel display
  initNeoPixelDisplay();

  // Check twitter
  // checkTwitterMentions();
}

void loop() {
  if (millis() - lastDisplayChange > TEXTUPDATEINTERVAL * 1000 && updateDisplayRotator) {
    lastDisplayChange = millis();

    display.clearDisplay();
    paintScreen();
  }

  if (millis() - lastBatteryCheck > BATTERYCHECKINTERVAL * 1000) {
    lastBatteryCheck = millis();
    checkBattery();
  }

  if (! digitalRead(BUTTON_A)) {
    clearNeoPixelDisplay();
    strip.setBrightness(25);

    colorWipe(strip.Color(255, 0, 0), 50); // Red
    colorWipe(strip.Color(0, 255, 0), 50); // Green
    colorWipe(strip.Color(0, 0, 255), 50); // Blue
    // Send a theater pixel chase in...
    theaterChase(strip.Color(127, 127, 127), 50); // White
    theaterChase(strip.Color(127, 0, 0), 50); // Red
    theaterChase(strip.Color(0, 0, 127), 50); // Blue

    rainbow(20);
    rainbowCycle(20);
    theaterChaseRainbow(50);

    clearNeoPixelDisplay();
  }

  if (! digitalRead(BUTTON_C)) {
    displayBatteryCharge();
  }
}

void checkBattery() {
  // Check battery
  int sensorVal = analogRead(LIPO_PIN);
  float batteryVoltage = (sensorVal / 4095.0) * 2 * 3.3 * 1.1;

  currentVoltage = batteryVoltage;

  // If under the voltage threshold set, display Low Batt message
  if (currentVoltage < LOWVOLTAGETHRESHOLD) {
    updateDisplayRotator = false;
    scrollText("LOW", "BATTERY!", 2);
    delay(10000);
    updateDisplayRotator = true;
  }
}

void displayBatteryCharge() {
  String chargePercentage = String(currentVoltage / maxVoltage * 100.0) + "%";
  Serial.println(chargePercentage);

  display.clearDisplay();
  display.setTextSize(3);

  display.println(chargePercentage);
  display.setCursor(0, 0);
  display.display();
  display.startscrollleft(0x00, 0x0F);
}

void initNeoPixelDisplay() {
  strip.setBrightness(25);
  strip.begin();

  strip.show(); // Initialize all pixels to 'off'
  colorWipe(strip.Color(0, 0, 255), 50); // Blue
}

void clearNeoPixelDisplay() {
  strip.setBrightness(0);
  strip.show();
  strip.show();
  strip.show(); // Seem to have to call 3x to really get all pixels off
}

void paintScreen() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  scrollText(textStrings[textIndex], textStrings[textIndex + 1], 2);
  if (textIndex + 2 == TEXTARRAYSIZE) {
    textIndex = 0;
  } else {
    textIndex = textIndex + 2;
  }
}

// Set scrolling text on the OLED screen
void scrollText(String line1, String line2, int textSize) {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(textSize);

  display.println(line1);
  display.println(line2);
  display.setCursor(0, 0);
  display.display();
  display.startscrollleft(0x00, 0x0F);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c);  //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void checkTwitterMentions() {
  // Create a URI for the request
  String url = tweetHost;
  url += "/v4/spreadsheets/";
  url += sheetId;
  url += "/values:batchGet?dateTimeRenderOption=SERIAL_NUMBER&majorDimension=COLUMNS&ranges=D1%3AD500&valueRenderOption=UNFORMATTED_VALUE";
  url += "&key=";
  url += apiKey;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // Use WiFiClient class to create TCP connections
  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      int len = http.getSize();
      // create buffer for read
      uint8_t buff[128] = { 0 };

      //WiFiClient * stream = http.getStreamPtr();
      
      // read all data from server
      //while (http.connected() && (len > 0 || len == -1)) {
        // get available data size
        //size_t size = stream->available();

        //if (size) {
          // read up to 128 byte
          //int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          String root = http.getString();

          // write it to a String
          //Serial.write(buff, c);
          //JsonArray& root = jsonBuffer.parseArray(buff);
          
          Serial.printf("[ROOT]: %s\n", root);
          
          //if (len > 0) {
          //  len -= c;
          //}
        //} else {
          http.end();
        //}
        //delay(1);
      //}
    }
  }

  http.end();
}
