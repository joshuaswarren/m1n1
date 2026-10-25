/* SPDX-License-Identifier: MIT */

#ifndef __WDT_H__
#define __WDT_H__

#include "types.h"

void wdt_disable(void);
void wdt_reboot(void);
int wdt_arm_seconds(unsigned seconds);

/* True when wdt_disable() cached a live watchdog base at boot, i.e.
 * wdt_reboot() will actually arm it. Read-only. */
bool wdt_available(void);

#endif
