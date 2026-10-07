#include "AudioEngine.h"

#include <Arduino.h>

#include "pins.h"

AudioEngine::AudioEngine(uint8_t pin) : pin(pin)
{
    pinMode(pin, OUTPUT);

    // flush any existing tone
    stop();
}

void AudioEngine::stop()
{
    noTone(this->pin);
    curr_melody = nullptr;
    melody_length = 0;
    curr_note_idx = 0;
    is_playing = false;
}

void AudioEngine::playMelody(const BuzzerNote* melody, size_t note_count)
{
    if (melody == nullptr || note_count == 0) return;

    curr_melody = melody;
    melody_length = note_count;
    curr_note_idx = 0;

    // play note immedieately and
    // dont wait for the first update() call
    playNextNote();
}

/**
 * non blocking function to play a single tone.
 * it overwrittes the previous melody
 */
void AudioEngine::playTone(const BuzzerNote& note)
{
    curr_melody = nullptr;
    melody_length = 0;
    curr_note_idx = 0;

    curr_note_duration = note.duration;
    curr_note_start_time = millis();
    is_playing = true;
    if (note.freq > 0) {
        tone(pin, note.freq);
    } else {
        noTone(pin);
    }
}

void AudioEngine::playNextNote()
{
    if (curr_note_idx < melody_length) {
        BuzzerNote note = curr_melody[curr_note_idx++];

        curr_note_duration = note.duration;

        curr_note_start_time = millis();
        is_playing = true;

        if (note.freq > 0) {
            tone(pin, note.freq);
        } else {
            noTone(pin);
        }
    } else {
        stop();
    }
}

void AudioEngine::update()
{
    if (!is_playing) return;

    // the timing isint exactly precise
    // since delays before update() calls
    // would stack up on currently played note's duration
    if (millis() - curr_note_start_time >= curr_note_duration) {
        if (curr_melody != nullptr) {
            playNextNote();
        } else {
            stop();
        }
    }
}

void AudioEngine::tillEnd()
{
    if (!is_playing) return;

    while (isPlaying()) update();
}

AudioEngine Buzzer(BUZZER);