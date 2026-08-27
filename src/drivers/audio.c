/* croOS audio.c - PC Speaker + Timer-based audio output
 * Uses PIT channel 2 for PC speaker frequency generation.
 * Can play melodies and sound effects via frequency/duration pairs. */

#include "kernel/types.h"
#include "audio.h"
#include "drivers/timer.h"

void speaker_init(void) {
    /* Enable PIT channel 2 for speaker */
    uint8_t tmp = inb(0x61);
    tmp &= 0xFC;  /* Clear bits 0-1 */
    outb(0x61, tmp);
}

void speaker_beep(uint32_t freq, uint32_t duration_ms) {
    if (freq == 0) { timer_sleep(duration_ms); return; }

    uint32_t divisor = 1193180 / freq;

    /* Configure PIT channel 2 for square wave */
    outb(0x43, 0xB6);  /* Channel 2, lo/hi byte, square wave */
    outb(0x42, (uint8_t)(divisor & 0xFF));
    outb(0x42, (uint8_t)((divisor >> 8) & 0xFF));

    /* Enable speaker */
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x03);

    timer_sleep(duration_ms);

    /* Disable speaker */
    tmp = inb(0x61);
    outb(0x61, tmp & 0xFC);
}

void speaker_off(void) {
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & 0xFC);
}

void speaker_play_notes(const uint32_t *freqs, const uint32_t *durations, int count) {
    for (int i = 0; i < count; i++) {
        speaker_beep(freqs[i], durations[i]);
    }
}

/* Pre-defined melodies */
void audio_boot_sound(void) {
    uint32_t freqs[] = { NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5 };
    uint32_t durs[]  = { 80, 80, 80, 200 };
    speaker_play_notes(freqs, durs, 4);
}

void audio_error_sound(void) {
    uint32_t freqs[] = { NOTE_C4, NOTE_REST, NOTE_C4 };
    uint32_t durs[]  = { 150, 50, 150 };
    speaker_play_notes(freqs, durs, 3);
}

void audio_click_sound(void) {
    speaker_beep(1000, 10);
}
