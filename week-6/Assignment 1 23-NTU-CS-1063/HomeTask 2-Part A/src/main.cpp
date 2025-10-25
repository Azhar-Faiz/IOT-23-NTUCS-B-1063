#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define LED1 17
#define LED2 18
#define LED3 19
#define MODE_BTN 25
#define RESET_BTN 26
#define PWM_LED3 0                      //channel 0
#define DEBOUNCE_US 50000                // 50ms

hw_timer_t *debounceTimerM = nullptr;
hw_timer_t *debounceTimerR = nullptr;
volatile bool debounceM_Active = false;
volatile bool debounceR_Active = false;
volatile bool m_Event = false;
volatile bool r_Event = false;

int mode = 0;
unsigned long lastAltStep = 0;          //previous alternate
int altState = 0;                       //current alternate

void IRAM_ATTR onM_Debounce()                       // when Mode btn(btn 1) is pressed  
{                                                   // for longer than debounce time
  if (digitalRead(MODE_BTN) == LOW) m_Event = true;
  debounceM_Active = false;
}
void IRAM_ATTR onR_Debounce()                       // when Reset btn(btn 2) is pressed 
{                                                   // for longer than debounce time
  if (digitalRead(RESET_BTN) == LOW) r_Event = true;
  debounceR_Active = false;
}
void IRAM_ATTR modeISR()                            //Interrupt called when btn1 is pressed
{
  if (!debounceM_Active)
{
  debounceM_Active = true;
  timerWrite(debounceTimerM, 0);
  timerAlarmWrite(debounceTimerM, DEBOUNCE_US, false);
  timerAlarmEnable(debounceTimerM);
}
}
void IRAM_ATTR resetISR() 
{                         //Interrupt called when btn2 is pressed
  if (!debounceR_Active) 
    {
        debounceR_Active = true;
        timerWrite(debounceTimerR, 0);
        timerAlarmWrite(debounceTimerR, DEBOUNCE_US, false);
        timerAlarmEnable(debounceTimerR);
    }
}

void setAllOff() 
{                                    //All leds become OFF
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    ledcWrite(PWM_LED3, 0);             //PWM is attached to led3 that is why this syntax used
}
void setAllOn() 
{
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);           //All led ON
    ledcWrite(PWM_LED3, 255);
}
void alternateStep() 
{
    digitalWrite(LED1, altState ? HIGH : LOW);
    digitalWrite(LED2, altState ? LOW : HIGH);      //ALternate the LED1 and LED2
    ledcWrite(PWM_LED3, 0);
altState = !altState;
}
String currentLabel() 
{
    switch (mode) 
    {
        case 0: return "ALL OFF";
        case 1: return "ALTERNATE";             //Cases to retun msg to show msg on OLED
        case 2: return "ALL ON";
        case 3: return "PWM FADE";
        default: return "UNKNOWN";
    }
}
void drawOLED() 
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);            //Displays msg on OLED
    display.setCursor(0, 0);
    display.print("State:\n" + currentLabel());
    display.display();
}

// ---- Setup ----
void setup() 
{
    Serial.begin(115200);
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(MODE_BTN, INPUT_PULLUP);                //modes of pins
    pinMode(RESET_BTN, INPUT_PULLUP);

    ledcSetup(PWM_LED3, 5000, 8);                   //attaching channel, frequency and resolution
    ledcAttachPin(LED3, PWM_LED3);                  //attaching led3 pin and pwm channel

    debounceTimerM = timerBegin(0, 80, true);       //timer start for debouncing for each btn
    debounceTimerR = timerBegin(1, 80, true);
    timerAttachInterrupt(debounceTimerM, &onM_Debounce, true);  //attach interrupt to timer
    timerAttachInterrupt(debounceTimerR, &onR_Debounce, true);

    attachInterrupt(digitalPinToInterrupt(MODE_BTN), modeISR, FALLING);   //when Interrupt occurs and where
    attachInterrupt(digitalPinToInterrupt(RESET_BTN), resetISR, FALLING);

    setAllOff();        //ALL LEDs are OFF state
    drawOLED();         //draws on oled OFF state
}

void loop() 
{
    unsigned long now = millis();

    if (m_Event) 
    {
        m_Event = false;
        mode = (mode + 1) % 4;              //change of Modes when btn1 pressed
        drawOLED();
    }

    if (r_Event) 
    {
        r_Event = false;                    //GO to OFF mode when btn2 pressed
        mode = 0;
        setAllOff();
        drawOLED();
    }

    switch (mode) 
    {
        case 0: setAllOff(); break;
        case 1:
            if (now - lastAltStep >= 200)       //time in ms for alternating.(alternates after 200ms)
            {
                alternateStep();
                lastAltStep = now;
            }
            break;
        case 2: setAllOn(); break;              //calls func for all on 
        case 3:
            digitalWrite(LED1, LOW);
            digitalWrite(LED2, LOW);
            for (int v = 0; v <= 255; ++v)          //PWM for LED3,oter leds are off
            {
                if (m_Event || r_Event) break;
                ledcWrite(PWM_LED3, v);
                delay(2);
            }
            for (int v = 255; v >= 0; --v) 
            {
                if (m_Event || r_Event) break;
                ledcWrite(PWM_LED3, v);
                delay(2);
             }
    break;
    }

    delay(5);
}