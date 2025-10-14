#include <Arduino.h>

#define LED_PIN 2            
#define Freq 5000
#define res 8
#define chn 0

void setup(){
  ledcSetup(chn,Freq,res);
  ledcAttachPin(LED_PIN,chn);

}

void loop()
{
  for (int i = 0; i <= 255; i++)
  {
    ledcWrite(chn,i);
    delay(20);
  }
  
  for (int i = 255; i >= 0; i--)
  {
    ledcWrite(chn,i);
    delay(20);
  }
}