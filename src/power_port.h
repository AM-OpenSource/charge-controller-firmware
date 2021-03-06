/*
 * Copyright (c) 2017 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef POWER_PORT_H
#define POWER_PORT_H

/** @file
 *
 * @brief Definition of charge controller terminals and internal DC buses
 */

#include <stdbool.h>

class PowerPort;    // forward-declaration

/**
 * DC bus class
 *
 * Stores measurement data and settings necessary for voltage control
 */
class DcBus
{
public:
    /**
     * Measured bus voltage
     */
    float voltage;

    /**
     * Upper voltage boundary where this bus may be used to sink current.
     *
     * This value is the voltage at zero current. Values for other currents are calculated
     * using the droop resistance.
     */
    float sink_voltage_bound;

    /**
     * Lower voltage boundary where this bus may be used to source current.
     *
     * This value is the voltage at zero current. Values for other currents are calculated
     * using the droop resistance.
     */
    float src_voltage_bound;

    /**
     * Droop resistance to adjust voltage bounds depending on actual current
     *
     * control voltage = nominal voltage - droop_res * current
     */
    float droop_res;

    /**
     * Pointer to current measurement that is used to determine the droop. This is typically the
     * battery or nanogrid terminal.
     */
    float *ref_current;

    /**
     * Available additional current towards the DC bus until limits of the connected ports are
     * reached
     */
    float sink_current_margin;

    /**
     * Available additional current from the DC bus until limits of the connected ports are reached
     * (has a negative sign)
     */
    float src_current_margin;

    inline float droop_voltage(float v0)
    {
        if (ref_current != nullptr) {
            return v0 - droop_res * (*ref_current);
        }
        else {
            return v0;
        }
    }
};

/**
 * Power Port class
 *
 * Stores current measurements and limits for external terminals or internal ports
 *
 * The signs follow the passive sign convention. Current or power from the considered system or
 * circuit towards an external device connected to the port has a positive sign. For all terminals,
 * the entire charge controller is considered as the system boundary and acts as a source or a sink.
 * For internal sub-circuits, e.g. the DC/DC converter circuit defines the sub-system boundaries.
 *
 *    -----------------
 *    |               |    >> positive current/power direction
 *    |               o---->----
 *    |               |     +  |
 *    |  considered   |       | | external device: battery / solar panel / load / DC grid
 *    |   system or   |       | |
 *    |  sub-circuit  |       | | (the port should be named after the external device)
 *    |               |     -  |
 *    |               o---------
 *    |               |
 *    -----------------
 *
 * Examples:
 * - Charging a connected battery has a positive sign, as current flows from the charge controller
 *   into the battery, i.e. the battery acts as a sink.
 * - Power from a solar panel (power source) has a negative sign, as the charge controller acts as
 *   the sink and power flows from the external device into the charge controller.
 * - A DC/DC converter in buck mode results in a positive current flow at the low voltage side and
 *   a negative current at the high voltage side. The system boundary is the DC/DC sub-circuit,
 *   which sources current from the high side port and sinks it through the low side port.
 */
class PowerPort
{
public:
    /**
     * Each power port is connected to a DC bus, containing relevant voltage information
     */
    DcBus *bus;

    /**
     * Measured current through this port (positive sign = current into the external device)
     */
    float current;

    /**
     * Product of port current and bus voltage
     */
    float power;

    /**
     * Maximum positive current (valid values >= 0.0)
     */
    float pos_current_limit;

    /**
     * Maximum negative current (valid values <= 0.0)
     */
    float neg_current_limit;

    /**
     * Cumulated energy in positive current direction since last counter reset (Wh)
     */
    float pos_energy_Wh;

    /**
     * Cumulated energy in negative current direction since last counter reset (Wh)
     */
    float neg_energy_Wh;

    /**
     * Constructor assigning the port to a DC bus
     *
     * @param dc_bus The DC bus this port is assigned to
     * @param assign_ref_current defines if the bus ref_current should point to the current
     *                           of this port (must be true for at least one port)
     */
    PowerPort(DcBus *dc_bus, bool assign_ref_current = false) :
        bus(dc_bus)
    {
        if (assign_ref_current) {
            bus->ref_current = &current;
        }
    }

    /**
     * Initialize power port for solar panel connection
     */
    void init_solar();

    /**
     * Initialize power port for nanogrid connection
     */
    void init_nanogrid();

    /**
     * Energy balance calculation for power port
     *
     * Must be called exactly once per second, otherwise energy calculation gets wrong.
     */
    void energy_balance();

    /**
     * Sets current limits for control of the bus voltage
     *
     * This function has to be called using the port defining the bus control targets, i.e.
     * the battery, the solar panel or the DC grid.
     */
    void update_bus_current_margins();
};

#endif /* POWER_PORT_H */
