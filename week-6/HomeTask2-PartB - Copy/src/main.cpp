// ESP32: LED Modes + Reset + BTN3 short/long behavior (melody loops / LED toggle)
// LEDs: 17,18,19 | Mode Btn:25 | Reset Btn:26 | BTN3:27 | Buzzer:14 | OLED I2C (21,22)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- Pins ----------------
#define LED1 17
#define LED2 18
#define LED3 19
#define MODE_BTN 25
#define RESET_BTN 26
#define BTN3 27
#define BUZZER_PIN 14

// ---------------- PWM channels ----------------
const uint8_t PWM1 = 0;
const uint8_t PWM2 = 1;
const uint8_t PWM3 = 2;
const uint8_t PWM_BUZ = 3; // buzzer channel for ledcWriteTone

// ---------------- Constants ----------------
const uint32_t DEBOUNCE_US  = 50000; // 50 ms
const uint32_t LONGPRESS_MS = 1500;  // 1.5 s
const uint32_t LED_TOGGLE_MS = 500;  // LED toggle interval when BTN3 short pressed
const uint32_t MELODY_NOTE_MS = 300; // per-note time

// ---------------- Debounce timers ----------------
hw_timer_t *debounceTimer1 = nullptr;
hw_timer_t *debounceTimer2 = nullptr;
hw_timer_t *debounceTimer3 = nullptr;

// ---------------- Volatile flags (ISR-safe) ----------------
volatile bool debounceActive1 = false;
volatile bool debounceActive2 = false;
volatile bool debounceActive3 = false;

volatile bool modeButtonEvent = false;
volatile bool resetButtonEvent = false;
volatile bool btn3DebouncedEvent = false; // set when BTN3 confirmed pressed (after debounce)

// ---------------- Application state ----------------
int mode = 0; // 0: All Off, 1: Alternate Blink, 2: All On, 3: PWM Fade
const char *modeNames[] = {"All Off", "Alternate Blink", "All On", "PWM Fade"};

// Melody
int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
const size_t melodyLen = sizeof(melody) / sizeof(melody[0]);
bool melodyPlaying = false;
size_t melodyIndex = 0;
unsigned long lastNoteMillis = 0;

// BTN3 press tracking (non-ISR)
bool waitingForBtn3Release = false;
unsigned long btn3PressStart = 0;

// LED toggle mode (activated by BTN3 short press)
bool ledToggleActive = false;
bool ledsStateOn = false;
unsigned long lastLedToggleMillis = 0;

// ---------------- ISR: debounce timers ----------------
void IRAM_ATTR onDebounceTimer1() {
  if (digitalRead(MODE_BTN) == LOW) modeButtonEvent = true;
  debounceActive1 = false;
}

void IRAM_ATTR onDebounceTimer2() {
  if (digitalRead(RESET_BTN) == LOW) resetButtonEvent = true;
  debounceActive2 = false;
}

void IRAM_ATTR onDebounceTimer3() {
  if (digitalRead(BTN3) == LOW) btn3DebouncedEvent = true;
  debounceActive3 = false;
}

// ---------------- ISR: button falling-edge handlers (start debounce) ----------------
void IRAM_ATTR onModeButtonISR() {
  if (!debounceActive1) {
    debounceActive1 = true;
    timerWrite(debounceTimer1, 0);
    timerAlarmWrite(debounceTimer1, DEBOUNCE_US, false);
    timerAlarmEnable(debounceTimer1);
  }
}

void IRAM_ATTR onResetButtonISR() {
  if (!debounceActive2) {
    debounceActive2 = true;
    timerWrite(debounceTimer2, 0);
    timerAlarmWrite(debounceTimer2, DEBOUNCE_US, false);
    timerAlarmEnable(debounceTimer2);
  }
}

void IRAM_ATTR onBtn3ISR() {
  if (!debounceActive3) {
    debounceActive3 = true;
    timerWrite(debounceTimer3, 0);
    timerAlarmWrite(debounceTimer3, DEBOUNCE_US, false);
    timerAlarmEnable(debounceTimer3);
  }
}

// ---------------- Helper: OLED ----------------
void showModeOnOLED(const String &msg) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 15);
  display.print(msg);
  display.setTextSize(1);
  display.setCursor(5, 50);
  display.print(melodyPlaying ? "Melody: ON" : "Melody: OFF");
  display.display();
  Serial.println(msg);
}

// ---------------- Helper: set all leds ----------------
void setAllLEDs(uint8_t v) {
  ledcWrite(PWM1, v);
  ledcWrite(PWM2, v);
  ledcWrite(PWM3, v);
}

// perform one quick alternate cycle (used previously; not used for continuous toggle)
void performAlternateOnce() {
  ledcWrite(PWM1, 255);
  ledcWrite(PWM2, 0);
  ledcWrite(PWM3, 0);
  delay(180);
  ledcWrite(PWM1, 0);
  ledcWrite(PWM2, 255);
  ledcWrite(PWM3, 0);
  delay(180);
  ledcWrite(PWM1, 0);
  ledcWrite(PWM2, 0);
  ledcWrite(PWM3, 255);
  delay(180);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found!");
    while (true) delay(10);
  }
  display.clearDisplay();
  display.display();

  // Pins
  pinMode(MODE_BTN, INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // PWM (LEDs) - channels for LEDs
  ledcSetup(PWM1, 5000, 8);
  ledcSetup(PWM2, 5000, 8);
  ledcSetup(PWM3, 5000, 8);
  ledcAttachPin(LED1, PWM1);
  ledcAttachPin(LED2, PWM2);
  ledcAttachPin(LED3, PWM3);

  // Buzzer channel for ledcWriteTone
  ledcSetup(PWM_BUZ, 2000, 8); // initial freq ignored by ledcWriteTone
  ledcAttachPin(BUZZER_PIN, PWM_BUZ);

  // Debounce timers (prescaler 80 → 1 µs tick)
  debounceTimer1 = timerBegin(0, 80, true);
  debounceTimer2 = timerBegin(1, 80, true);
  debounceTimer3 = timerBegin(2, 80, true);

  timerAttachInterrupt(debounceTimer1, &onDebounceTimer1, true);
  timerAttachInterrupt(debounceTimer2, &onDebounceTimer2, true);
  timerAttachInterrupt(debounceTimer3, &onDebounceTimer3, true);

  timerAlarmDisable(debounceTimer1);
  timerAlarmDisable(debounceTimer2);
  timerAlarmDisable(debounceTimer3);

  // Attach falling-edge ISRs to start debounce
  attachInterrupt(digitalPinToInterrupt(MODE_BTN), onModeButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(RESET_BTN), onResetButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN3), onBtn3ISR, FALLING);

  // Initial state
  setAllLEDs(0);
  showModeOnOLED("Ready");
}

// ---------------- Main loop ----------------
void loop() {
  unsigned long now = millis();

  // --- Handle mode button event (confirmed by debounce timer) ---
  if (modeButtonEvent) {
    modeButtonEvent = false;
    // pressing mode or reset cancels LED-toggle-mode
    ledToggleActive = false;
    melodyPlaying = melodyPlaying; // leave melody intact per your spec (only BTN3 short stops melody)
    mode = (mode + 1) % 4;
    showModeOnOLED(String("Mode: ") + modeNames[mode]);
  }

  // --- Handle reset button event ---
  if (resetButtonEvent) {
    resetButtonEvent = false;
    ledToggleActive = false; // stop LED toggle mode
    mode = 0;
    showModeOnOLED("Reset → Off");
  }

  // --- Handle BTN3 debounced event: start tracking press duration (non-blocking) ---
  if (btn3DebouncedEvent && !waitingForBtn3Release) {
    // confirmed falling edge after debounce: start measuring until release
    btn3DebouncedEvent = false;
    waitingForBtn3Release = true;
    btn3PressStart = millis();
    // do not decide yet; wait for release
  }

  // --- If waiting for BTN3 release, check release and take action ---
  if (waitingForBtn3Release) {
    if (digitalRead(BTN3) == HIGH) {
      // released: determine press length
      unsigned long pressDuration = millis() - btn3PressStart;
      waitingForBtn3Release = false;

      if (pressDuration >= LONGPRESS_MS) {
        // Long press -> start melody loop (plays forever until short press on BTN3)
        if (!melodyPlaying) {
          melodyPlaying = true;
          melodyIndex = 0;
          lastNoteMillis = 0;
          showModeOnOLED("Melody started");
          Serial.println("Melody started (long press)");
        } else {
          // if already playing (edge-case), keep playing
          Serial.println("Melody already playing");
        }
      } else {
        // Short press -> stop melody (if playing) and start LED toggling forever
        if (melodyPlaying) {
          melodyPlaying = false;
          ledcWriteTone(PWM_BUZ, 0); // stop buzzer
          Serial.println("Melody stopped by short press");
        }
        // Start LED toggle mode (continues until MODE or RESET pressed)
        ledToggleActive = true;
        ledsStateOn = false; // will toggle soon
        lastLedToggleMillis = millis();
        showModeOnOLED("LED Toggle Mode");
        Serial.println("LED Toggle Mode started (short press)");
      }
    }
    // else still pressed -> wait for release
  }

  // --- If ledToggleActive then perform toggling (overrides normal mode behavior) ---
  if (ledToggleActive) {
    if (now - lastLedToggleMillis >= LED_TOGGLE_MS) {
      ledsStateOn = !ledsStateOn;
      setAllLEDs(ledsStateOn ? 255 : 0);
      lastLedToggleMillis = now;
    }
    // while in LED toggle mode we skip the normal "mode" behavior
  } else if (!melodyPlaying) {
    // --- Normal mode behavior (only when not playing melody and not in led-toggle) ---
    static unsigned long lastAltStep = 0;
    static int altState = 0;

    switch (mode) {
      case 0: // All Off
        setAllLEDs(0);
        break;

      case 1: // Alternate Blink (continuous)
        if (now - lastAltStep >= 200) {
          altState = (altState + 1) % 3;
          ledcWrite(PWM1, (altState == 0) ? 255 : 0);
          ledcWrite(PWM2, (altState == 1) ? 255 : 0);
          ledcWrite(PWM3, (altState == 2) ? 255 : 0);
          lastAltStep = now;
        }
        break;

      case 2: // All On
        setAllLEDs(255);
        break;

      case 3: // PWM Fade
        static int brightness = 0;
        static int dir = 4;
        // step every 15 ms approximately
        static unsigned long lastFadeStep = 0;
        if (now - lastFadeStep >= 15) {
          brightness += dir;
          if (brightness <= 0) { brightness = 0; dir = -dir; }
          if (brightness >= 255) { brightness = 255; dir = -dir; }
          ledcWrite(PWM1, brightness);
          ledcWrite(PWM2, brightness);
          ledcWrite(PWM3, brightness);
          lastFadeStep = now;
        }
        break;
    }
  }
  // if melodyPlaying is true, we skip the normal mode behavior above

  // --- Melody playback when melodyPlaying is true ---
  if (melodyPlaying) {
    if (now - lastNoteMillis >= MELODY_NOTE_MS) {
      // play the next note
      int freq = melody[melodyIndex];
      ledcWriteTone(PWM_BUZ, freq); // start tone at specified frequency
      melodyIndex = (melodyIndex + 1) % melodyLen;
      lastNoteMillis = now;
    }
  } else {
    // ensure buzzer is off
    ledcWriteTone(PWM_BUZ, 0);
  }

  // Small yield
  delay(5);
}
