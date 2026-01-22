#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>


#define SDA_PIN 21       // I2C SDA
#define SCL_PIN 22       // I2C SCL
const char* ssid     = "Redmi 12C";
const char* password = "nibatana";

// --- OLED setup ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// --- Setup function ---
void setup() {
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");

  // Initialize I2C on custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);   // Start connection

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Initialize DHT sensor
  display.println("WiFi Connected!");
  display.print("IP Address: ");
  display.println(WiFi.localIP());
  display.display();
}

// --- Main loop ---
void loop() {
  
}


