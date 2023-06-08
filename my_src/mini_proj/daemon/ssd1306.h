#pragma once
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

int  ssd1306_init();
void ssd1306_set_position (uint32_t column, uint32_t row);
void ssd1306_putc(char c);
void ssd1306_puts(const char* str);
void ssd1306_clear_display();

#endif