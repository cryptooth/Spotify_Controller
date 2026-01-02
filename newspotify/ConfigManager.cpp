#include "ConfigManager.h"
#include "Globals.h"
#include "UIManager.h"
#include <ArduinoJson.h>
#include <FFat.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiManager.h>

#define FILESYSTEM FFat

void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void loadConfig() {
  if (FILESYSTEM.begin(true)) {
    Serial.println("mounted file system");
    if (FILESYSTEM.exists("/config.json")) {
      Serial.println("reading config file");
      File configFile = FILESYSTEM.open("/config.json", "r");
      if (configFile) {
        DynamicJsonDocument json(1024);
        DeserializationError error = deserializeJson(json, configFile);
        configFile.close();

        if (!error) {
          Serial.println("\nparsed json");
          if (json.containsKey("spotify_refresh_token")) {
            strncpy(spotify_refresh_token, json["spotify_refresh_token"], 510);
            spotify_refresh_token[510] = 0;
          }
          if (json.containsKey("artist_size"))
            strlcpy(artist_size, json["artist_size"], sizeof(artist_size));
          if (json.containsKey("song_size"))
            strlcpy(song_size, json["song_size"], sizeof(song_size));
          if (json.containsKey("screensaverTimeout"))
            screensaverTimeout = json["screensaverTimeout"];
          if (json.containsKey("currentBrightness"))
            currentBrightness = json["currentBrightness"];
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

void saveConfig() {
  Serial.println("saving config");
  DynamicJsonDocument json(1024);
  json["spotify_refresh_token"] = spotify_refresh_token;
  json["artist_size"] = artist_size;
  json["song_size"] = song_size;
  json["screensaverTimeout"] = screensaverTimeout;
  json["currentBrightness"] = currentBrightness;

  File configFile = FILESYSTEM.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }
  serializeJson(json, configFile);
  configFile.close();
}

void resetConfig() {
  Serial.println("!!! Factory Reset Initiated !!!");
  if (FILESYSTEM.begin(true)) {
    if (FILESYSTEM.exists("/config.json")) {
      FILESYSTEM.remove("/config.json");
      Serial.println("Config file deleted.");
    }
  }

  // Clear RAM
  memset(spotify_refresh_token, 0, sizeof(spotify_refresh_token));
  memset(spotify_access_token, 0, sizeof(spotify_access_token));

  // UI Feedback
  drawStatus("FACTORY RESET", "Please Wait...");
  delay(1000);

  ESP.restart();
}

void pollRelayForToken() {
  HTTPClient http;
  String pollUrl =
      String(relay_base_url) + "/poll.php?device_id=" + String(device_id);

  while (true) {
    // 1. Draw QR Code
    String loginUrl =
        String(relay_base_url) + "/login.php?device_id=" + String(device_id);
    drawQR(loginUrl);

    // 2. Poll Server
    if (http.begin(pollUrl)) {
      int httpCode = http.GET();
      if (httpCode == 200) {
        String response = http.getString();
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, response);

        if (doc.containsKey("refresh_token")) {
          // SUCCESS!
          const char *rt = doc["refresh_token"];
          strncpy(spotify_refresh_token, rt, 510);
          spotify_refresh_token[510] = 0;
          saveConfig();

          drawStatus("SUCCESS!", "Rebooting...");
          delay(2000);
          ESP.restart();
        }
      }
      http.end();
    }

    // Poll every 3 seconds
    delay(3000);
  }
}

void setupConfig() {
  // 1. Generate Device ID from MAC Address
  uint64_t chipid = ESP.getEfuseMac();
  snprintf(device_id, 32, "%04X%08X", (uint16_t)(chipid >> 32),
           (uint32_t)chipid);
  Serial.print("Device ID: ");
  Serial.println(device_id);

  loadConfig();

  // Apply loaded brightness immediately
  u8g2.setContrast(currentBrightness);

  // 2. Connect WiFi
  WiFiManager wifiManager;
  drawStatus("CONNECTING...", "Please Wait");
  bool connected = wifiManager.autoConnect("ESP32-Spotify", "password");

  if (!connected) {
    Serial.println("Failed to connect to WiFi. Restarting...");
    ESP.restart();
  }

  // 3. Check Protocol
  if (strlen(spotify_refresh_token) < 10) {
    // No Token -> Enter Pairing Mode
    pollRelayForToken();
  }

  Serial.println("Config Valid. Proceeding...");
}
