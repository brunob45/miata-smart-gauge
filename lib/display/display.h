#if !defined(DASH_DISPLAY_H)
#define DASH_DISPLAY_H

#include <ILI9341_T4.h>

#define DISPLAY_ALERT ILI9341_RED
#define DISPLAY_ACCENT1 0x7fe2
#define DISPLAY_ACCENT2 0x39e7
#define DISPLAY_FG1 ILI9341_LIGHTGREY
#define DISPLAY_FG2 ILI9341_DARKGREY
#define DISPLAY_BG ILI9341_BLACK

namespace Display
{
void init(void);
void update(void);
void alert(bool enable);
} // namespace Display

#endif // DASH_DISPLAY_H