#ifndef OLEDDISPLAY
#define OLEDDISPLAY

#include "ColorFormat.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "img_lock.h"
#include "img_unlock.h"
#include "reset_icon.h"


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class OLEDDisplay {
  private:
    Adafruit_SSD1306* display;
  public:
    OLEDDisplay() {
      display = nullptr;
    }
    void ToneRefused(uint8_t BUZZER_PIN) {
      for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 500, 200);
        delay(250);
      }
      noTone(BUZZER_PIN);
    }
    void ToneAccepted(uint8_t BUZZER_PIN) {
      tone(BUZZER_PIN, 2000, 150);
      delay(200);
      tone(BUZZER_PIN, 2500, 100);
      delay(150);
      noTone(BUZZER_PIN);
    }
    void ToneReset(uint8_t BUZZER_PIN) {
      tone(BUZZER_PIN, 500, 100);   // 500 Hz pendant 100 ms
      delay(150);
      tone(BUZZER_PIN, 300, 100);   // 300 Hz pendant 100 ms
      delay(150);
      tone(BUZZER_PIN, 700, 100);   // 700 Hz pendant 100 ms
      delay(200);
      noTone(BUZZER_PIN);
    }
    void begin(uint8_t SDA_PIN, uint8_t SCL_PIN) {
      Wire.begin(SDA_PIN, SCL_PIN);
      display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
      if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true);
      }
    }
    void showRefused(uint8_t BUZZER_PIN) {
      display->clearDisplay();
      for (int i = 0; i < 23; i++) {
        display->clearDisplay();
        display->drawBitmap(39, 7, lock_animationallArray[i], 50, 50, WHITE);
        display->display();
        delay(50);
      }
      ToneRefused(BUZZER_PIN);  // 🔴 Son de refus après animation
    }
    void displayLock(){
      display -> clearDisplay();
      display-> drawBitmap(39, 7, lock_animationallArray[1], 50, 50, WHITE);
      display->display();
    }

    void displayUnLock(){
      display -> clearDisplay();
      display -> drawBitmap(39, 7, unlock_animationallArray[1], 50, 50, WHITE);
      display->display();
    }


    void showAccepted(uint8_t BUZZER_PIN) {
      for (int i = 0; i < 23; i++) {
        display->clearDisplay();
        display->drawBitmap(39, 7, unlock_animationallArray[i], 50, 50, WHITE);
        display->display();
        delay(50);
      }
      ToneAccepted(BUZZER_PIN);  //  Son d'acceptation après animation
    }
    void showReset(uint8_t BUZZER_PIN) {
      display -> clearDisplay();
      display -> drawBitmap(39, 7, reset_icon, 50, 50, WHITE);
      delay(50);
      ToneReset(BUZZER_PIN);
    }
    void displayText(const String& text, int x = 0, int y = 0, int size = 1, bool clear = true) {
      if (clear) {
        display->clearDisplay();
      }
      display->setTextSize(size);
      display->setTextColor(WHITE);
      display->setCursor(x, y);
      display->println(text);
      display->display();
    }
};
#endif