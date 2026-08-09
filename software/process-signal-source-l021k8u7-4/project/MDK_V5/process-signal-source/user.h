#ifndef USER_H
#define USER_H

#include "global_vars.h"

void setup(void);

void loop(void);

bool switch_mode(device_mode_t next_mode);
bool set_dcoffset(int16_t next_value);
bool set_vpp(int16_t next_value);

#endif
