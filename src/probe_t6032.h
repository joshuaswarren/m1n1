/* SPDX-License-Identifier: MIT */

#ifndef PROBE_T6032_H
#define PROBE_T6032_H

#include "types.h"

/* On-screen T6032 (Mac Studio) probe: runs at boot on this chip only, draws
 * its findings in large text on the firmware framebuffer, then returns so
 * the normal proxy path runs. Never boots anything. */

void probe_t6032_run(void);

/* Renders the same screen with made-up numbers labelled DEMO. Only called
 * from the PROBE_FB_DEMO build on non-T6032 boards (the M6 bring-up path). */
void probe_fb_demo_run(void);

/* Countdown line formatting (pure; host-tested) and the single-line redraw.
 * "REBOOTING IN m:ss - hold power to turn off": the reset does not select a
 * boot volume, returning to macOS needs the tester restored separately. */
void probe_fmt_countdown(char *dst, u32 cap, int remain);
void probe_countdown_line(int remain);

/* The framebuffer row the countdown line occupies (set by the last render). */
u32 probe_countdown_row(void);

/* Test hooks: the countdown length in seconds and the per-update wait. */
extern u32 probe_autoreturn_secs;
extern u32 probe_autoreturn_step_us;

#endif
