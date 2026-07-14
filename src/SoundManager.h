#pragma once

#include <Arduino.h>

enum class SoundEffect : uint8_t {
    NONE,
    BUTTON,
    ERROR,
    LASER,
    MISSILE,
    AUTOCANNON,
    MACHINE_GUN,
    DESTROYED,
    VICTORY,
    RADAR_PING,
    LEVEL_UP
};

void soundSetup(uint8_t buzzerPin);
void soundUpdate();

void playSound(SoundEffect effect);
void stopSound();

bool isSoundPlaying();
void setSoundEnabled(bool enabled);
bool isSoundEnabled();