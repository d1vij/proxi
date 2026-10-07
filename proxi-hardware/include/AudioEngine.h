#pragma once

#include <array>

typedef struct BuzzerNote {
    uint32_t freq;
    uint64_t duration;
};

/**
 * Wrapper class to time and play melody on a standard passive buzzer.
 * Melody initiated by calling playMelody() fn, and by constantly polling
 * using the update() fn
 */
class AudioEngine
{
   public:
    // NOTE: the explicit keyword ensures that no implicit
    // casting happens at initialization
    explicit AudioEngine(uint8_t pin);

    void playTone(const BuzzerNote& melody);
    void playMelody(const BuzzerNote* melody, size_t note_count);

    void update();

    /**
     * plays the audio till completion
     */
    void tillEnd();
    void stop();

    bool isPlaying() { return is_playing; }

   private:
    uint8_t pin;
    bool is_playing = false;
    const BuzzerNote* curr_melody;

    size_t melody_length;
    size_t curr_note_idx;
    size_t curr_note_duration;
    size_t curr_note_start_time;

    void playNextNote();
};

extern AudioEngine Buzzer;