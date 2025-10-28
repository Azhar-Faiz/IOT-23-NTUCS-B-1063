#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LED_1 17
#define LED_2 18
#define LED_3 19
#define BTN 27
#define BZR 14
#define LONG_PRESS 1500 // in ms

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int melody[] = {262, 294, 262, 349, 330, 262, 294, 262, 392, 349, 262, 523, 440, 349, 330, 294, 466, 440, 349, 392, 349};


bool melodyPlaying = false;

void updateDisplay(const char *msg) {
display.clearDisplay();
display.setTextSize(2);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0,0);
display.print(msg);
display.display();
}

void playMelody() {
static int i = 0;
ledcWriteTone(0, melody[i]);
delay(1000);
i = (i + 1) % (sizeof(melody) / sizeof(melody[0]));
}

void stopMelody() {
ledcWriteTone(0, 0);
melodyPlaying = false;
}

void setup() {
pinMode(LED_1, OUTPUT);
pinMode(LED_2, OUTPUT);
pinMode(LED_3, OUTPUT);
pinMode(BTN, INPUT_PULLUP);

ledcAttachPin(BZR, 0);
Wire.begin(21, 22);
display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
updateDisplay("Ready");

digitalWrite(LED_1, LOW);
digitalWrite(LED_2, LOW);
digitalWrite(LED_3, LOW);
}

void loop() {
if (digitalRead(BTN) == LOW) {
unsigned long startTime = millis();
while (digitalRead(BTN) == LOW) delay(10);
unsigned long pressTime = millis() - startTime;

if (pressTime >= LONG_PRESS) {
  melodyPlaying = true;
  updateDisplay("\nMelody");
  while (melodyPlaying) {
    playMelody();
    // Check for short press to stop
    if (digitalRead(BTN) == LOW) {
      unsigned long shortStart = millis();
      while (digitalRead(BTN) == LOW) delay(10);
      unsigned long shortPressTime = millis() - shortStart;
      if (shortPressTime < LONG_PRESS) {
        stopMelody();
        updateDisplay("Melody\nStopped");
        break;
      }
    }
  }
} else {
  // Short press toggles LEDs or stops melody if playing
  if (melodyPlaying) {
    stopMelody();
    updateDisplay("Melody\nStopped");
  } else {
    static bool ledState = false;
    ledState = !ledState;
    digitalWrite(LED_1, ledState);
    digitalWrite(LED_2, ledState);
    digitalWrite(LED_3, ledState);
    updateDisplay("\nLED");
  }
}


}
}