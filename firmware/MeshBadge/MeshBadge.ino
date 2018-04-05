// OLED Includes
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// NeoPixel Includes
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif


// Pins, pins, pins
#define BUTTON_A 15
#define BUTTON_C 14
#define LED      13
#define NEOPIXEL_PIN 32
#define LIPO_PIN A13

#define TEXTARRAYSIZE 6

// All in seconds
#define TEXTUPDATEINTERVAL 10
#define BATTERYCHECKINTERVAL 60
#define TWITTERCHECKINTERVAL 360
#define LOWVOLTAGETHRESHOLD 3.3

long lastDisplayChange = 0;
long lastBatteryCheck = 0;
int textIndex = 0;
bool updateDisplayRotator = true;
String textStrings[TEXTARRAYSIZE] = { "Brandon", "Satrom", "DevRel @", "Particle", "@Brandon", "Satrom" };

// Loop control booleans
bool displayUpdating = false;
bool neoPixelsUpdating = false;
bool batteryChecking = false;

//LiPo Management variables
float startVoltage = 0;
float currentVoltage = 0;
float maxVoltage = 4.2;

Adafruit_SSD1306 display = Adafruit_SSD1306();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(32, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Configure Hardware Input Buttons
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // Initialize with the I2C addr 0x3C (for the 128x32)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Clear the buffer.
  display.display();
  delay(1000);

  // Start the process to paint the OLED
  paintScreen();

  // Check the Voltage of the Power Supply
  checkBattery();

  // Init NeoPixel display
  initNeoPixelDisplay();
}

void loop() {
  if (millis() - lastDisplayChange > TEXTUPDATEINTERVAL * 1000 && updateDisplayRotator && !displayUpdating) {
    displayUpdating = true;
    lastDisplayChange = millis();

    paintScreen();
    displayUpdating = false;
  }

  if (millis() - lastBatteryCheck > BATTERYCHECKINTERVAL * 1000) {
    lastBatteryCheck = millis();
    checkBattery();
  }

  if (! digitalRead(BUTTON_A) && !neoPixelsUpdating) {
    neoPixelsUpdating = true;
    
    clearNeoPixelDisplay();
    strip.setBrightness(15);

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

    neoPixelsUpdating = false;
  }

  if (! digitalRead(BUTTON_C) && !batteryChecking) {
    batteryChecking = true;
    
    displayBatteryCharge();

    batteryChecking = false;
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
  display.setTextWrap(false);
  display.setTextSize(textSize);

  display.println(line1);
  display.println(line2);
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
