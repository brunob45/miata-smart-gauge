#if !defined(BB_DISPLAY_H)
#define BB_DISPLAY_H

namespace Display
{
void init();
void update();
void select(uint8_t index);
void setLumi(uint16_t lumi);
}; // namespace Display

#endif // BB_DISPLAY_H