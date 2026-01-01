#include "UIManager.h"
#include "Globals.h"
#include "qrcode.h"
#include <WiFi.h>
#include <time.h>

unsigned long lastFrameTime = 0;

// Scrolling State
int scrollX_Artist = 0;
int scrollX_Song = 0;

// Fonts - Helpers
const uint8_t *getFont(const char *size, const char *weight) {
  String s = String(size);
  String w = String(weight);

  // Basic Logic
  if (s == "small")
    return u8g2_font_helvR10_tr;
  if (s == "large")
    return u8g2_font_helvB14_tr;
  return u8g2_font_helvB12_tr; // Medium/Default
}

String msToTime(long ms) {
  int totalSeconds = ms / 1000;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  char buf[8];
  sprintf(buf, "%d:%02d", minutes, seconds);
  return String(buf);
}

void drawQR(String url) {
  // Kept for Setup Mode
  u8g2.clearBuffer();
  int qrVersion = 5;
  int scale = 1;

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];
  qrcode_initText(&qrcode, qrcodeData, qrVersion, 0, url.c_str());

  int size = qrcode.size * scale;
  int startX = 20;
  int startY = (64 - size) / 2;

  // White Box
  u8g2.setDrawColor(1);
  u8g2.drawBox(startX - 2, startY - 2, size + 4, size + 4);

  u8g2.setDrawColor(0);
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        u8g2.drawBox(startX + x, startY + y, scale, scale);
      }
    }
  }

  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(70, 20, "SCAN ME");
  u8g2.setFont(u8g2_font_5x7_tr); // Tiny font
  u8g2.drawStr(70, 35, "Spotify Login");

  u8g2.sendBuffer();
}

void drawNowPlaying() {
  int W = 256;

  // 0. Battery Icon (Top Right)
  // Simple Mock or Real?
  // User asked for battery symbol. BAT_PIN is defined.
  // We need to read it? Or is it updated in loop?
  int batX = W - 20;
  u8g2.drawFrame(batX, 2, 16, 8);
  u8g2.drawBox(batX + 16, 4, 2, 4); // Nipple
  int fill = map(batteryPercent, 0, 100, 0, 14);
  if (fill > 0)
    u8g2.drawBox(batX + 1, 3, fill, 6);

  // 1. Bitrate / Codec (Top Left of Battery)
  // Removed "SPOTIFY" text as per user request
  // u8g2.setFont(u8g2_font_5x7_tr);
  // u8g2.drawStr(batX - 35, 8, "SPOTIFY");

  // 2. Artist (Top Line)
  const uint8_t *fArtist = getFont(artist_size, "bold");
  u8g2.setFont(fArtist);

  int wA = u8g2.getStrWidth(currentArtist.c_str());
  int maxW = batX - 40; // Space available

  if (wA > W) { // Scroll if too long
    u8g2.drawStr(-scrollX_Artist, 18, currentArtist.c_str());
  } else {
    u8g2.drawStr(0, 18, currentArtist.c_str());
  }

  // 3. Song (Middle Line)
  const uint8_t *fSong = getFont(song_size, "normal");
  u8g2.setFont(fSong);

  int wS = u8g2.getStrWidth(currentSong.c_str());
  if (wS > W) {
    u8g2.drawStr(-scrollX_Song, 40, currentSong.c_str());
  } else {
    // Center it? Or Left? Let's Center
    u8g2.drawStr((W - wS) / 2, 40, currentSong.c_str());
  }

  // 4. Progress Bar & Time
  int yBar = 54;
  int hBar = 6;

  u8g2.setFont(u8g2_font_helvB08_tr);

  // Calc Times
  long dProg = currentProgress;
  if (isPlaying && lastServerUpdateMillis > 0)
    dProg = lastServerProgress + (millis() - lastServerUpdateMillis);
  if (dProg > currentDuration)
    dProg = currentDuration;

  String sCurr = msToTime(dProg);
  String sDur = msToTime(currentDuration);

  int wCurr = u8g2.getStrWidth(sCurr.c_str());
  int wDur = u8g2.getStrWidth(sDur.c_str());

  // Draw Times
  u8g2.drawStr(2, 63, sCurr.c_str());
  u8g2.drawStr(W - wDur - 2, 63, sDur.c_str());

  // Draw Bar
  int barStart = wCurr + 6;
  int barWidth = (W - wDur - 6) - barStart;

  u8g2.drawFrame(barStart, yBar, barWidth, hBar);
  if (currentDuration > 0) {
    long fill = (dProg * barWidth) / currentDuration;
    if (fill > barWidth)
      fill = barWidth;
    u8g2.drawBox(barStart, yBar, fill, hBar);
  }
}

void drawMenu() {
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.drawStr(0, 14, "MENU");
  u8g2.drawLine(0, 16, 45, 16);

  u8g2.setFont(u8g2_font_helvB10_tr);

  // Render 3 items window
  for (int i = 0; i < menuLen; i++) {
    if (abs(menuIndex - i) < 2) {
      int relative = i - menuIndex;
      int y = 35 + (relative * 16);

      String label = String(menuItems[i]);
      String val = "";
      if (label == "Brightness")
        val = ": " + String(map(currentBrightness, 0, 255, 0, 100)) + "%";
      if (label == "Artist Font")
        val = ": " + String(artist_size);
      if (label == "Song Font")
        val = ": " + String(song_size);
      if (label == "Screensaver")
        val = ": " + String(screensaverTimeout / 1000) + "s";

      if (i == menuIndex) {
        u8g2.drawStr(48, y, ">");
      }
      u8g2.drawStr(60, y, (label + val).c_str());
    }
  }
}

void drawStatus(String line1, String line2) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB10_tr);
  int w1 = u8g2.getStrWidth(line1.c_str());
  u8g2.drawStr((256 - w1) / 2, 30, line1.c_str());

  if (line2 != "") {
    u8g2.setFont(u8g2_font_helvR08_tr);
    int w2 = u8g2.getStrWidth(line2.c_str());
    u8g2.drawStr((256 - w2) / 2, 50, line2.c_str());
  }
  u8g2.sendBuffer();
}

void drawInfo() {
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(0, 12, "SYSTEM INFO");
  u8g2.drawLine(0, 14, 256, 14);

  u8g2.setFont(u8g2_font_helvR08_tr);
  u8g2.setCursor(0, 26);
  u8g2.print("IP: ");
  u8g2.print(WiFi.localIP());

  u8g2.setCursor(0, 38);
  u8g2.print("Device ID: ");
  u8g2.print(device_id);

  u8g2.setCursor(0, 50);
  u8g2.print("RAM: ");
  u8g2.print(ESP.getFreeHeap());

  u8g2.setCursor(0, 62);
  u8g2.print("SVer: 2.0 (Relay)");
}

void drawClock() {
  u8g2.clearBuffer();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.drawStr(60, 35, "No Time Data");
    u8g2.sendBuffer();
    return;
  }

  char timeStr[10];
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

  char secStr[10];
  strftime(secStr, sizeof(secStr), ":%S", &timeinfo);

  // Burn-in Protection: Random Movement
  static unsigned long lastMoveTime = 0;
  static int clkX = 10, clkY = 50;

  // Move every minute (or faster for testing? Let's do every 10 seconds)
  // Actually, for visible effect let's do 60s to be subtle but effective
  if (millis() - lastMoveTime > 10000) {
    lastMoveTime = millis();
    // Random position within 256x64
    // Text approx 100x40
    clkX = random(0, 256 - 110);
    clkY = random(40, 64);
  }

  // Big Clock - Using logisoso42 or fallback
  u8g2.setFont(u8g2_font_logisoso42_tn);
  // int w = u8g2.getStrWidth(timeStr); // Don't recenter, use random X
  u8g2.drawStr(clkX, clkY, timeStr);
}

void loopUI() {
  // 60 FPS Cap
  if (millis() - lastFrameTime < 16)
    return;
  lastFrameTime = millis();

  // Handle Input (Button debouncing etc is in ISR but actions happen in main
  // loop) InputManager needs to be called in MAIN Loop, not here in UI thread
  // usually? Actually dedoy called it here. Let's separate it if possible, but
  // for now we can rely on main loop calling handleInputs().

  // Scroll Logic
  scrollX_Artist += 1;
  if (scrollX_Artist > 300)
    scrollX_Artist = -256;

  scrollX_Song += 1;
  if (scrollX_Song > 300)
    scrollX_Song = -256;

  u8g2.clearBuffer();

  if (strlen(spotify_refresh_token) < 5) {
    // Setup Mode
    String qrUrl =
        String(relay_base_url) + "/login.php?device_id=" + String(device_id);
    drawQR(qrUrl);
  } else {
    switch (currentMode) {
    case MODE_MENU:
      drawMenu();
      break;
    case MODE_INFO:
      drawInfo();
      break;
    case MODE_CLOCK:
      drawClock();
      break;
    default:
      drawNowPlaying();
      break;
    }
  }

  u8g2.sendBuffer();
}

void setupUI() {
  u8g2.begin();
  u8g2.setContrast(currentBrightness);
}
