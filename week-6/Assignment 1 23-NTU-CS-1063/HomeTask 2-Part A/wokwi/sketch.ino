// Week4-BonusTask
// Embedded IoT System Fall-2025
//
// Name: Muhammad Azhar Faiz                 Reg#: 23-ntu-cs-1063

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- OLED setup ----
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---- Pins ----
#define LED1 17
#define LED2 18
#define LED3 19
#define MODE_BTN 25
#define RESET_BTN 26

// ---- PWM ----
#define PWM_LED3 0

// ---- Constants ----
#define DEBOUNCE_US 50000 // 50ms
#define OLED_REFRESH_MS 300

// ---- Timers ----
hw_timer_t *debounceTimerMode = nullptr;
hw_timer_t *debounceTimerReset = nullptr;
volatile bool debounceModeActive = false;
volatile bool debounceResetActive = false;
volatile bool modeEvent = false;
volatile bool resetEvent = false;

// ---- App state ----
int mode = 0;
unsigned long lastAltStep = 0;
int altState = 0;

// ---- ISRs ----
void IRAM_ATTR onModeDebounce() {
if (digitalRead(MODE_BTN) == LOW) modeEvent = true;
debounceModeActive = false;
}
void IRAM_ATTR onResetDebounce() {
if (digitalRead(RESET_BTN) == LOW) resetEvent = true;
debounceResetActive = false;
}
void IRAM_ATTR modeISR() {
if (!debounceModeActive) {
debounceModeActive = true;
timerWrite(debounceTimerMode, 0);
timerAlarmWrite(debounceTimerMode, DEBOUNCE_US, false);
timerAlarmEnable(debounceTimerMode);
}
}
void IRAM_ATTR resetISR() {
if (!debounceResetActive) {
debounceResetActive = true;
timerWrite(debounceTimerReset, 0);
timerAlarmWrite(debounceTimerReset, DEBOUNCE_US, false);
timerAlarmEnable(debounceTimerReset);
}
}

// ---- Helpers ----
void setAllOff() {
digitalWrite(LED1, LOW);
digitalWrite(LED2, LOW);
ledcWrite(PWM_LED3, 0);
}
void setAllOn() {
digitalWrite(LED1, HIGH);
digitalWrite(LED2, HIGH);
ledcWrite(PWM_LED3, 255);
}
void alternateStep() {
digitalWrite(LED1, altState ? HIGH : LOW);
digitalWrite(LED2, altState ? LOW : HIGH);
ledcWrite(PWM_LED3, 0);
altState = !altState;
}
String currentLabel() {
switch (mode) {
case 0: return "ALL OFF";
case 1: return "ALTERNATE";
case 2: return "ALL ON";
case 3: return "PWM FADE";
default: return "UNKNOWN";
}
}
void drawOLED() {
display.clearDisplay();
display.setTextSize(2);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);
display.print("State:\n" + currentLabel());
display.display();
}

// ---- Setup ----
void setup() {
Serial.begin(115200);
display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

pinMode(LED1, OUTPUT);
pinMode(LED2, OUTPUT);
pinMode(LED3, OUTPUT);
pinMode(MODE_BTN, INPUT_PULLUP);
pinMode(RESET_BTN, INPUT_PULLUP);

ledcSetup(PWM_LED3, 5000, 8);
ledcAttachPin(LED3, PWM_LED3);

debounceTimerMode = timerBegin(0, 80, true);
debounceTimerReset = timerBegin(1, 80, true);
timerAttachInterrupt(debounceTimerMode, &onModeDebounce, true);
timerAttachInterrupt(debounceTimerReset, &onResetDebounce, true);

attachInterrupt(digitalPinToInterrupt(MODE_BTN), modeISR, FALLING);
attachInterrupt(digitalPinToInterrupt(RESET_BTN), resetISR, FALLING);

setAllOff();
drawOLED();
}

// ---- Loop ----
void loop() {
unsigned long now = millis();

if (modeEvent) {
modeEvent = false;
mode = (mode + 1) % 4;
drawOLED();
}

if (resetEvent) {
resetEvent = false;
mode = 0;
setAllOff();
drawOLED();
}

switch (mode) {
case 0: setAllOff(); break;
case 1:
if (now - lastAltStep >= 200) {
alternateStep();
lastAltStep = now;
}
break;
case 2: setAllOn(); break;
case 3:
digitalWrite(LED1, LOW);
digitalWrite(LED2, LOW);
for (int v = 0; v <= 255; ++v) {
if (modeEvent || resetEvent) break;
ledcWrite(PWM_LED3, v);
delay(2);
}
for (int v = 255; v >= 0; --v) {
if (modeEvent || resetEvent) break;
ledcWrite(PWM_LED3, v);
delay(2);
}
break;
}

delay(5);
}