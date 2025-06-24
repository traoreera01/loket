#include "rfid.h"
#include "mqtt.h"
#include "OLEDDisplay.h"
#include "wifi_manager.h"

MQTTController* mqttCtrl;
OLEDDisplay oled;



#define LED_INDICATOR   2
#define LOCKET          3
#define BUZZER_PIN      4
#define SS_PIN          5
#define RESET_BUTTON    12


unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 50000; // 50 secondes

extern String user_id;
extern String topic_base;
extern bool wifi_connected;




RFIDReader rfid(SS_PIN);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  const String Topics = topic_base + "/" + user_id;
  const String Commands = topic_base + "/" + user_id + "cmd";

  Serial.print("📩 MQTT Message reçu [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(message);

  if (String(topic) == Commands.c_str()) {
    if (message == "0x01") {
      digitalWrite(LOCKET, HIGH);
      oled.showAccepted(BUZZER_PIN);
    } else if (message == "0x00") {
      digitalWrite(LOCKET, LOW);
      oled.showRefused(BUZZER_PIN);
    } else if (message == "0x3307") {
      Preferences preferences;
      preferences.begin("wifi_config", false);
      preferences.clear();
      preferences.end();
      oled.showReset(BUZZER_PIN);
      delay(1000);
      ESP.restart();
    } else if (message.startsWith("ADD:")) {
      String uidToAdd = message.substring(4);
      rfid.addMasterCard(uidToAdd);
      oled.showAccepted(BUZZER_PIN);
    } else if (message.startsWith("DEL:")) {
      String uidToRemove = message.substring(4);
      rfid.removeMasterCard(uidToRemove);
      oled.showRefused(BUZZER_PIN);
    } else if (message == "0x11") {
      mqttCtrl->publish(Commands.c_str(),"online");
    }
  }
}

void checkWiFiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt > reconnectInterval) {
      Serial.println("🔄 Tentative de reconnexion WiFi...");
      oled.displayText("Reconnexion WiFi...", 0, 0, true);
      WiFi.begin(ssid_saved.c_str(), password_saved.c_str());
      lastReconnectAttempt = millis();
    }
    wifi_connected = false;
    oled.displayLock();
  } else {
    if (!wifi_connected) {
      // Si WiFi vient d'être reconnecté
      Serial.println("✅ Reconnecté au WiFi !");
      oled.displayText("WiFi OK", 0, 0, true);
      wifi_connected = true;
      oled.displayLock();
      // Reconnexion MQTT automatique
      if (mqttCtrl != nullptr) {
        mqttCtrl->begin(); // Redémarre MQTT proprement
      }
    }
  }
}


void pinsConf() {
  pinMode(LED_INDICATOR, OUTPUT);
  pinMode(RESET_BUTTON, INPUT);
  pinMode(LOCKET, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);
  digitalWrite(LOCKET, LOW);
}


void handleResetButton(int buttonPin) {
  if (digitalRead(buttonPin) == HIGH) {
    oled.showReset(BUZZER_PIN);
    digitalWrite(LED_INDICATOR, HIGH);
    Preferences preferences;
    preferences.begin("wifi_config", false);
    preferences.clear();
    preferences.end();
    delay(1000);
    oled.ToneAccepted(BUZZER_PIN);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  pinsConf();
  oled.begin(25, 26);  // SDA, SCL
  handleWiFiConfigPortal();

  mqttCtrl = new MQTTController(
    ssid_saved.c_str(),
    password_saved.c_str(),
    "2a94bbeb1f484944aea1327a5b2142bc.s1.eu.hivemq.cloud",
    8883,
    "Admin10",
    "Admin123A"
  );
  mqttCtrl->begin();

  rfid.begin();
  oled.showRefused(BUZZER_PIN);
}

void loop() {
  const String Topics = topic_base + "/" + user_id + "/0x3306";

  if (mqttCtrl != nullptr && wifi_connected) {
    mqttCtrl->loop();
  }

  handleResetButton(RESET_BUTTON);
  String uid = rfid.checkCard();
  if (uid != "") {
    if (rfid.isMasterCard(uid)) {
      Serial.println("🔓 Carte passe-partout détectée !");
      digitalWrite(LOCKET, HIGH);
      oled.showAccepted(BUZZER_PIN);
      delay(3000);
      digitalWrite(LOCKET, LOW);
    }
    else if(mqttCtrl != nullptr && wifi_connected){
      mqttCtrl->publish(Topics.c_str(), uid);
    }
  }
  checkWiFiReconnect();
  delay(10);
}
