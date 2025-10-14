#include <Arduino.h>

#define BUZZER_PIN  27     // GPIO connected to buzzer
#define BUZ_CH      0      // PWM channel (0–15)
#define BUZ_FREQ        2000   // Default frequency (Hz)
#define BUZ_RESOLUTION  10     // 10-bit resolution (0–1023)

#define LED1_PIN 18
#define LED1_CH 1
#define LED1_FREQ 5000

#define LED2_PIN 19
#define LED2_CH 2
#define LED2_FREQ 8000

#define LED_RES 8

void setup() {
  //LED1
   ledcSetup(LED1_CH, LED1_FREQ, LED_RES);
  ledcAttachPin(LED1_PIN, LED1_CH);
  //LED2
   ledcSetup(LED2_CH, LED2_FREQ, LED_RES);
  ledcAttachPin(LED2_PIN, LED2_CH);
  //BUZ
  ledcSetup(BUZ_CH, BUZ_FREQ, BUZ_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZ_CH);

  // --- 1. Simple beep pattern ---
  for (int i = 0; i < 3; i++) {
    ledcWriteTone(BUZ_CH, 2000 + i * 400); // change tone
    delay(150);
    ledcWrite(BUZ_CH, 0);                  // stop tone
    delay(150);
  }

  // --- 2. Frequency sweep (400Hz → 3kHz) ---
  for (int f = 400; f <= 3000; f += 100) {
    ledcWriteTone(BUZ_CH, f);
    delay(20);
  }
  ledcWrite(BUZ_CH, 0);
  delay(500);

  // --- 3. Short melody ---
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
  for (int i = 0; i < 8; i++) {
    ledcWriteTone(BUZ_CH, melody[i]);
    delay(250);
  }
  ledcWrite(BUZ_CH, 0); // stop buzzer
}

void loop() {
  for (int i = 0; i < 3; i++) {
    ledcWriteTone(BUZ_CH, 2000 + i * 400); // change tone
    delay(150);
    ledcWrite(BUZ_CH, 0);                  // stop tone
    delay(150);
  }

  // --- 2. Frequency sweep (400Hz → 3kHz) ---
  for (int f = 400; f <= 3000; f += 100) {
    ledcWriteTone(BUZ_CH, f);
    delay(20);
  }
  ledcWrite(BUZ_CH, 0);
  delay(500);

  // --- 3. Short melody ---
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
  for (int i = 0; i < 8; i++) {
    ledcWriteTone(BUZ_CH, melody[i]);
    delay(250);
  }
  //LED1
  for (int d=0; d<=255; d=d+10) { 
ledcWrite(LED1_CH, d); delay(10);
ledcWrite(LED2_CH, d); delay(10);
  //LED2
 }
  for (int d=255; d>=0; d=d-20) { 
ledcWrite(LED1_CH, d); delay(10);
ledcWrite(LED2_CH, d); delay(10);

 }
  ledcWrite(BUZ_CH, 0); // stop buzzer
}