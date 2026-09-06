#pragma once

// Plug's WiFi-status LED (PIN_LED in board_pins.h), repurposed here as the
// Matter network/commissioning indicator. This is the only software-driven
// LED this firmware writes — the plug's other LED (measured on the P26/relay
// net in the original CB2S wiring) sits on the relay drive net in hardware
// and follows relay state without a GPIO of its own; see relay.cpp.
//
// Polarity is UNVERIFIED — confirm on the bench (README's Verification
// section) before relying on "off" meaning what you expect.

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

void status_led_set(status_led_state_t state);

#ifdef __cplusplus
}
#endif
