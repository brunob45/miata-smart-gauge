#ifndef DASH_SPEEDO_H
#define DASH_SPEEDO_H

#include <stdint.h>

namespace Speedo
{
void init();
void update();
uint16_t get_value();
} // namespace Speedo

#endif // DASH_SPEEDO_H