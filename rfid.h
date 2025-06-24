#ifndef RFID_H
#define RFID_H

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <Preferences.h>
#include <vector>

class RFIDReader {
  private:
    MFRC522DriverPinSimple ssPin;
    MFRC522DriverSPI driver;
    MFRC522 mfrc522;
    String lastUID;
    unsigned long lastReadTime = 0;
    const unsigned long debounceDelay = 1000;
    std::vector<String> masterCards;

  public:
    RFIDReader(uint8_t ssPinNumber)
      : ssPin(ssPinNumber), driver(ssPin), mfrc522(driver) {}

    void begin() {
      mfrc522.PCD_Init();
      MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
      Serial.println(F("🔍 Prêt à scanner une carte RFID..."));
      loadMasterCards();
    }

    String checkCard() {
      if (!mfrc522.PICC_IsNewCardPresent()) {
        lastUID = "";
        return "";
      }

      if (!mfrc522.PICC_ReadCardSerial()) return "";

      unsigned long now = millis();
      if (now - lastReadTime < debounceDelay) return "";

      String uid = getUIDString();
      if (uid != lastUID) {
        lastUID = uid;
        lastReadTime = now;
        Serial.print("📇 UID détecté : ");
        Serial.println(uid);
        return uid;
      }

      return "";
    }

    String getUIDString() {
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
        if (i < mfrc522.uid.size - 1) uid += ":";
      }
      uid.toUpperCase();
      return uid;
    }

    bool isMasterCard(const String& uid) {
      for (const auto& master : masterCards) {
        if (uid == master) return true;
      }
      return false;
    }

    void addMasterCard(const String& uid) {
      if (!isMasterCard(uid)) {
        masterCards.push_back(uid);
        saveMasterCards();
        Serial.println("✅ Carte ajoutée : " + uid);
      }
    }

    void removeMasterCard(const String& uid) {
      for (auto it = masterCards.begin(); it != masterCards.end(); ++it) {
        if (*it == uid) {
          masterCards.erase(it);
          saveMasterCards();
          Serial.println("❌ Carte supprimée : " + uid);
          return;
        }
      }
    }

    void loadMasterCards() {
      Preferences prefs;
      prefs.begin("rfid", true);
      int count = prefs.getInt("count", 0);
      masterCards.clear();
      for (int i = 0; i < count; i++) {
        String key = "uid" + String(i);
        String val = prefs.getString(key.c_str(), "");
        if (val != "") masterCards.push_back(val);
      }
      prefs.end();
    }

    void saveMasterCards() {
      Preferences prefs;
      prefs.begin("rfid", false);
      prefs.clear();
      prefs.putInt("count", masterCards.size());
      for (int i = 0; i < masterCards.size(); i++) {
        String key = "uid" + String(i);
        prefs.putString(key.c_str(), masterCards[i]);
      }
      prefs.end();
    }
};

#endif
