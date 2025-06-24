#ifndef RFID_H
#define RFID_H

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>


class RFIDReader {
  private:
    MFRC522DriverPinSimple ssPin;
    MFRC522DriverSPI driver;
    MFRC522 mfrc522;
    String lastUID;

  public:
    RFIDReader(uint8_t ssPinNumber): ssPin(ssPinNumber), driver(ssPin), mfrc522(driver) {}
   
    void begin() {
      Serial.begin(115200);
      mfrc522.PCD_Init();
      MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
      Serial.println(F("🔍 Scan une carte RFID"));
    }

    String checkCard() {
      if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return "";

      String uid = getUIDString();
      if (uid != lastUID) { // éviter les répétitions
        lastUID = uid;
        Serial.print("📇 UID détecté : ");
        Serial.println(uid);
        delay(500);  // évite lectures rapides
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

};

#endif
