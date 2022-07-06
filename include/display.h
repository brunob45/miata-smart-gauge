#if !defined(DASH_DISPLAY_H)
#define DASH_DISPLAY_H

#include <ChRt.h>
#include <ILI9341_t3n.h>

int display_init(int prio, void* arg);
uint8_t numSize(uint16_t n);
void drawNumber(int number, int scale, int offset, int x, int y);

#endif // DASH_DISPLAY_H