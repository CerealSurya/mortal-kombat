// Sound.h
// Runs on MSPM0
// Play sounds on 12-bit DAC.
// Your name
// 11/5/2023
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

// Each "note" is one full play of a waveform at a given pitch.
struct Note {
    const uint8_t *wave;
    uint32_t       len;      // samples in one waveform cycle
    uint32_t       period;   // SysTick reload value for this note's pitch
    uint32_t       repeats;  // how many waveform cycles to hold this note
};

// Initialize an 11 kHz SysTick, but do not start any sound.
// Initialize any global variables.
// Initialize the 12-bit DAC.
// Call once at startup.
void Sound_Init(void);

//******* Sound_Start ************
// Does not output to the DAC directly.
// Sets a pointer and counter so the SysTick ISR outputs one sample per tick.
// Sound plays once and stops.
// Input: pt    pointer to an array of 8-bit unsigned PCM samples
//        count number of samples in the array
// Output: none
void Sound_Start(const uint8_t *pt, uint32_t count);

// The following functions configure pointers/counters and call Sound_Start.
void Sound_Shoot(void);
void Sound_Killed(void);
void Sound_Explosion(void);
void Sound_Fastinvader1(void);
void Sound_Fastinvader2(void);
void Sound_Fastinvader3(void);
void Sound_Fastinvader4(void);
void Sound_Highpitch(void);

// Begin looping through a Note sequence.
// Sound effects (Sound_Start) preempt music; music resumes when the effect ends.
// Input: seq  pointer to array of Note structs
//        len  number of notes in the array
void Sound_MusicStart(const Note *seq, uint32_t len);

// Stop music playback and return to silence.
void Sound_MusicStop(void);

#endif
