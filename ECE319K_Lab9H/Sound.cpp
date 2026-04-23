// Sound.cpp
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "./inc/Sound.h"
#include "sounds/sounds.h"
#include "./inc/DAC.h"

static const uint8_t *SoundPt;       // pointer into current waveform
static uint32_t       SoundCount;    // samples remaining in this waveform pass

static const Note    *MusicSeq;      // pointer to note array
static uint32_t       MusicLen;      // total notes in sequence
static uint32_t       MusicIdx;      // current note index
static uint8_t        MusicActive;   // 1 = music playing
static uint32_t       NoteRepeatsLeft; // waveform cycles still to play in this note
static uint8_t        PlayingEffect; // 1 = SoundPt is from Sound_Start, not music

void SysTick_IntArm(uint32_t period, uint32_t priority){
    SysTick->CTRL  = 0;                          // disable while configuring
    SysTick->LOAD  = period - 1;
    SysTick->VAL   = 0;
    SCB->SHP[11]   = priority << 5;              // SysTick is exception 15, shp[11]
    SysTick->CTRL  = 0x07;                       // enable, interrupts, use core clock
}

void Sound_Init(void){
    SoundPt         = 0;
    SoundCount      = 0;
    MusicActive     = 0;
    NoteRepeatsLeft = 0;
    PlayingEffect   = 0;
    DAC_Init();
    SysTick_IntArm(80000000 / 11000, 2);  // 11 kHz at 80 MHz bus
}

extern "C" void SysTick_Handler(void);
void SysTick_Handler(void){
    if(SoundCount > 0){
        DAC_Out((uint32_t)(*SoundPt) << 4);
        SoundPt++;
        SoundCount--;
        // end of one waveform cycle for a music note — loop if repeats remain
        if(SoundCount == 0 && MusicActive && !PlayingEffect && NoteRepeatsLeft > 1){
            NoteRepeatsLeft--;
            SoundPt    = MusicSeq[MusicIdx].wave;
            SoundCount = MusicSeq[MusicIdx].len;
        }
    } else if(MusicActive){
        // advance to next note
        MusicIdx        = (MusicIdx + 1) % MusicLen;
        const Note *n   = &MusicSeq[MusicIdx];
        NoteRepeatsLeft = n->repeats;
        PlayingEffect   = 0;
        SysTick->LOAD   = n->period - 1;
        SysTick->VAL    = 0;
        SoundPt         = n->wave;
        SoundCount      = n->len;
        DAC_Out((uint32_t)(*SoundPt) << 4);
        SoundPt++;
        SoundCount--;
    } else {
        DAC_Out(2048);  // midpoint = silence
    }
}

void Sound_Start(const uint8_t *pt, uint32_t count){
    SysTick->CTRL &= ~0x02;
    PlayingEffect = 1;
    SoundPt    = pt;
    SoundCount = count;
    SysTick->CTRL |= 0x02;
}

void Sound_Shoot(void)     { Sound_Start(shoot,     4080); }
void Sound_Killed(void)    { Sound_Start(shoot,     4080); }
void Sound_Explosion(void) { Sound_Start(shoot,     4080); }

void Sound_Fastinvader1(void){}
void Sound_Fastinvader2(void){}
void Sound_Fastinvader3(void){}
void Sound_Fastinvader4(void){}
void Sound_Highpitch(void) {}

void Sound_MusicStart(const Note *seq, uint32_t len){
    MusicSeq        = seq;
    MusicLen        = len;
    MusicIdx        = 0;
    MusicActive     = 1;
    PlayingEffect   = 0;
    NoteRepeatsLeft = seq[0].repeats;
    SoundPt         = seq[0].wave;
    SoundCount      = seq[0].len;
    SysTick->LOAD   = seq[0].period - 1;
    SysTick->VAL    = 0;
}

void Sound_MusicStop(void){
    MusicActive = 0;
}