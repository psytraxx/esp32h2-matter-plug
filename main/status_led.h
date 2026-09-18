#pragma once

// Three LEDs show this plug's state, each with a distinct job.
//
// 1. PIN_LED — the plug's own WiFi-status LED on the CB2S footprint,
//    repurposed as the Matter network/commissioning indicator: solid on for
//    boot/paired, blinking while the commissioning window is open. This is
//    the only indicator visible once the enclosure is closed, so it is the
//    user-facing one. The plug's other LED (measured on the P26/relay net in
//    the original CB2S wiring) sits on the relay drive net in hardware and
//    follows relay state without a GPIO of its own; see relay.cpp.
//
// 2. PIN_RGB_LED — the SuperMini's onboard addressable RGB LED, carrying the
//    same network state as PIN_LED but as a colour, which is legible at a
//    glance on the bench in a way a single blink pattern is not:
//
//      boot           white, dim
//      commissioning  blue, blinking
//      paired / OK    green, dim
//      error          red, solid
//
// 3. PIN_ONBOARD_LED — the SuperMini's plain yellow user LED, mirroring relay
//    state. Relay state matters on the bench because the relay coil runs off
//    a mains-derived rail and will not physically click on USB-only power
//    (see relay.h), so this LED is the only feedback that a controller toggle
//    actually landed. It is kept separate from the RGB LED deliberately: the
//    two states are independent, and one indicator per state means neither
//    has to pre-empt the other — a single onboard LED arbitrating between them
//    would lose relay state for the whole commissioning window.
//
// PIN_LED polarity is UNVERIFIED — confirm on the bench (README's Verification
// section) before relying on "off" meaning what you expect. Both onboard LEDs
// are driven active-high, which is how this board wires them.

#include <stdbool.h>

typedef enum
{
    STATUS_LED_BOOT,          // device alive, boot in progress
    STATUS_LED_COMMISSIONING, // commissioning window open
    STATUS_LED_OK,            // commissioned and paired
    STATUS_LED_ERROR,         // error indication
} status_led_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void status_led_init(void);

// Drives the network/commissioning indication on both the plug's LED
// (PIN_LED) and the onboard RGB LED (PIN_RGB_LED).
void status_led_set(status_led_state_t state);

// Drives the onboard yellow LED (PIN_ONBOARD_LED) from relay state. Takes
// effect immediately and is never overridden — it shares no LED with the
// network indication.
void status_led_set_relay(bool on);

#ifdef __cplusplus
}
#endif
