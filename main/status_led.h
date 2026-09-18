#pragma once

// Three LEDs show this plug's state, each with a distinct job.
//
// 1. PIN_LED — the plug's own WiFi-status LED on the CB2S footprint. This is
//    the only indicator visible once the enclosure is closed, so it is the
//    user-facing one, and it shows different things in different phases:
//
//      boot           solid on
//      commissioning  blinking (pairing window open)
//      paired / OK    follows the relay — on when the load is on
//      error          off (with the onboard RGB red)
//
//    The handover happens at STATUS_LED_OK and is tracked by
//    sRelayOwnsPlugLed in the .cpp. The reasoning: before pairing there is no
//    meaningful relay state to show (the relay is held open through boot, and
//    on USB-only bench power the coil cannot click at all) and a pairing cue
//    is the only thing a user can act on; afterwards, relay state is what a
//    mains plug's indicator is for. Error reclaims the LED so a stuck-on
//    relay cannot mask the error cue.
//
//    The plug's other LED (measured on the P26/relay net in the original CB2S
//    wiring) sits on the relay drive net in hardware and follows relay state
//    without a GPIO of its own; see relay.cpp.
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
// PIN_LED is driven ACTIVE-LOW: this plug ties the LED's anode to 3.3 V and
// its cathode to the GPIO, so the pin sinks to light it. The two onboard LEDs
// are the opposite, active-high, which is how the SuperMini wires them — so
// "on" is 0 for the plug's LED and 1 for the onboard pair. Don't unify them.

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

// Drives the network/commissioning indication on the onboard RGB LED
// (PIN_RGB_LED), and on the plug's LED (PIN_LED) except in the paired/OK
// state, where PIN_LED is handed over to status_led_set_relay().
void status_led_set(status_led_state_t state);

// Drives relay state onto the onboard yellow LED (PIN_ONBOARD_LED) always,
// and onto the plug's LED (PIN_LED) once paired. Safe to call in any phase:
// before the handover it just records the state, so the commissioning blink
// is left intact and the LED still comes up at the right level at handover.
void status_led_set_relay(bool on);

#ifdef __cplusplus
}
#endif
