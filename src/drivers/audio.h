/* croOS audio.h - Audio output: PC speaker + AC97 */
#ifndef _AUDIO_H
#define _AUDIO_H

#include "kernel/types.h"

/* PC Speaker */
void speaker_init(void);
void speaker_beep(uint32_t freq, uint32_t duration_ms);
void speaker_off(void);
void speaker_play_notes(const uint32_t *freqs, const uint32_t *durations, int count);

/* Note frequencies (Hz) */
#define NOTE_C3  131
#define NOTE_D3  147
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_REST 0

#endif
