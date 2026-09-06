#include "relay.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "board_pins.h"

static const char *TAG = "relay";

namespace
{
bool sOn;
} // namespace

void RelayInit(void)
{
	gpio_config_t cfg = {
		.pin_bit_mask = (1ULL << PIN_RELAY),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	ESP_ERROR_CHECK(gpio_config(&cfg));

	// Relay starts open regardless of persisted state; the OnOff cluster's
	// restore path (matter_setup.cpp, after esp_matter::start()) may switch
	// it back on shortly after, once the stack has read the persisted OnOff
	// attribute back out of NVS.
	gpio_set_level(PIN_RELAY, 0);
	sOn = false;
}

void RelaySet(bool on)
{
	gpio_set_level(PIN_RELAY, on ? 1 : 0);

	if (on != sOn) {
		ESP_LOGI(TAG, "Relay %s", on ? "on" : "off");
	}
	sOn = on;

	// This plug has only one software-driven LED (the network/commissioning
	// indicator on PIN_LED, see status_led.h) -- unlike uascent-matter's
	// donor board, this CB2S plug's relay-state LED (if any) sits on the
	// relay drive net itself in hardware and needs no GPIO of its own to
	// follow relay state. No StatusLedSetRelayState() call here as a result;
	// confirm this against the actual board before assuming it.
}

bool RelayIsOn(void)
{
	return sOn;
}
