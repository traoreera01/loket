#include "rfid.h"
#include "mqtt.h"
#include "OLEDDisplay.h"
#include "wifi_manager.h"

bool wifi_connected = false;

#define BUZZER_PIN      4
#define SS_PIN          5
#define LOCKET          3
#define LED_INDICATOR   2
#define RESET_BUTTON    12

MQTTController* mqttCtrl = nullptr;
OLEDDisplay oled;
RFIDReader rfid(SS_PIN);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("📩 MQTT Message reçu [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(message);

  if (String(topic) == "event/commande") {
    if (message == "ON" || message == "open") {
      digitalWrite(LOCKET, HIGH);
      Serial.println("🔓 Locket activée");
    } else if (message == "OFF") {
      digitalWrite(LOCKET, LOW);
      Serial.println("🔒 Locket désactivée");
    } else if (message == "reset") {
      Preferences preferences;
      preferences.begin("wifi_config", false);
      preferences.clear();
      preferences.end();
      delay(1000);
      ESP.restart();
    } else if (message == "status") {
      if (mqttCtrl != nullptr && wifi_connected) {
        mqttCtrl->publish("rfid/", "status OK");
      }
    }
  }
}

void pinsConf() {
  pinMode(LED_INDICATOR, OUTPUT);
  pinMode(RESET_BUTTON, INPUT_PULLDOWN);
  pinMode(LOCKET, OUTPUT);
  digitalWrite(LOCKET, LOW); // par défaut verrouillé
}

void handleResetButton(int buttonPin) {
  if (digitalRead(buttonPin) == HIGH) {
    oled.ToneRefused(BUZZER_PIN);
    oled.displayText("reset ...", 0, 0, true);
    digitalWrite(LED_INDICATOR, HIGH);
    Preferences preferences;
    preferences.begin("wifi_config", false);
    preferences.clear();
    preferences.end();
    delay(1000);
    digitalWrite(LED_INDICATOR, LOW);
    oled.ToneAccepted(BUZZER_PIN);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  pinsConf();

  oled.begin(25, 26); // exemple SDA/SCL
  oled.showRefused(BUZZER_PIN);

  // Gère Wi-Fi et portail si besoin
  handleWiFiConfigPortal();

  // Initialise MQTT seulement si le Wi-Fi est connecté
  if (wifi_connected) {
    mqttCtrl = new MQTTController(
      ssid_saved.c_str(),
      password_saved.c_str(),
      "2a94bbeb1f484944aea1327a5b2142bc.s1.eu.hivemq.cloud", // ton broker
      8883,
      "Admin10",
      "Admin123A"
    );
    mqttCtrl->begin();
  }

  rfid.begin();
}

void loop() {
  if (mqttCtrl != nullptr && wifi_connected) {
    mqttCtrl->loop();
  }

  handleResetButton(RESET_BUTTON);

  String uid = rfid.checkCard();
  if (uid != "") {
    Serial.println("🪪 Carte détectée : " + uid);
    if (mqttCtrl != nullptr && wifi_connected) {
      mqttCtrl->publish("rfid/uid", uid);
    }

    digitalWrite(LOCKET, HIGH);
    tone(BUZZER_PIN, 1000, 200);
    delay(2000);
    digitalWrite(LOCKET, LOW);
  }
  checkWiFiReconnect();
  delay(50);
}
