#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "OledDisplay.h"

extern OLEDDisplay oled;
extern bool wifi_connected;


WebServer server(80);

String ssid_saved = "";
String password_saved = "";
String topic_base = "";
String user_id = "";

Preferences preferences;
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 10000; // 10 secondes

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "OledDisplay.h"

extern OLEDDisplay oled;

WebServer server(80);
Preferences preferences;

String ssid_saved = "";
String password_saved = "";
String topic_base = "";
String user_id = "";

void handleRoot() {
    String html = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
          <meta charset="UTF-8">
          <title>Lock</title>
          <link rel="icon" href="data:,">
          <style>
            body {
              font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
              background-color: #f2f2f2;
              color: #333;
              display: flex;
              justify-content: center;
              align-items: center;
              height: 100vh;
              margin: 0;
            }
            .container {
              background-color: white;
              padding: 30px;
              border-radius: 12px;
              box-shadow: 0 0 20px rgba(0,0,0,0.1);
              width: 90%;
              max-width: 400px;
              animation: fadeInUp 0.6s ease-out;
            }
            h2 {
              text-align: center;
              color: #4CAF50;
            }
            input[type="text"], input[type="password"] {
              width: 100%;
              padding: 12px;
              margin: 8px 0;
              border: 1px solid #ccc;
              border-radius: 8px;
              box-sizing: border-box;
            }
            input[type="submit"] {
              width: 100%;
              background-color: #4CAF50;
              color: white;
              padding: 12px;
              border: none;
              border-radius: 8px;
              cursor: pointer;
              font-size: 16px;
            }
            input[type="submit"]:hover {
              background-color: #45a049;
            }
            @keyframes fadeInUp {
              from {
                opacity: 0;
                transform: translateY(20px);
              }
              to {
                opacity: 1;
                transform: translateY(0);
              }
            }
          </style>
        </head>
        <body>
          <div class="container">
            <h2>Configuration du Lock</h2>
            <form action='/save' method='POST'>
              <label for='ssid'>SSID WiFi :</label>
              <input type='text' id='ssid' name='ssid' required>
              <label for='password'>Mot de passe WiFi :</label>
              <input type='password' id='password' name='password' required>
              <label for='topic'>Topic de base MQTT :</label>
              <input type='text' id='topic' name='topic' required>
              <label for='user_id'>User ID :</label>
              <input type='text' id='user_id' name='user_id' required>
              <input type='submit' value='Enregistrer'>
            </form>
          </div>
        </body>
        </html>
    )rawliteral";
    server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password") &&
      server.hasArg("topic") && server.hasArg("user_id")) {

    String ssid = server.arg("ssid");
    String password = server.arg("password");
    topic_base = server.arg("topic");
    user_id = server.arg("user_id");

    preferences.begin("wifi_config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("topic", topic_base);
    preferences.putString("user_id", user_id);
    preferences.end();

    oled.displayText("Paramètres OK", 0, 0, 1);
    oled.displayText("Redémarrage...", 0, 16, 1, false);

    server.send(200, "text/html", "<h3>Enregistré. Redémarrage...</h3>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Paramètres manquants !");
  }
}

bool connectToSavedWiFi() {
  preferences.begin("wifi_config", true);
  ssid_saved = preferences.getString("ssid", "");
  password_saved = preferences.getString("password", "");
  topic_base = preferences.getString("topic", "");
  user_id = preferences.getString("user_id", "");
  preferences.end();

  if (ssid_saved == "" || password_saved == "") {
    return false;  // Demarrera l'AP
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_saved.c_str(), password_saved.c_str());

  Serial.print("🔌 Connexion WiFi");
  oled.displayText("Connexion WiFi", 0, 0, 1);
  oled.displayText("SSID: " + ssid_saved, 0, 16, 1, false);

  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Connecté au WiFi !");
      Serial.print("IP : ");
      Serial.println(WiFi.localIP());

      oled.displayText("WiFi Connecté", 0, 0, 1);
      oled.displayText("IP: " + WiFi.localIP().toString(), 0, 16, 1, false);
      oled.displayText("User: " + user_id, 0, 32, 1, false);
      wifi_connected = true;
      return true;
    }
    Serial.print(".");
    delay(500);
  }

  Serial.println("\n❌ Échec WiFi");
  oled.displayText("Echec WiFi", 0, 0, 1);
  wifi_connected = false;
  return true; // ATTENTION : on retourne true pour signaler que config est OK mais WiFi KO
}

void startAccessPoint() {
  WiFi.softAP("Home Lock", "123456789");
  IPAddress IP = WiFi.softAPIP();

  Serial.print("🌐 Point d'accès démarré à : ");
  Serial.println(IP);

  oled.displayText("Mode AP actif", 0, 0, 1);
  oled.displayText("SSID: Home Lock", 0, 16, 1, false);
  oled.displayText("IP: " + IP.toString(), 0, 32, 1, false);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void handleWiFiConfigPortal() {
  bool config_found = connectToSavedWiFi();
  
  if (!config_found) {
    // Pas de config => portail de config
    startAccessPoint();
    while (true) {
      server.handleClient();
      delay(10);
    }
  }

  // Si config présente mais Wi-Fi KO, on continue silencieusement
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
  } else {
    if (!wifi_connected) {
      // Si WiFi vient d'être reconnecté
      Serial.println("✅ Reconnecté au WiFi !");
      oled.displayText("WiFi OK", 0, 0, true);
      wifi_connected = true;

      // Reconnexion MQTT automatique
      if (mqttCtrl != nullptr) {
        mqttCtrl->begin(); // Redémarre MQTT proprement
      }
    }
  }
}

#endif
