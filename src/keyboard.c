/* corrOS — PS/2 keyboard (polling, no IRQ needed). */
#include <stdint.h>
#include "ports.h"

static const char scancode_table[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,
    '\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,'+',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    '7','8','9','-', 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const char scancode_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,
    '|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

int kb_read_scancode(void)
{
    if (!(inb(0x64) & 1)) return -1;
    uint8_t sc = inb(0x60);
    if (sc >= 128) return -1;  /* key-up ignored */
    return sc;
}

char kb_scancode_to_ascii(int sc, int shift)
{
    if (sc < 0 || sc >= 128) return 0;
    if (shift) return scancode_shift[sc];
    return scancode_table[sc];
}
