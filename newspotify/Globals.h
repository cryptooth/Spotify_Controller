#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <WebServer.h>

// ==========================================
// UTILS
// ==========================================
extern WebServer server;

// ==========================================
// DISPLAY & PINS (ESP32-S3 N16R8)
// ==========================================
// U8g2 Constructor will be defined in .cpp
extern U8G2_SSD1322_NHD_256X64_F_3W_HW_SPI u8g2;

// Pin Definitions
// Pin Definitions (ESP32-S3 N16R8)
// Matches dedoy_oled configuration
#define OLED_CLK 4
#define OLED_MOSI 6
#define OLED_CS 7
// OLED_DC Not used in 3-wire SPI (mapped to -1 or unused)
#define OLED_RST 2

#define ENC_CLK 1 // Updated from 18
#define ENC_DT 3  // Updated from 17
#define ENC_SW 5  // Updated from 16
#define BAT_PIN                                                                \
  4 // Shared with CLK? No, confirm this. BAT_PIN in dedoy_oled Globals.h is 0
    // for S3?
// Waiting, dedoy_oled Globals.h says BAT_PIN 0 for S3 (line 25).

// ==========================================
// SPOTIFY CONFIGURATION (RELAY FLOW)
// ==========================================
// We no longer store Client ID/Secret.
// We only need the Relay URL and our unique deviceId.
extern char relay_base_url[100];
extern char device_id[32]; // Generated from MAC

// The tokens we receive from Relay
extern char spotify_refresh_token[512]; // Can be long
extern char spotify_access_token[512];  // Access tokens are even longer

// ==========================================
// PLAYBACK STATE
// ==========================================
extern String currentArtist;
extern String currentSong;
extern String currentAlbum;
extern int currentDuration; // in milliseconds
extern int currentProgress; // in milliseconds
extern bool isPlaying;

// Interpolation / Dead Reckoning
extern unsigned long lastServerUpdateMillis; // Time when we received data
extern long lastServerProgress;              // Value received from server

// ==========================================
// SYSTEM STATE
// ==========================================
extern bool shouldSaveConfig;
extern int batteryPercent;
extern unsigned long lastInputTime;

// Configurable Settings
extern char artist_size[10];
extern char song_size[10];
extern int screensaverTimeout;
extern int currentBrightness;

// Menu State
extern int menuIndex;
extern int menuLen;
extern const char *menuItems[];

// UI Modes
enum AppMode { MODE_CLOCK, MODE_PLAYING, MODE_MENU, MODE_INFO, MODE_CONFIG };
extern AppMode currentMode;

// Inputs
void setupInputs();
void handleInputs();

#endif
