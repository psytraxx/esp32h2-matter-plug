# ESP32-H2 SuperMini — Pin Map

Transcribed from the board's own pinout drawing ([`h2-supermini.png`](h2-supermini.png)).
The SuperMini labels its headers with **raw GPIO numbers**, so a pad's
silkscreen number is the chip GPIO number directly.

See [`../main/board_pins.h`](../main/board_pins.h) for which of these this
firmware actually uses.

## Header pads

| Pad | Chip pin | Alternate functions | Notes |
|---|---|---|---|
| `5V`  | VBUS   | | Power input/output |
| `GND` |        | | |
| `3V3` | 3V3_OUT| | Power output — also the **input** used to back-feed the board from the plug's rail |
| `TX`  | GPIO24 | | **Console UART0 TX** — leave unused |
| `RX`  | GPIO23 | | **Console UART0 RX** — leave unused |
| `0`   | GPIO0  | FSPIQ | |
| `1`   | GPIO1  | FSPICS0, ADC `A0` | |
| `2`   | GPIO2  | FSPIWP, MTMS, ADC `A1` | **Strapping** |
| `3`   | GPIO3  | FSPIHD, MTDO, ADC `A2` | **Strapping** |
| `4`   | GPIO4  | FSPICLK, MTCK, ADC `A3` | |
| `5`   | GPIO5  | FSPID, MTDI, ADC `A4` | |
| `8`   | GPIO8  | | **Strapping**; wired to the onboard **RGB LED** |
| `9`   | GPIO9  | | **Strapping**; wired to the onboard **BOOT** button |
| `10`  | GPIO10 | | |
| `11`  | GPIO11 | | |
| `12`  | GPIO12 | | |
| `13`  | GPIO13 | | Wired to the onboard **yellow user LED** |
| `14`  | GPIO14 | | |
| `22`  | GPIO22 | | |
| `25`  | GPIO25 | FSPICS3 | **Strapping** |
| `26`  | GPIO26 | **USB_D-** | Carries the USB-C port — leave unused |
| `27`  | GPIO27 | **USB_D+** | Carries the USB-C port — leave unused |

Every GPIO pad above is PWM-capable.

## Onboard indicators and controls

| Item | GPIO | Notes |
|---|---|---|
| RGB LED | GPIO8 | Single addressable WS2812-family pixel, driven over RMT |
| Yellow user LED | GPIO13 | Plain GPIO, active-high |
| Battery charge LED | — | Wired to the charger IC; no GPIO, not software-controllable |
| BOOT button | GPIO9 | Active-low to GND |
| RST button | CHIP_PU | Hardware reset; no GPIO |

## Notes for this design

- **No RF antenna switch.** The board has a single PCB trace antenna, no U.FL
  connector, and no switch-control GPIOs — there is nothing to select in
  software.
- **No Wi-Fi.** The ESP32-H2 is 802.15.4 + BLE only. That suits this project —
  the transport is Matter over Thread — but it does mean Wi-Fi is not
  available as a fallback.
- **Two software-drivable onboard LEDs**, which lets network state and relay
  state each have their own indicator (see
  [`../main/status_led.h`](../main/status_led.h)).
- **Maximum clock is 96 MHz.**
