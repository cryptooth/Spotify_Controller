#include "Globals.h"
#include "SpotifyManager.h" // To control playback

// Encoder State
volatile long encoderPos = 0;
volatile int lastEncoded = 0;
long lastEncoderPos = 0;

// Button State for ISR
volatile bool btnShortPressTriggered = false;
volatile bool btnLongPressTriggered = false;
volatile unsigned long isrBtnDownTime = 0;
volatile bool isrLongPressHandled = false;

// Config Manager Prototypes
void saveConfig();
void resetConfig();

// ISR for Encoder Rotation
void IRAM_ATTR isrEncoder() {
  int MSB = digitalRead(ENC_CLK);
  int LSB = digitalRead(ENC_DT);

  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderPos++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderPos--;

  lastEncoded = encoded;
}

// ISR for Button
void IRAM_ATTR isrEncSwitch() {
  int state = digitalRead(ENC_SW);
  // Simple Debounce
  static unsigned long lastIsrTime = 0;
  if (millis() - lastIsrTime < 50)
    return;
  lastIsrTime = millis();

  if (state == LOW) { // Pressed
    isrBtnDownTime = millis();
    isrLongPressHandled = false;
  } else { // Released
    if (isrBtnDownTime != 0 && !isrLongPressHandled) {
      if (millis() - isrBtnDownTime < 800) {
        btnShortPressTriggered = true;
      }
    }
    isrBtnDownTime = 0;
  }
}

void setupInputs() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_CLK), isrEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT), isrEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_SW), isrEncSwitch, CHANGE);
  Serial.println("[INP] Encoders Initialized on Pins 1, 3, 5");
}

void handleInputs() {
  // Debug Heartbeat for Inputs (Verify Loop)
  /*
  static unsigned long lastDb = 0;
  if (millis() - lastDb > 5000) {
      Serial.printf("[INP] Pos: %ld | Pin State: CLK=%d DT=%d SW=%d\n",
  encoderPos, digitalRead(ENC_CLK), digitalRead(ENC_DT), digitalRead(ENC_SW));
      lastDb = millis();
  }
  */

  // Wake Logic
  if (currentMode == MODE_CLOCK) {
    long currentDividedPos = encoderPos / 2;
    if (currentDividedPos != lastEncoderPos || digitalRead(ENC_SW) == LOW) {
      currentMode = MODE_PLAYING;
      lastInputTime = millis();
      // Consume input to prevent accidental action
      if (digitalRead(ENC_SW) == LOW)
        isrBtnDownTime = 0;
      lastEncoderPos = currentDividedPos;
      return;
    }
  }

  bool interaction = false;

  // 1. ROTATION
  long newPos = encoderPos / 2; // Divide by 2 for stability
  if (newPos != lastEncoderPos) {
    interaction = true;
    int direction = (newPos > lastEncoderPos) ? 1 : -1;
    lastEncoderPos = newPos;

    if (currentMode == MODE_MENU) {
      if (direction > 0) { // CW: Next
        menuIndex++;
        if (menuIndex >= menuLen)
          menuIndex = 0;
      } else { // CCW: Prev
        menuIndex--;
        if (menuIndex < 0)
          menuIndex = menuLen - 1;
      }
    } else if (currentMode == MODE_PLAYING) {
      // Rotate to Skip?
      if (direction > 0) {
        Serial.println("Skip Next");
        nextTrack();
      } else {
        Serial.println("Skip Prev");
        prevTrack();
      }
    }
  }

  if (interaction)
    lastInputTime = millis();

  // 2. LONG PRESS CHECK (Polled)
  if (isrBtnDownTime != 0 && !isrLongPressHandled) {
    if (millis() - isrBtnDownTime > 800) {
      btnLongPressTriggered = true;
      isrLongPressHandled = true;
    }
  }

  // 3. ACTIONS
  if (btnLongPressTriggered) {
    btnLongPressTriggered = false;
    Serial.println("LONG PRESS");

    if (currentMode == MODE_MENU || currentMode == MODE_INFO) {
      currentMode = MODE_PLAYING; // Exit
    } else {
      currentMode = MODE_MENU; // Enter Menu
      menuIndex = 0;
    }
  }

  if (btnShortPressTriggered) {
    btnShortPressTriggered = false;
    Serial.println("SHORT PRESS");
    lastInputTime = millis();

    if (currentMode == MODE_PLAYING) {
      // Toggle Play/Pause
      playPause();
    } else if (currentMode == MODE_MENU) {
      String item = String(menuItems[menuIndex]);

      if (item == "Exit")
        currentMode = MODE_PLAYING;
      else if (item == "Reboot")
        ESP.restart();
      else if (item == "Factory Reset")
        resetConfig();
      else if (item == "Info")
        currentMode = MODE_INFO;
      else if (item == "Artist Font") {
        if (String(artist_size) == "small")
          strcpy(artist_size, "medium");
        else if (String(artist_size) == "medium")
          strcpy(artist_size, "large");
        else
          strcpy(artist_size, "small");
        saveConfig();
      } else if (item == "Song Font") {
        if (String(song_size) == "small")
          strcpy(song_size, "medium");
        else if (String(song_size) == "medium")
          strcpy(song_size, "large");
        else
          strcpy(song_size, "small");
        saveConfig();
      } else if (item == "Brightness") {
        currentBrightness += 51;
        if (currentBrightness > 255)
          currentBrightness = 0; // Off logic? Maybe 51 min
        if (currentBrightness == 0)
          currentBrightness = 51; // Don't allow full off
        u8g2.setContrast(currentBrightness);
        saveConfig();
      } else if (item == "Screensaver") {
        if (screensaverTimeout == 5000)
          screensaverTimeout = 15000;
        else if (screensaverTimeout == 15000)
          screensaverTimeout = 30000;
        else if (screensaverTimeout == 30000)
          screensaverTimeout = 60000;
        else
          screensaverTimeout = 5000;
        saveConfig();
      }
    } else if (currentMode == MODE_INFO) {
      currentMode = MODE_MENU; // Back to Menu
    }
  }
}
