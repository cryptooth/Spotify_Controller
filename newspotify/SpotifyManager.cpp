#include "SpotifyManager.h"
#include "Globals.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>

unsigned long lastPollTime = 0;
const int pollInterval = 3000; // Poll every 3 seconds

unsigned long tokenExpireTime = 0;

// Helper for POST requests
void sendSpotifyCommand(const char *endpoint, const char *method = "POST") {
  if (strlen(spotify_access_token) < 10)
    return;

  // We need a new client/http instance for commands to avoid conflict with
  // polling? Actually polling is blocking main loop, so this is fine as long as
  // we aren't in a thread.
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String("https://api.spotify.com/v1/me/player/") + endpoint;

  if (http.begin(client, url)) {
    http.addHeader("Authorization", String("Bearer ") + spotify_access_token);
    http.addHeader("Content-Length", "0"); // Fix 411 Length Required

    // PUT for play/pause, POST for next/prev
    // Note: Play/Pause takes device_id optionally but we let Spotify decide
    // active device
    int httpCode = http.sendRequest(method, "");
    Serial.printf("[SPT] Command %s: %d\n", endpoint, httpCode);

    if (httpCode == 401) {
      tokenExpireTime = 0; // Force refresh
    }
    http.end();
  }
}

void playPause() {
  if (isPlaying) {
    sendSpotifyCommand("pause", "PUT");
    isPlaying = false; // Optimistic update
  } else {
    sendSpotifyCommand("play", "PUT");
    isPlaying = true;
  }
  // Force immediate poll soon
  lastPollTime = millis() - pollInterval + 500;
}

void nextTrack() {
  sendSpotifyCommand("next", "POST");
  lastPollTime = millis() - pollInterval + 1000;
}

void prevTrack() {
  sendSpotifyCommand("previous", "POST");
  lastPollTime = millis() - pollInterval + 1000;
}

void setupSpotify() {
  // Initial Token Fetch if we have credits
  if (strlen(spotify_refresh_token) > 5) {
    refreshSpotifyToken();
  }
}

bool refreshSpotifyToken() {
  if (strlen(spotify_refresh_token) < 5) {
    Serial.println("[Spotify] Missing Token. Skipping refresh.");
    return false;
  }

  Serial.println("[Spotify] Refreshing Token via Relay...");

  WiFiClientSecure client;
  client.setInsecure(); // Skip cert check
  HTTPClient http;

  // Use Relay Endpoint
  String url = String(relay_base_url) + "/refresh.php";

  if (http.begin(client, url)) {
    http.addHeader("Content-Type", "application/json");

    // Send Refresh Token to our Relay
    // Relay holds the Client Secret and does the exchange for us.
    DynamicJsonDocument payloadDoc(1024);
    payloadDoc["refresh_token"] = spotify_refresh_token;
    String payload;
    serializeJson(payloadDoc, payload);

    int httpCode = http.POST(payload);

    if (httpCode == 200) {
      String response = http.getString();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, response);

      if (doc.containsKey("access_token")) {
        const char *newToken = doc["access_token"];
        int expiresIn = doc["expires_in"];

        strncpy(spotify_access_token, newToken, 510);
        tokenExpireTime = millis() + (expiresIn * 1000) - 60000;

        // Important: If Relay sends back a NEW refresh token (rotation), save
        // it.
        if (doc.containsKey("refresh_token")) {
          const char *newRefresh = doc["refresh_token"];
          strncpy(spotify_refresh_token, newRefresh, 510);
          spotify_refresh_token[510] = 0;
          shouldSaveConfig = true;
          Serial.println("[Spotify] Refresh Token Rotated!");
        }

        Serial.println("[Spotify] Token Refreshed!");
        http.end();
        return true;
      }
    } else {
      Serial.printf("[Spotify] Refresh Failed: %d\n", httpCode);
      Serial.println(http.getString());
    }
    http.end();
  }
  return false;
}

void getSpotifyCurrentData() {
  // Check if we need token refresh
  if (millis() > tokenExpireTime) {
    if (!refreshSpotifyToken())
      return;
  }

  if (strlen(spotify_access_token) < 10)
    return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = "https://api.spotify.com/v1/me/player/currently-playing";

  if (http.begin(client, url)) {
    http.addHeader("Authorization", "Bearer " + String(spotify_access_token));

    int httpCode = http.GET();
    if (httpCode == 200) {
      String response = http.getString();
      // Serial.println(response); // Debug

      DynamicJsonDocument doc(4096); // Spotify JSON can be large
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        isPlaying = doc["is_playing"];
        currentProgress = doc["progress_ms"];

        JsonObject item = doc["item"];
        if (item) {
          currentSong = item["name"].as<String>();
          currentDuration = item["duration_ms"];

          JsonArray artists = item["artists"];
          if (artists.size() > 0) {
            currentArtist = artists[0]["name"].as<String>();
          }

          JsonObject album = item["album"];
          if (album) {
            currentAlbum = album["name"].as<String>();
          }
        }

        // Dead Reckoning Sync
        // Only sync if significant drift or new track
        long newProgress = currentProgress;
        if (abs(newProgress - lastServerProgress) > 1000 ||
            newProgress < lastServerProgress) {
          lastServerProgress = newProgress;
          lastServerUpdateMillis = millis();
        }
      }
    } else if (httpCode == 204) {
      // Nothing playing
      isPlaying = false;
      currentSong = "Paused / Idle";
    } else {
      Serial.printf("[Spotify] API Error: %d\n", httpCode);
      // 401 = Unauthorized -> Maybe token revoked?
      if (httpCode == 401) {
        tokenExpireTime = 0; // Force refresh next time
      }
    }
    http.end();
  }
}

void loopSpotify() {
  if (millis() - lastPollTime > pollInterval) {
    getSpotifyCurrentData();
    lastPollTime = millis();
  }
}
