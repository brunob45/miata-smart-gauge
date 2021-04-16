#ifndef BB_RPM_H
#define BB_RPM_H

#include <stdint.h>

namespace RPM
{
void init();
void update();
uint16_t get_value();
} // namespace RPM

#endif // BB_RPM_H