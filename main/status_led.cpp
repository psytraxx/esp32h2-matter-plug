#include "status_led.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "board_pins.h"

static const char *TAG = "status_led";

namespace
{
TimerHandle_t sBlinkTimer;
bool sBlinkOn;

void SetLevel(bool on)
{
    // Polarity UNVERIFIED for this plug's LED — see status_led.h.
    gpio_set_level(PIN_LED, on ? 1 : 0);
    // XIAO's own onboard LED, mirrored 1:1 with the plug's LED so status is
    // visible without the plug-side LED net wired up.
    gpio_set_level(PIN_ONBOARD_LED, on ? 1 : 0);
}

void BlinkTimerCallback(TimerHandle_t)
{
    sBlinkOn = !sBlinkOn;
    SetLevel(sBlinkOn);
}

void StopBlink(void)
{
    if (sBlinkTimer)
    {
        xTimerStop(sBlinkTimer, 0);
    }
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

    sBlinkTimer = xTimerCreate("led_blink", pdMS_TO_TICKS(500), pdTRUE, NULL, BlinkTimerCallback);
    if (!sBlinkTimer)
    {
        ESP_LOGE(TAG, "Failed to create blink timer");
    }
}

void status_led_set(status_led_state_t state)
{
    switch (state)
    {
    case STATUS_LED_BOOT:
        StopBlink();
        SetLevel(true);
        break;
    case STATUS_LED_COMMISSIONING:
        sBlinkOn = false;
        if (sBlinkTimer)
        {
            xTimerStart(sBlinkTimer, 0);
        }
        break;
    case STATUS_LED_OK:
        StopBlink();
        SetLevel(true);
        break;
    case STATUS_LED_ERROR:
        StopBlink();
        SetLevel(false);
        break;
    }
}
