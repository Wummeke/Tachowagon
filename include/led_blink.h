#include <Arduino.h>


// void ledblink(){

//   //LED blinking sequence
//   unsigned long currentMillis = millis();

//   // LED1
//   if (blinking){
//     if((ledState1 == HIGH) && (currentMillis - blinkMillis1 >= OnTime))
//     {
//       ledState1 = LOW;  // Turn it off
//       blinkMillis1 = currentMillis;  // Remember the time
//       digitalWrite(LED1, ledState1);  // Update the actual LED
//     }
//     else if ((ledState1 == LOW) && (currentMillis - blinkMillis1 >= interval1))
//     {
//       ledState1 = HIGH;  // turn it on
//       blinkMillis1 = currentMillis;   // Remember the time
//       digitalWrite(LED1, ledState1);	  // Update the actual LED
//     }

//   // LED2
//       if((ledState2 == HIGH) && (currentMillis - blinkMillis2 >= OnTime))
//     {
//       ledState2 = LOW;  // Turn it off
//       blinkMillis2 = currentMillis;  // Remember the time
//       digitalWrite(LED2, ledState2);  // Update the actual LED
//     }
//     else if ((ledState2 == LOW) && (currentMillis - blinkMillis2 >= interval2))
//     {
//       ledState2 = HIGH;  // turn it on
//       blinkMillis2 = currentMillis;   // Remember the time
//       digitalWrite(LED2, ledState2);	  // Update the actual LED
//     }
//   }
//   else{
//     digitalWrite(LED1, LOW);
//     digitalWrite(LED2, LOW);
//   }

// }

void init_leds(){
  for (byte i = 0; i < sizeof(leds); i++)
  {
    pinMode(leds[i], OUTPUT);
  }
  currentLed = 0;

  if(!blinking){
    for (byte i = 0; i < sizeof(leds); i++)
    {
      digitalWrite(leds[i], LOW);
    }      
  }
}

void flash(byte ledIndex)
{
    digitalWrite(leds[ledIndex], HIGH);
    delay(FLASH_DELAY);
    digitalWrite(leds[ledIndex], LOW);
    delay(FLASH_DELAY);
}

void ledblink(){
  currentLed++;
    currentLed %= sizeof(leds);
    for(byte i=0; i<FLASH_TIMES; i++) {
        flash(currentLed);
    }
}