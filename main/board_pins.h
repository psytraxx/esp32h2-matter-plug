#pragma once

#include "driver/gpio.h"

// ─────────────────────────────────────────────────────────────────────────────
// Single source of truth for every GPIO used by this firmware, on a Seeed
// XIAO ESP32-C6 wired into the CB2S module's 11-pad castellated footprint
// (the module itself is desoldered — this is a flying-wire rework, not a
// drop-in; see README.md's "Wiring" section for the header pad ↔ XIAO pad
// table and the physical/power caveats).
//
// PIN ROLES ARE NEARLY INVERTED relative to the sibling uascent-matter
// project (a UAM023-based plug). Do NOT reuse that project's board overlay
// or pin numbers — this repo's README "Measured pinout" is the only
// authoritative source for THIS plug:
//
//   Signal        Uascent (UAM023)   This plug (CB2S)
//   BL0937 CF     P24                P7  -> here: PIN_BL0937_CF
//   BL0937 CF1    P26                P6  -> here: PIN_BL0937_CF1
//   BL0937 SEL    P8                 P24 -> here: PIN_BL0937_SEL
//   Relay         P6                 P26 -> here: PIN_RELAY
//   LED           P7                 P8  -> here: PIN_LED
//   Button        RX1 (P10)          RX1 (P10, same convention)
//
// XIAO-side pin choices (D0-D5) are ours, since the two boards are joined by
// hand. Constraints applied:
//   - D6/D7 (GPIO16/17) are the ESP32-C6's default console UART0 pins —
//     deliberately left unused, matching the trap documented in the sibling
//     esp32c6-radar-demo-matter project's board_pins.h.
//   - None of D0-D10 are ESP32-C6 strapping pins (those are GPIO4/5/8/9/15,
//     on the MTMS/MTDI/Boot/Light pads, not used by this design) — so there
//     is no boot-state constraint on any of the choices below.
//   - BL0937 signals grouped on D0-D2; relay on D5, furthest from the pulse
//     inputs to reduce switching-noise coupling into the pulse counters.
// ─────────────────────────────────────────────────────────────────────────────

// BL0937 energy-metering IC (see main/bl0937.h for the driver).
#define PIN_BL0937_CF  GPIO_NUM_0  // D0 — active-power pulse input
#define PIN_BL0937_CF1 GPIO_NUM_1  // D1 — voltage/current pulse input (muxed by SEL)
#define PIN_BL0937_SEL GPIO_NUM_2  // D2 — output; selects what CF1 currently carries

// Plug's own tactile button. Measured pinout: the button sits on P10, which
// on this module's footprint is the pad silkscreened RX1 — confirmed against
// the plug's schematic, not a rework or jumper.
#define PIN_BUTTON GPIO_NUM_21 // D3

// Plug's WiFi-status LED (repurposed here as the Matter network/commissioning
// indicator — see status_led.h).
#define PIN_LED GPIO_NUM_22 // D4

// Plug's relay, switching the load.
#define PIN_RELAY GPIO_NUM_23 // D5

// D6/D7 (GPIO16/17) intentionally unused — console UART0. D8-D10 are spare.
