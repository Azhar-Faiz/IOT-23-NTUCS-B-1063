#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- OLED setup ----
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void DisplayNameandRec() {

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4,2);
  display.setTextSize(1);
  display.println("Azhar Faiz");
  display.setCursor(4,10);
  display.println("23-NTU-CS-1063");
  // Rectangles
  display.drawRect(1, 1, 100, 20, SSD1306_WHITE);
}


void setup() {
  Wire.begin(21, 22); // ESP32 default I2C pins (SDA=21, SCL=22)

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // If it fails, check wiring and address (0x3C/0x3D)
    for (;;);
  }
}

void loop() {
  display.clearDisplay();
  DisplayNameandRec();
  display.display();
  delay(2000);

}

