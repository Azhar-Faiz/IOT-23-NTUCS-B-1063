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

int melody[] = {
  440, 440, 440, 349, 523, 440, 349, 523, 440,            //STAR WARS intro melody 
  659, 659, 659, 698, 523, 415, 349, 523, 440,
  880, 440, 440, 880, 830, 784, 740, 698, 740
};

bool Playing = false;         // is melody Playing?

void updateDisplay(const char *msg) {   // Method for Writing on Screen 
display.clearDisplay();
display.setTextSize(2);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0,0);
display.print(msg);                     //msg is either "Ready","Melody","LED"
display.display();
}

void play() {                 //Play the Melody
  static int i = 0;           //Only runs when this function runs the first time
  ledcWriteTone(0, melody[i]);
  delay(700);                   //delay of 700ms between each tone
  i = (i + 1) % (sizeof(melody) / sizeof(melody[0]));       //Size of array containing all the melody tones 
}

void stop() {               //Stops the melody 
  ledcWriteTone(0, 0);
  Playing = false;
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
  if (digitalRead(BTN) == LOW) {                  //This part checks for time wether <1.5s or not
  unsigned long startTime = millis();
  while (digitalRead(BTN) == LOW)
  delay(10);
  unsigned long pressTime = millis() - startTime;

  if (pressTime >= LONG_PRESS) {              //Time of press >= 1.5s
    Playing = true;
    updateDisplay("\nMelody");
    while (Playing) {
      play();                                        
      if (digitalRead(BTN) == LOW) {                    // Check for short press to stop 
        unsigned long shortStart = millis();
        while (digitalRead(BTN) == LOW) delay(10);            //Hold of Button is <1.5s or not
        unsigned long shortPressTime = millis() - shortStart;
        if (shortPressTime < LONG_PRESS) {
          stop();
          updateDisplay("Melody\nStopped");
          break;
        }
      }
    }
  } else {
    if (Playing) {            // Short press toggles LEDs or stops melody if playing
      stop();
      updateDisplay("Melody\nStopped");
    } else {
      static bool ledState = false;             //only runs 1st time this part of code runs
      ledState = !ledState;
      digitalWrite(LED_1, ledState);
      digitalWrite(LED_2, ledState);
      digitalWrite(LED_3, ledState);
      updateDisplay("\nLED");
    }
  }
  }
}