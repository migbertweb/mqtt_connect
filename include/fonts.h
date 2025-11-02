#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

// Declarar el font como extern (definido en fonts.c)
extern const uint8_t font_5x7[95][5];

// Prototipos de funciones simples
const uint8_t* fonts_get_font_5x7(void);
int fonts_get_char_width(void);
int fonts_get_char_height(void);

#endif // FONTS_H