#include "SoundManager.h"

// ESP32 LEDC configuration
static constexpr uint8_t BUZZER_CHANNEL = 0;
static constexpr uint8_t BUZZER_RESOLUTION = 8;

struct SoundStep {
    uint16_t frequency;
    uint16_t durationMs;
    uint16_t pauseMs;
};

static uint8_t gBuzzerPin = 255;
static bool gSoundEnabled = true;
static bool gSoundPlaying = false;

static const SoundStep* gCurrentSequence = nullptr;
static size_t gCurrentSequenceLength = 0;
static size_t gCurrentStep = 0;

static unsigned long gStepStartedAt = 0;
static bool gInPause = false;

// ---------------------------------------------------------
// Sound sequences
// ---------------------------------------------------------

static const SoundStep BUTTON_SOUND[] = {
    {1400, 25, 0}
};

static const SoundStep ERROR_SOUND[] = {
    {350, 100, 40},
    {250, 180, 0}
};

static const SoundStep LASER_SOUND[] = {
    {2000, 25, 0},
    {1800, 25, 0},
    {1600, 25, 0},
    {1400, 30, 0},
    {1200, 40, 0}
};

static const SoundStep MISSILE_SOUND[] = {
    {700, 35, 20},
    {800, 35, 20},
    {900, 35, 20},
    {1000, 45, 0}
};

static const SoundStep AUTOCANNON_SOUND[] = {
    {180, 70, 55},
    {150, 80, 55},
    {190, 90, 0}
};

static const SoundStep MACHINE_GUN_SOUND[] = {
    {1100, 20, 18},
    {900, 20, 18},
    {1150, 20, 18},
    {850, 20, 18},
    {1050, 20, 0}
};

static const SoundStep DESTROYED_SOUND[] = {
    {1500, 45, 0},
    {1350, 45, 0},
    {1200, 45, 0},
    {1050, 50, 0},
    {900, 55, 0},
    {750, 60, 0},
    {600, 70, 0},
    {450, 85, 0},
    {300, 110, 40},
    {140, 450, 0}
};

static const SoundStep VICTORY_SOUND[] = {
    {523, 100, 35},
    {659, 100, 35},
    {784, 120, 35},
    {1047, 260, 0}
};

static const SoundStep RADAR_PING_SOUND[] = {
    {1800, 35, 30},
    {2400, 70, 0}
};

static const SoundStep LEVEL_UP_SOUND[] = {
    {440, 80, 25},
    {554, 80, 25},
    {659, 80, 25},
    {880, 180, 0}
};

// ---------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------

static void buzzerTone(uint16_t frequency)
{
    if (!gSoundEnabled || gBuzzerPin == 255 || frequency == 0) {
        ledcWriteTone(BUZZER_CHANNEL, 0);
        return;
    }

    ledcWriteTone(BUZZER_CHANNEL, frequency);
}

static void beginSequence(
    const SoundStep* sequence,
    size_t sequenceLength
) {
    if (!gSoundEnabled || sequence == nullptr || sequenceLength == 0) {
        return;
    }

    gCurrentSequence = sequence;
    gCurrentSequenceLength = sequenceLength;
    gCurrentStep = 0;
    gInPause = false;
    gSoundPlaying = true;
    gStepStartedAt = millis();

    buzzerTone(gCurrentSequence[0].frequency);
}

// ---------------------------------------------------------
// Public interface
// ---------------------------------------------------------

void soundSetup(uint8_t buzzerPin)
{
    gBuzzerPin = buzzerPin;

    pinMode(gBuzzerPin, OUTPUT);

    ledcSetup(
        BUZZER_CHANNEL,
        2000,
        BUZZER_RESOLUTION
    );

    ledcAttachPin(
        gBuzzerPin,
        BUZZER_CHANNEL
    );

    ledcWriteTone(BUZZER_CHANNEL, 0);
}

void soundUpdate()
{
    if (!gSoundPlaying ||
        gCurrentSequence == nullptr ||
        gCurrentStep >= gCurrentSequenceLength) {
        return;
    }

    const unsigned long now = millis();
    const SoundStep& step = gCurrentSequence[gCurrentStep];

    if (!gInPause) {
        if (now - gStepStartedAt >= step.durationMs) {
            buzzerTone(0);

            if (step.pauseMs > 0) {
                gInPause = true;
                gStepStartedAt = now;
            } else {
                gCurrentStep++;

                if (gCurrentStep >= gCurrentSequenceLength) {
                    stopSound();
                    return;
                }

                gStepStartedAt = now;
                buzzerTone(
                    gCurrentSequence[gCurrentStep].frequency
                );
            }
        }
    } else {
        if (now - gStepStartedAt >= step.pauseMs) {
            gInPause = false;
            gCurrentStep++;

            if (gCurrentStep >= gCurrentSequenceLength) {
                stopSound();
                return;
            }

            gStepStartedAt = now;
            buzzerTone(
                gCurrentSequence[gCurrentStep].frequency
            );
        }
    }
}

void playSound(SoundEffect effect)
{
    // A new sound replaces the current sound.
    stopSound();

    switch (effect) {
        case SoundEffect::BUTTON:
            beginSequence(
                BUTTON_SOUND,
                sizeof(BUTTON_SOUND) / sizeof(BUTTON_SOUND[0])
            );
            break;

        case SoundEffect::ERROR:
            beginSequence(
                ERROR_SOUND,
                sizeof(ERROR_SOUND) / sizeof(ERROR_SOUND[0])
            );
            break;

        case SoundEffect::LASER:
            beginSequence(
                LASER_SOUND,
                sizeof(LASER_SOUND) / sizeof(LASER_SOUND[0])
            );
            break;

        case SoundEffect::MISSILE:
            beginSequence(
                MISSILE_SOUND,
                sizeof(MISSILE_SOUND) / sizeof(MISSILE_SOUND[0])
            );
            break;

        case SoundEffect::AUTOCANNON:
            beginSequence(
                AUTOCANNON_SOUND,
                sizeof(AUTOCANNON_SOUND) /
                    sizeof(AUTOCANNON_SOUND[0])
            );
            break;

        case SoundEffect::MACHINE_GUN:
            beginSequence(
                MACHINE_GUN_SOUND,
                sizeof(MACHINE_GUN_SOUND) /
                    sizeof(MACHINE_GUN_SOUND[0])
            );
            break;

        case SoundEffect::DESTROYED:
            beginSequence(
                DESTROYED_SOUND,
                sizeof(DESTROYED_SOUND) /
                    sizeof(DESTROYED_SOUND[0])
            );
            break;

        case SoundEffect::VICTORY:
            beginSequence(
                VICTORY_SOUND,
                sizeof(VICTORY_SOUND) /
                    sizeof(VICTORY_SOUND[0])
            );
            break;

        case SoundEffect::RADAR_PING:
            beginSequence(
                RADAR_PING_SOUND,
                sizeof(RADAR_PING_SOUND) /
                    sizeof(RADAR_PING_SOUND[0])
            );
            break;

        case SoundEffect::LEVEL_UP:
            beginSequence(
                LEVEL_UP_SOUND,
                sizeof(LEVEL_UP_SOUND) /
                    sizeof(LEVEL_UP_SOUND[0])
            );
            break;

        case SoundEffect::NONE:
        default:
            break;
    }
}

void stopSound()
{
    ledcWriteTone(BUZZER_CHANNEL, 0);

    gSoundPlaying = false;
    gCurrentSequence = nullptr;
    gCurrentSequenceLength = 0;
    gCurrentStep = 0;
    gInPause = false;
}

bool isSoundPlaying()
{
    return gSoundPlaying;
}

void setSoundEnabled(bool enabled)
{
    gSoundEnabled = enabled;

    if (!enabled) {
        stopSound();
    }
}

bool isSoundEnabled()
{
    return gSoundEnabled;
}