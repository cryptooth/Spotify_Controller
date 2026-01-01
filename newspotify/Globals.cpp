#include "Globals.h"

// Initialize Display (HW SPI for performance, matches dedoy_oled)
U8G2_SSD1322_NHD_256X64_F_3W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_RST);

WebServer server(80);

// Spotify Config
char relay_base_url[100] = "https://sptfy.belekonline.com";
char device_id[32] = "";

char spotify_refresh_token[512] = "";
char spotify_access_token[512] = "";

// Playback State
String currentArtist = "Spotify";
String currentSong = "Waiting...";
String currentAlbum = "";
int currentDuration = 0;
int currentProgress = 0;
bool isPlaying = false;

// Interpolation
unsigned long lastServerUpdateMillis = 0;
long lastServerProgress = -1;

// System
bool shouldSaveConfig = false;
int batteryPercent = 100;
AppMode currentMode = MODE_PLAYING;

// Settings Defaults
// Settings Defaults
char artist_size[10] = "medium";
char song_size[10] = "small";
int screensaverTimeout = 15000; // 15s default
int currentBrightness = 255;
unsigned long lastInputTime = 0;

// Menu Definition
const char *menuItems[] = {"Artist Font",   "Song Font", "Brightness",
                           "Screensaver",   "Info",      "Reboot",
                           "Factory Reset", "Exit"};
int menuLen = 8;
int menuIndex = 0;
