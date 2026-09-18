#include "status_led.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "led_strip.h"

#include "board_pins.h"

static const char *TAG = "status_led";

namespace
{
TimerHandle_t sBlinkTimer;
bool sBlinkOn;

led_strip_handle_t sRgb;

// Current network state, re-applied to the RGB LED on every blink edge.
status_led_state_t sState = STATUS_LED_BOOT;

struct Rgb
{
    uint8_t r, g, b;
};

// Kept dim on purpose: this is a bare WS2812 a few centimetres from the eye
// on an open bench, and full brightness is genuinely uncomfortable to work
// next to. It also keeps the LED's draw off the AMS1117's budget (README's
// Power note) once the module runs from the plug's own 3.3 V rail.
constexpr uint8_t kLevel = 12;

Rgb ColorFor(status_led_state_t state)
{
    switch (state)
    {
    case STATUS_LED_BOOT:          return {kLevel, kLevel, kLevel}; // white
    case STATUS_LED_COMMISSIONING: return {0, 0, kLevel};           // blue
    case STATUS_LED_OK:            return {0, kLevel, 0};           // green
    case STATUS_LED_ERROR:         return {kLevel, 0, 0};           // red
    }
    return {0, 0, 0};
}

// Active-low: this plug wires the LED's anode to 3.3 V through a resistor and
// its cathode to the GPIO, so the pin has to sink for the LED to light and 0
// lights it. Measured on the bench — driving it active-high left it dark in
// every state.
void SetLevel(bool on)
{
    gpio_set_level(PIN_LED, on ? 0 : 1);
}

// Active-high: this board's yellow user LED sources through the pin.
void SetOnboardLevel(bool on)
{
    gpio_set_level(PIN_ONBOARD_LED, on ? 1 : 0);
}

// `on` is the blink phase: while commissioning the RGB LED blanks between
// pulses, in every other state it stays lit.
void SetRgb(bool on)
{
    if (!sRgb)
    {
        return;
    }

    Rgb c = on ? ColorFor(sState) : Rgb{0, 0, 0};
    led_strip_set_pixel(sRgb, 0, c.r, c.g, c.b);
    led_strip_refresh(sRgb);
}

void BlinkTimerCallback(TimerHandle_t)
{
    sBlinkOn = !sBlinkOn;
    SetLevel(sBlinkOn);
    SetRgb(sBlinkOn);
}

void StopBlink(void)
{
    if (sBlinkTimer)
    {
        xTimerStop(sBlinkTimer, 0);
    }
}

// Onboard RGB LED (PIN_RGB_LED), a single WS2812 pixel driven over RMT.
// Failure here is not fatal — it costs the bench colour indication, while the
// plug's own LED (PIN_LED) and the yellow relay LED keep working — so it logs
// and leaves sRgb null rather than aborting the boot.
void RgbInit(void)
{
    led_strip_config_t strip_cfg = {
        .strip_gpio_num        = PIN_RGB_LED,
        .max_leds              = 1,
        .led_model             = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags                 = {.invert_out = 0},
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src       = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .mem_block_symbols = 0,            // driver default
        .flags         = {.with_dma = 0},
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &sRgb);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Onboard RGB LED init failed (%s) — colour status unavailable",
                 esp_err_to_name(err));
        sRgb = NULL;
        return;
    }
    led_strip_clear(sRgb);
}
} // namespace

void status_led_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PIN_LED) | (1ULL << PIN_ONBOARD_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    SetLevel(false);
    status_led_set_relay(false);

    RgbInit();

    sBlinkTimer = xTimerCreate("led_blink", pdMS_TO_TICKS(500), pdTRUE, NULL, BlinkTimerCallback);
    if (!sBlinkTimer)
    {
        ESP_LOGE(TAG, "Failed to create blink timer");
    }
}

void status_led_set_relay(bool on)
{
    // Has an LED to itself, so it never has to yield to the commissioning
    // indication.
    SetOnboardLevel(on);
}

void status_led_set(status_led_state_t state)
{
    sState = state;

    switch (state)
    {
    case STATUS_LED_BOOT:
        StopBlink();
        SetLevel(true);
        SetRgb(true);
        break;
    case STATUS_LED_COMMISSIONING:
        sBlinkOn = false;
        SetLevel(false);
        SetRgb(false);
        if (sBlinkTimer)
        {
            xTimerStart(sBlinkTimer, 0);
        }
        break;
    case STATUS_LED_OK:
        StopBlink();
        SetLevel(true);
        SetRgb(true);
        break;
    case STATUS_LED_ERROR:
        StopBlink();
        SetLevel(false);
        SetRgb(true); // red stays lit — the plug's LED going dark is the "error" cue
        break;
    }
}
