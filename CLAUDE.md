# esp32h2-matter-plug

Two things live in this repo:

1. **Reverse-engineering notes** (`README.md`, `doc/`) for a Tuya CB2S
   (BK7231N) energy-metering smart plug — measured pinout, BL0937
   calibration block, flash layout, stock Tuya schema.
2. **A Matter firmware** (`main/`, ESP-IDF) that replaces the CB2S module
   with an ESP32-H2 SuperMini, turning the plug into an On/Off Plug-in Unit
   with live power/energy metering over Matter-over-Thread.

   When porting anything from the sibling repos below, check it against
   `main/board_pins.h` and `doc/h2-supermini-pinmap.md` — neither ancestor
   runs on this SoC, so their pins, console UART, and radio setup do not
   carry over.

The firmware is a **port**, not a from-scratch build. Its two ancestors:

- **`uascent-matter`** (sibling repo, nRF52840/Zephyr) — the *application*.
  BL0937 driver algorithm, ElectricalPowerMeasurement/
  ElectricalEnergyMeasurement Matter cluster wiring, relay/button/LED,
  over-power trip. Hardware-verified against real mains on a *different*
  plug (Uascent UAM023-based, not this CB2S one).
- **`esp32c6-radar-demo-matter`** (sibling repo, ESP-IDF/esp_matter) — the
  *platform*. Commissioning flow, button handling, sdkconfig, build/flash
  workflow, the `tools/spake2p_verifier.py` pairing-code generator. Note that
  repo is an ESP32-C6 project, so its GPIO and radio specifics do not
  transfer to this one even though the esp_matter scaffolding does.

## Build

```sh
source ~/.espressif/v6.0.3/esp-idf/export.sh
idf.py set-target esp32h2
idf.py build
idf.py flash monitor
```

Matter builds are memory-hungry with LTO and can get OOM-killed at default
parallelism on a constrained machine. Pass
`-- -DCMAKE_JOB_POOLS="compile=4;link=1"` to `idf.py build` if that happens.

Give this device its own pairing code before flashing (see
`main/chip_project_config.h`):

```sh
python3 tools/spake2p_verifier.py <passcode>
```

## Hardware — pin roles are NOT the same as the sibling project

**Do not port `uascent-matter`'s board overlay or pin numbers.** The two
donor plugs place BL0937/relay/LED on almost inverted pins (the left two
columns are the *plug-side* nets, unchanged by the board swap):

| Signal | Uascent (UAM023, sibling repo) | This plug (CB2S) |
|---|---|---|
| BL0937 `CF` | P24 | **P7** |
| BL0937 `CF1` | P26 | **P6** |
| BL0937 `SEL` | P8 | **P24** |
| Relay | P6 | **P26** |
| LED | P7 | **P8** |
| Button | RX1 (P10) | RX1 (P10) — same convention |

`main/board_pins.h` is the single source of truth for this plug's
SuperMini-side GPIO assignments; the pad-level mapping is in README.md's
"Wiring" section, and the replacement board's own pinout is in
`doc/h2-supermini-pinmap.md`.

The SuperMini labels its headers with raw GPIO numbers, so a pad's silkscreen
number *is* the GPIO number. Pins avoided, and why: GPIO23/24 are the ESP32-H2's console UART0
(RX/TX, silkscreened `RX`/`TX`), GPIO26/27 are `USB_D-`/`USB_D+`, and GPIO2/3/
8/9/25 are strapping pins. GPIO8 and GPIO9 are used anyway, but only as the
module's own RGB LED and BOOT button, which the module already wires that way.

**`P10` is the pad silkscreened `RX1` on this module** — confirmed against
the plug's schematic, not a rework or jumper. A reader looking at the CB2S
drawing would otherwise expect a UART line at that pad.

## No esp_matter patch needed (unlike the sibling project)

`esp32c6-radar-demo-matter`'s root `CMakeLists.txt` patches
`managed_components/espressif__esp_matter` after every fresh checkout,
because that project's OccupancySensing cluster keeps its setter private
(`attribute::update()` silently no-ops for a registered cluster's reads).

**That problem does not apply here — do not port that patch machinery.**

Both measurement clusters register the same way in this esp_matter version
(1.6.0), and neither self-registers. **A CHIP `Instance` is not enough on its
own**: constructing one and calling `Init()` registers an
`AttributeAccessInterface`, but without a matching esp_matter `cluster_t` the
endpoint's descriptor never advertises the cluster, so no controller ever
discovers or subscribes to it — and nothing errors. This firmware shipped with
exactly that bug: EPM had an `Instance` but no
`cluster::electrical_power_measurement::create()` call, and reported no
wattage while logging nothing wrong. `PowerMeasurementInit()`'s
`kRequiredServerClusters` loop now fails loudly on that.

`ElectricalPowerMeasurement` is created by
`esp_matter::cluster::electrical_power_measurement::create()` with
`PlugPowerDelegate` (`main/power_measurement.cpp`) passed as
`config_t::delegate`. That delegate pointer is what arms
`ElectricalPowerMeasurementDelegateInitCB`, which constructs the CHIP
`Instance`, calls `Init()` on it, and owns its lifetime via a shutdown
callback — so `power_measurement.cpp` must **not** construct an `Instance`
itself (two would double-register the same `AttributeAccessInterface`), and
the delegate has static storage so it outlives `create_endpoints()`. The
cluster's optional-attribute set is derived from which optional attributes
actually exist on it, so declaring exactly `rms_voltage` and `rms_current` is
how this endpoint says "AC RMS, nothing else" — matching the Zephyr sibling's
`.matter`, and matching the delegate's non-null getters.

`ElectricalEnergyMeasurement` is registered by adding the cluster via
`esp_matter::cluster::electrical_energy_measurement::create()`
(`main/matter_setup.cpp`'s `create_endpoints()`), which triggers an
esp_matter-owned init callback
(`ESPMatterElectricalEnergyMeasurementClusterServerInitCallback`,
in `managed_components/espressif__esp_matter/components/esp_matter/
data_model_provider/clusters/electrical_energy_measurement/integration.cpp`)
that constructs the real cluster automatically once the data model loads.
`power_measurement.cpp` then talks to it purely through that integration
header's free functions (`GetClusterInstance()`, `SetMeasurementAccuracy()`,
`NotifyCumulativeEnergyMeasured()`) — no manual `AttrAccess` construction,
which this esp_matter build doesn't even compile CHIP's own
`ElectricalEnergyMeasurementAttrAccess` shim for (link error if you try).

If bumping `espressif/esp_matter` past 1.6.0 ever reintroduces that
constructor, `PowerMeasurementInit()`'s `GetClusterInstance() == nullptr`
check will fail loudly with a clear log line, not silently drop energy
reports — check there first. That check is deliberately separate from the
`kRequiredServerClusters` loop above it: the loop proves esp_matter's
`cluster_t` exists, while `GetClusterInstance()` proves the init callback
actually built the CHIP cluster behind it.

## Calibration — unverified for this unit

`main/app_config.h`'s `METER_MILLI_COUNTS_PER_SEC_PER_*` constants and
`METER_SEL_HIGH_SELECTS_VOLTAGE` are **placeholders copied from
uascent-matter's Uascent unit**, not this CB2S plug's own values. They
exist only so the driver has something to divide by during USB-only
bring-up. Two independent facts make this plug's true calibration unknown:

- This README's "Calibration coefficients" section recovered a *different*
  set of numbers (1.0 / 2.2 / 0.1 / 0.1) from the CB2S's own Tuya config
  block, but those are Tuya-format *multipliers* — a different convention
  from the counts-per-second-per-unit *divisors* `main/bl0937.cpp` expects.
  The conversion between the two has not been worked out.
- Even if it were worked out, those values belong to this specific unit's
  shunt, not a transferable constant.
- SEL polarity (`METER_SEL_HIGH_SELECTS_VOLTAGE`) is likewise unmeasured for
  this plug. Getting it wrong silently swaps voltage and current in every
  reading — it will not error.

Do not trust any power/voltage/current reading from this firmware until
these are re-derived on the bench (README.md's Verification checklist).

## Mains safety

The BL0937's ground sits at mains potential. Never connect USB to the
SuperMini while the plug is connected to mains. Develop with the board USB-powered
and mains disconnected — everything except real power readings works that
way.

## Matter data model

Endpoint 1 = `on_off_plug_in_unit` (0x010A), with `ElectricalPowerMeasurement`
(0x0090) and `ElectricalEnergyMeasurement` (0x0091) added directly onto it
in `create_endpoints()`. Unlike the Zephyr sibling project, there is no ZAP
GUI or hand-maintained `.matter`/`.zap` file to keep in sync — esp_matter
builds the data model programmatically from the `config_t` structs and
`cluster::*::create()` calls in `matter_setup.cpp`.

Those `create()` calls *are* the data model: a cluster with no call is absent
from the endpoint's descriptor no matter what app code does with it (see the
EPM bug above). When adding a cluster here, add its id to
`kRequiredServerClusters` in `main/power_measurement.cpp` too, so a forgotten
`create()` fails at boot instead of silently reporting nothing. The Zephyr
sibling's `src/default_zap/smart_plug.matter` is the reference for what this
endpoint should advertise.

Transport is **Matter over Thread**, FTD (always-on, no ICD/sleep) — see
`sdkconfig.defaults`'s comment for why this differs from the nRF sibling's
MTD+ICD choice. Requires a Thread Border Router on the network to commission.
The ESP32-H2 has no Wi-Fi radio at all, so Thread is not a preference here but
the only option — there is no Wi-Fi fallback to configure or disable.

## Onboard LEDs

The SuperMini has two software-drivable LEDs, and the firmware gives each its
own job rather than time-sharing one (see `main/status_led.h`):

- **RGB LED (GPIO8)** — Matter network state as colour: white/boot,
  blue-blinking/commissioning, green/paired, red/error. A single WS2812 pixel
  driven over RMT via the `espressif/led_strip` managed component. Init failure
  is logged and tolerated, not fatal — it costs only the bench indication.
- **Yellow LED (GPIO13)** — relay state. This matters on USB-only bench power,
  where the relay coil (mains-derived rail) will not physically click, so the
  LED is the only confirmation a controller toggle landed.

The plug's own front-panel LED (`PIN_LED`) is the only one of the three visible
once the enclosure is closed, so it shows different things in different phases:
solid through boot, blinking while the commissioning window is open, and
**following the relay once paired** (on when the load is on), the way a mains
plug's own indicator behaves. Error takes it back — dark plug LED plus red RGB
— so a relay that happens to be on cannot mask the error cue. `status_led.cpp`
derives the handover from the current state (`RelayOwnsPlugLed()`) rather than
latching it separately, and reads relay state from `RelayIsOn()` rather than
keeping its own copy. A third onboard LED, the battery-charge indicator, is
wired to the charger IC and has no GPIO.

There is **no RF antenna switch** on this board — single PCB trace antenna, no
U.FL, and no switch-control GPIOs. There is nothing to select in software, so
do not add an antenna-init step.
