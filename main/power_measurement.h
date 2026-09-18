/*
 * ElectricalPowerMeasurement (0x0090) and ElectricalEnergyMeasurement
 * (0x0091) server wiring for the plug endpoint.
 *
 * Ported from uascent-matter/src/power_measurement.{h,cpp} (nRF52840/Zephyr,
 * hardware-verified). The esp_matter SDK exposes the same underlying CHIP
 * cluster APIs the nRF build used directly (esp_matter vendors the same CHIP
 * SDK, not a reimplementation) — confirmed present in
 * managed_components/espressif__esp_matter:
 *   - electrical_power_measurement: Delegate/Instance pattern
 *     (data_model_provider/clusters/electrical_power_measurement/integration.h)
 *   - electrical_energy_measurement: public GetClusterInstance(),
 *     NotifyCumulativeEnergyMeasured(), NotifyPeriodicEnergyMeasured(),
 *     SetMeasurementAccuracy()
 * So this port keeps the nRF version's structure nearly unchanged; only the
 * persistence backend (esp_matter's own NVS-backed KeyValueStoreMgr, same
 * API surface as Zephyr's) and the timebase (esp_timer instead of
 * k_uptime_get()) differ.
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/core/DataModelTypes.h>

#include <cstdint>

/* The ElectricalPowerMeasurement delegate, for matter_setup.cpp to hand to
 * cluster::electrical_power_measurement::create() as config_t::delegate.
 *
 * Must be called BEFORE esp_matter::start(), i.e. during create_endpoints():
 * esp_matter stores this pointer on the cluster and passes it to
 * ElectricalPowerMeasurementDelegateInitCB once the data model loads, which
 * is what constructs and Init()s the CHIP Instance. The returned object has
 * static storage duration, so it is valid from first call until reboot; this
 * file deliberately does not construct an Instance itself (esp_matter owns
 * that one, and a second would double-register the same
 * AttributeAccessInterface).
 *
 * Returns void* because that is config_t::delegate's type; the pointee is a
 * chip::app::Clusters::ElectricalPowerMeasurement::Delegate. */
void *PowerMeasurementGetDelegate(void);

/* Must run AFTER esp_matter::start(), unlike PowerMeasurementGetDelegate():
 * it checks the clusters esp_matter's init callbacks have by then built, and
 * sets EEM's accuracy on the resulting instance. */
CHIP_ERROR PowerMeasurementInit(chip::EndpointId endpoint);

/* Pushes one reading into both clusters: EPM's attributes through the
 * delegate, dirty-marked only when they move past a deadband (see
 * ReportIfMoved() in the .cpp); and EEM's cumulative energy by integrating
 * activePowerMw over the elapsed time since the previous call, also
 * dirty-marked on every call that reports. First call after init only primes
 * the integrator; it reports zero elapsed energy.
 *
 * The cumulative energy total is also periodically written to the KVS (see
 * PersistCumulativeEnergyIfDue() in the .cpp) so it survives a reboot; this
 * function's caller does not need to do anything for that to happen. */
void PowerMeasurementUpdate(int64_t activePowerMw, int64_t rmsVoltageMv, int64_t rmsCurrentMa);
