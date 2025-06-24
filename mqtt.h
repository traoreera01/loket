#ifndef MQTT_H
#define MQTT_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "OledDisplay.h"
#include "wifi_manager.h"  // pour wifi_connected

extern String user_id;
extern OLEDDisplay oled;
extern bool wifi_connected;
extern String topic_base;
extern void mqttCallback(char* topic, byte* payload, unsigned int length);

class MQTTController {
  private:
    const char* ssid;
    const char* password;
    const char* mqtt_server;
    int mqtt_port;
    const char* mqtt_user;
    const char* mqtt_password;

    WiFiClientSecure secureClient;
    PubSubClient client;

    String Commands;  // Défini dynamiquement après la récupération des préférences

    bool mqtt_connected = false;

  public:
    MQTTController(const char* ssid, const char* password,
                   const char* mqtt_server, int mqtt_port,
                   const char* mqtt_user, const char* mqtt_password)
      : ssid(ssid), password(password),
        mqtt_server(mqtt_server), mqtt_port(mqtt_port),
        mqtt_user(mqtt_user), mqtt_password(mqtt_password),
        client(secureClient) {}

    void begin() {
      connectToWiFi();  // Tente la connexion Wi-Fi + récupère topic_base et user_id

      if (!wifi_connected) {
        Serial.println("⏭️ MQTT ignoré (Wi-Fi non connecté)");
        return;
      }

      // Maintenant que user_id et topic_base sont récupérés :
      Commands = topic_base + "/" + user_id + "/cmd";

      client.setServer(mqtt_server, mqtt_port);
      client.setCallback(mqttCallback);

      connectMQTT();
    }

    void loop() {
      if (wifi_connected && client.connected()) {
        client.loop();
      } else if (wifi_connected && !client.connected()) {
        connectMQTT();
      }
    }

    void publish(const char* topic, const String& message) {
      if (wifi_connected && client.connected()) {
        client.publish(topic, message.c_str());
        Serial.println("✅ Message MQTT publié : " + message);
      } else {
        Serial.println("⚠️ MQTT non connecté. Message ignoré.");
      }
    }

  private:
    void connectMQTT() {
      if (!wifi_connected) return;
      secureClient.setInsecure();  // Ignorer les certificats

      while (!client.connected()) {
        Serial.print("🔄 Connexion MQTT...");
        oled.displayText("MQTT...", 0, 48, true);

        if (client.connect("ESPClient", mqtt_user, mqtt_password)) {
          Serial.println("✅ Connecté au broker MQTT !");
          oled.displayText("MQTT OK", 0, 48, true);

          client.subscribe(Commands.c_str());
          client.publish(Commands.c_str(), "0x10");  // exemple de premier message
          mqtt_connected = true;
        } else {
          Serial.print("❌ MQTT échoué, code = ");
          Serial.print(client.state());
          Serial.println(" - on continue sans MQTT");
          oled.displayText("MQTT echec", 0, 48, true);
          mqtt_connected = false;
          break;  // on arrête ici, pour éviter de bloquer le programme
        }
      }
    }
};

#endif
