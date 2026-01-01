#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <Arduino.h>

void setupConfig();
void saveConfig();
void loadConfig();
void resetConfig();

// Callback for WiFiManager
void saveConfigCallback();

#endif
