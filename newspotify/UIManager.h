#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <Arduino.h>

void setupUI();
void loopUI();
void drawInfo();                                  // System Info Screen
void drawStatus(String line1, String line2 = ""); // Simple Message
void drawQR(String url);                          // New Helper for QR Code

#endif
