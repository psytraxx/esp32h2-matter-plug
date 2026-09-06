#pragma once

// Interrupt-driven plug button (PIN_BUTTON — see board_pins.h; this is the
// CB2S plug's own tactile switch, wired to the pad measured as P10, which on
// this module's footprint is the pad silkscreened RX1).
//
// A hold of at least FACTORY_RESET_HOLD_MS fires on_long_press; a shorter
// press fires on_short_press. Both callbacks run in a dedicated task
// context, so they may safely call Matter APIs. Either callback may be NULL.
//
// Ported near-verbatim from esp32c6-radar-demo-matter/main/button.{h,cpp};
// only PIN_WAKE_BUTTON -> PIN_BUTTON changed.
void button_init(void (*on_long_press)(void), void (*on_short_press)(void));
