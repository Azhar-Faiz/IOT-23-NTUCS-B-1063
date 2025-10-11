#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void drawLogo() {

  display.clearDisplay();

  display.fillCircle(64, 32, 24, SSD1306_WHITE);
  display.fillCircle(64, 32, 18, SSD1306_BLACK);
  display.fillCircle(64, 32, 10, SSD1306_WHITE);
  display.display();
}

void setup() {
  Wire.begin(21, 22); 

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // If OLED not detected, stop program
    for (;;);
  }

  drawLogo();  
}

void loop() {
  // blinking effect 
  display.invertDisplay(true);
  delay(150);
  display.invertDisplay(false);
  delay(150);
}
