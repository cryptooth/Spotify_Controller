#ifndef SPOTIFYMANAGER_H
#define SPOTIFYMANAGER_H

#include <Arduino.h>

void setupSpotify();
void loopSpotify(); // Main polling loop

// Helper functions (internal usually, but exposed if needed)
// Helper functions (internal usually, but exposed if needed)
bool refreshSpotifyToken();
void getSpotifyCurrentData();

void playPause();
void nextTrack();
void prevTrack();

#endif
