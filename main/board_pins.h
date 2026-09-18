#pragma once

#include "driver/gpio.h"

// ─────────────────────────────────────────────────────────────────────────────
// Single source of truth for every GPIO used by this firmware, on an
// ESP32-H2 SuperMini wired into the CB2S module's 11-pad castellated
// footprint (the module itself is desoldered — this is a flying-wire rework,
// not a drop-in; see README.md's "Wiring" section for the header pad ↔
// SuperMini pad table and the physical/power caveats).
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
// The board-side pin choices are ours, since the two boards are joined by
// hand. The SuperMini brings raw GPIO numbers out on its headers, so the
// numbers below are the chip's own. Constraints applied:
//   - GPIO23/GPIO24 are the ESP32-H2's default console UART0 pins (RX/TX
//     respectively, per soc/uart_pins.h — and silkscreened RX/TX on this
//     board). Deliberately left unused: wiring a signal there crash-loops the
//     console the moment a peripheral driver also claims the pad.
//   - GPIO26/GPIO27 are USB_D-/USB_D+, carrying the USB-C port used to flash
//     and monitor. Left unused.
//   - ESP32-H2 strapping pins (GPIO2, GPIO3, GPIO8, GPIO9, GPIO25) are all
//     avoided for wired signals, so nothing this design drives can hold the
//     chip out of its normal boot mode. GPIO8 and GPIO9 appear below only as
//     the module's *own* onboard RGB LED and BOOT button, which the module
//     already wires that way.
//   - BL0937 signals grouped low (GPIO0/1/4); relay on GPIO11, furthest from
//     the pulse inputs to reduce switching-noise coupling into the counters.
// ─────────────────────────────────────────────────────────────────────────────

// BL0937 energy-metering IC (see main/bl0937.h for the driver).
#define PIN_BL0937_CF  GPIO_NUM_0  // active-power pulse input
#define PIN_BL0937_CF1 GPIO_NUM_1  // voltage/current pulse input (muxed by SEL)
#define PIN_BL0937_SEL GPIO_NUM_4  // output; selects what CF1 currently carries

// Plug's own tactile button. Measured pinout: the button sits on P10, which
// on this module's footprint is the pad silkscreened RX1 — confirmed against
// the plug's schematic, not a rework or jumper.
#define PIN_BUTTON GPIO_NUM_5

// ESP32-H2 SuperMini's own onboard BOOT button (not on the CB2S footprint —
// this is the dev-board button soldered to the module itself, GPIO9,
// active-low to GND via the module's own pull-up, independent of PIN_BUTTON
// above). Wired in as a second, bench-only factory-reset trigger: once the
// plug is closed up this button isn't reachable, so PIN_BUTTON remains the
// real user-facing control.
#define PIN_BOOT_BUTTON GPIO_NUM_9

// Plug's WiFi-status LED (repurposed here as the Matter network/commissioning
// indicator — see status_led.h). This is the only indicator visible once the
// plug's enclosure is closed up; the two onboard LEDs below are bench aids.
#define PIN_LED GPIO_NUM_10

// Plug's relay, switching the load.
#define PIN_RELAY GPIO_NUM_11

// ── Onboard LEDs (module's own, not on the CB2S footprint) ─────────────────
//
// The SuperMini carries two software-drivable LEDs. A third, the battery
// charge LED, is wired to the charger IC and has no GPIO — ignore it.

// Addressable RGB LED (WS2812-family, single pixel) on GPIO8. Driven over RMT
// by the led_strip component; carries Matter network state as colour — see
// status_led.h for the colour table.
#define PIN_RGB_LED GPIO_NUM_8

// Plain yellow user LED on GPIO13, active-high on this board. Mirrors relay
// state, which is the only toggle feedback available on USB-only bench power
// (the relay coil needs the mains-derived rail — see relay.h).
#define PIN_ONBOARD_LED GPIO_NUM_13

// No RF antenna switch on this board: a single PCB trace antenna, no U.FL
// connector and no switch GPIOs. There is nothing to select in software, so
// this design has no antenna-init step.
