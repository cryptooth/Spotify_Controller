#include "ConfigManager.h"
#include "Globals.h"
#include "SpotifyManager.h"
#include "UIManager.h"
#include <Arduino.h>
#include <SPI.h>

void setup() {
  // HARDWARE SPI MAPPING (Critical for Display)
  // CLK=4, MOSI=6, CS=7
  SPI.begin(4, -1, 6, 7);

  Serial.begin(115200);
  Serial.println("Starting ESP32 Spotify Display...");

  // 1. Setup Display (Show logo or blank)
  setupUI();

  // 2. Setup Config (WiFi + Keys)
  setupConfig();

  // NTP Setup (Turkey GMT+3)
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // 3. Setup Inputs (Encoder) - ADDED THIS
  setupInputs();

  // 4. Setup Spotify (Auth)
  setupSpotify();

  Serial.println("Setup Complete.");
}

void loop() {
  static unsigned long lastAlive = 0;
  if (millis() - lastAlive > 2000) {
    Serial.println("[System] Loop is running...");
    lastAlive = millis();
  }

  // Input Handling
  handleInputs();

  // Power Saving / Screensaver
  if (currentMode == MODE_PLAYING && !isPlaying) {
    if (millis() - lastInputTime > (unsigned long)screensaverTimeout) {
      currentMode = MODE_CLOCK;
    }
  }

  // Wake on Play (Auto-Wake)
  if (currentMode == MODE_CLOCK && isPlaying) {
    currentMode = MODE_PLAYING;
    lastInputTime = millis();
  }

  // UI Loop (Needs to be fast, 60FPS)
  loopUI();

  // Spotify Loop (Polled every few seconds)
  loopSpotify();

  // Save Config if Token Rotated
  if (shouldSaveConfig) {
    saveConfig();
    shouldSaveConfig = false;
  }

  // Handle WiFi Manager Portal (if active)
  // currently blocking in setupConfig, avoiding complex non-blocking for now
}
