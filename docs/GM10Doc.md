---
layout: default
title: GM10
nav_order: 4
---

# GM10 Support
{: .no_toc}

## Table of contents
{: .no_toc .text-delta}

- TOC
{:toc}

## Overview

This module provides support for the Yokogawa GM10 Digital Acquisition
System. The GM10 communicates over Ethernet using a proprietary TCP
protocol and has a modular design that allows mixing DAC, ADC, relay,
TTL, and other I/O modules.

{: .important }
> The driver is written for **Linux** IOCs where dynamic memory
> allocation is not a problem. Other platforms may work but are not
> tested. The **/E1 Ethernet option** is mandatory. The **/MT math
> option** is highly recommended and the **/MC communication input
> option** is recommended as it allows the GM10 to connect to Modbus
> devices.

{: .note }
> The amount of available channels for certain channel types depends
> on the memory option (GM10-1 or GM10-2). Support for chaining
> expansion units is not included in this driver.

The GM10 is an industrial device primarily used for monitoring, so its
design is not optimized for high-performance operation. The system
supports reading Modbus channels over serial and network, performing
calculations on measurements, setting alarms, logging data, and
controlling relays. This driver exposes a subset of the full device
functionality. The GM10 can be simultaneously controlled by vendor
software or its built-in web server, as it supports multiple
simultaneous and independent connections.

The support has been tested with the following modules: GX90XA-10-U2,
GX90YA-04-C1, and GX90WD-0806-01. All modules are supported except the
PID module GX90UT-02-11.

## Channel Descriptions

As chaining expansion units is not supported, the GM10 has a measurement
channel address space of 0001-0999, supporting up to 100 channels. The
/MT math option adds either 100 or 200 computation channels: A001-A100
for GM10-1 and A001-A200 for GM10-2. The /MC communications option adds
either 300 or 500 communication input channels: C001-C300 for GM10-1 and
C001-C500 for GM10-2. There are 100 channels for constants (K001-K100)
and 100 channels for variable constants (W001-W100).

The module bus can have up to 10 modules on it. Each of the possible ten
modules has an address slice like 0X01-0X99 (where X is 0 to 9). The
modules are numbered, starting at 0, in the order they are attached to
the GM10 module.

## Setting Up the Unit

Due to the complexity of the GM10, the configuration of the unit
currently needs to be done through its web interface. This includes not
only service configuration, but also channel configuration.

{: .note }
> Configuration must be done while the "Recording" and "Computing"
> modes are disabled. Measuring the channels does not stop
> configuration. The modes can be toggled from EPICS or through the
> native web server.

Upon getting the unit, get it on your network as described in the
manual. Plug your modules into the module bus, noting that each module
location to the left of the main module has an address starting at 0,
and the address increases as you go left.

## Operation

Once the unit is configured and running on the network, the EPICS
support in this module constantly reads data and computes calculations
when math is enabled.

The default way measurements are done is that there is a record that
triggers reading all of the channel values on the device: input, output,
math, and communication. By default, the records use a time value in
their SCAN fields to periodically poll values and put them into a cache.
The records for channel values are set to **I/O Intr**, which allows
them to be updated by an interrupt call.

If a measurement record is decoupled from the master poll rates by
setting its SCAN field to a time value, the measurement data associated
with that record will be retrieved with all the associated information
(status and alarms) put into the cache, and the new value will be
returned. However, the associated status and alarm records for the
measurement will return cached values only. This means that to read an
independent channel is to read the measurement value, then forward link
to all the associated records to grab the cached values from that read.
A similar polling record is used for reading the constants and variable
constants.

The calculations (which need to be defined through the web interface)
allow the transformation of measurements with the communication input
values and predefined constants. The calculation channels allow for
expression strings of 120 characters.

When an error happens, the error string is displayed, split into three
40-character string records. The error can be cleared, which also clears
it from the unit's LED display.

### Modules

The GX90XA analog input modules are of the typical voltage/current
measuring type. They have many configuration options, such as being
configured as a thermocouple reader, giving a result through EPICS as a
temperature value and units.

The GX90YA analog output modules are only directly controlled by EPICS
if they are put into manual mode. If a channel is put into retransmit
mode, it is controlled by other GM10 channels, although the value can
still be read by EPICS. These channels do not provide a voltage, but
either a 0-20 mA or 4-20 mA current. To provide a voltage, use a
resistor of known resistance and use the voltage that occurs across the
resistor for a given current.

The GX90XD and GX90WD binary digital input modules can be used to sense
a circuit closure or a voltage level.

The GX90YD and GX90WD digital output modules act as relays and are
typically used in one of two modes: direct control from EPICS ("Manual")
or controlled by a GM10 alarm ("Alarm").

## Driver Initialization

The EPICS driver for the GM10 is loaded using the `gm10Init` command:

```
gm10Init(instrument_handle, IP_address)
```

| Parameter | Type | Description |
| - | - | - |
| instrument_handle | string | A user-defined string used by records to refer to this GM10 instance. Each GM10 must have a unique handle. |
| IP_address | string | The IP address of the GM10. |

Example:

```bash
gm10Init("hdl", "164.54.52.228")
```

## gm10\_probe Utility

Included with the module is the `gm10_probe` utility, built alongside
the module. This program queries a connected GM10 and automatically
generates a substitutions file (`auto_gm10.substitutions`) and an
autosave request file (`auto_gm10.req`). The substitutions file can be
loaded at boot time or flattened by `msi` into a database file.

To use it, the following values are needed: the IP address of the
instrument; the desired IOC PV prefix (**P**); the desired IOC PV name
(**DAU**); and the handle that will be given to the driver for this
instrument (**HANDLE**). Run `gm10_probe` with no arguments to see
usage help.

The generated substitutions file will have entries for all hardware
channels, as well as 100 each of the constant, variable constant,
calculation, and communication channels.

## Database Loading

The simplest way to load databases for a populated GM10 is by using
the autogenerated substitutions file from `gm10_probe`.

The substitutions file references database files using a relative path
based on the **YOKOGAWA\_DAS** environment variable, which must be set to
the top of this module's installed directory. The synApps distribution
sets this automatically, but it can be set manually in the startup
script before loading the substitutions file:

```bash
epicsEnvSet("YOKOGAWA_DAS", "/my/path/to/YOKOGAWA_DAS")
```

To load the substitutions file in the startup script:

```bash
dbLoadTemplate("auto_gm10.substitutions")
```

## Display Files

Autoconverted `.bob` display files are included with the module for use
with Phoebus. Legacy `.adl` files for MEDM may also be available.

## EPICS Databases

The following database files are provided in `yokogawaDasApp/Db/` for
use with the GM10. All database files use the macros **P** (PV
prefix), **DAU** (instrument name), **ADDRESS** (channel address),
and **HANDLE** (driver handle).

| Database File | Description |
| - | - |
| `GM10_system.db` | System-level records: IP address, module information, errors, alarms, polling triggers, recording mode, and computation control. |
| `GM10_analog_input_channel.db` | Analog input channel (GX90XA modules). |
| `GM10_analog_output_channel.db` | Analog output channel (GX90YA modules) with mode and tweak support. |
| `GM10_digital_input_channel.db` | Digital input channel (GX90XD/GX90WD modules). |
| `GM10_digital_output_channel.db` | Digital output channel (GX90YD/GX90WD modules) with mode support. |
| `GM10_pulse_input_channel.db` | Pulse/counter input channel. |
| `GM10_calculation_channel.db` | Calculation channel with expression and alarm support. |
| `GM10_communication_channel.db` | Communication channel with alarm support. |
| `GM10_constant_channel.db` | Constant channel. |
| `GM10_varconstant_channel.db` | Variable constant channel. |

## Records

There are currently 7 record types supported: ai, ao, bi, bo, mbbi,
mbbo, and stringin. Record commands determine the record function, and
for commands that can refer to multiple items or locations, addressing
is used. A few commands also require sub-addressing.

### DTYP

When using the supported records with the commands, **DTYP** must be
set to "Yokogawa GM10".

### INP and OUT Format

For each GM10, the handle provided to the driver is used with record
device support. The general format of the **INP** or **OUT** field is
one of the following:

```
@handle command
@handle command:address
@handle command:address.sub_address
```

### Interrupts

The input records support interrupt scan mode (I/O Intr). There are
several interrupts used. The first four interrupts can be indirectly
invoked after causing a read with their TRIG commands, although only the
first three are typically necessary, as an information read is called
when the status read notices a mode change into measurements mode.

| Interrupt | Description |
| - | - |
| Channels | Called after reading all channels. |
| Miscellaneous | Called after reading all constants and variable constants. |
| Status | Called after reading the operational statuses of the GM10. |
| Information | Called after reading GM10 and channel information that cannot be changed during measurement mode. |
| Error | Called after the error state of the GM10 occurs. |

### Addresses

The type of address needed depends on the record. The format of the
addresses comes directly from the GM10.

| Address Type | Range | Description |
| - | - | - |
| Module | 0-9 | Module slot position. |
| Hardware channel | 0001-0999 | Each of the ten possible modules uses at most 99 addresses. Module 3 with ten channels uses 0301-0310, etc. |
| Calculation channel | A001-A100 or A001-A200 | Computation channels (depends on memory option). |
| Communication channel | C001-C300 or C001-C500 | Communication input channels (depends on memory option). |
| Constant channel | K001-K100 | Constant channels. |
| Variable constant channel | W001-W100 | Variable constant channels. |

## Commands

### MODULE\_STRING
---

Shows the module identification string for a location, or "empty" if no
module exists. Linked to information interrupt.

- *stringin record:* Uses module address.

<br>

### MODULE\_PRESENCE
---

Shows whether a module is at a location. Linked to information
interrupt.

- *bi record:* Uses module address.
  - **0** -- Not present
  - **1** -- Present

<br>

### VAL
---

Sends or reads a numeric value for a channel. Input records are linked
to either the input interrupt (input channels) or the output interrupt
(output channels).

This command is associated with several record types:

- *bi record:* Uses hardware addresses for binary digital input and
  output modules.
- *bo record:* Uses hardware addresses for binary digital output
  modules.
- *longin record:* Uses hardware addresses for integer digital input
  modules.
- *ai record:* Uses hardware addresses for analog input and output
  modules, as well as calculation, communication, and constant
  addresses.
- *ao record:* Uses hardware addresses for analog output modules, as
  well as calculation, communication, and constant addresses.

<br>

### VAL\_STATUS
---

Shows any special status for a value read from a channel. Records are
linked to either the input interrupt (input channels) or the output
interrupt (output channels).

- *mbbi record:* Uses any hardware, calculation, or communication
  addresses.
  - **0** -- Normal
  - **1** -- Skip
  - **2** -- Positive Overrange
  - **3** -- Negative Overrange
  - **4** -- Positive Burnout
  - **5** -- Negative Burnout
  - **6** -- A/D Conversion Error
  - **7** -- Invalid Data
  - **16** -- Math Result NAN
  - **17** -- Communication Error
  - **32** -- Unknown

<br>

### ALARMS
---

Shows the alarms set for an input channel. Records are linked to the
channel interrupt.

- *mbbi record:* Uses any hardware or calculation addresses. Each bit
  of the 4-bit value corresponds to the status of an alarm, as multiple
  alarms can occur simultaneously. Bit 0 is the least significant bit.
  - **bit 0** -- Alarm 1
  - **bit 1** -- Alarm 2
  - **bit 2** -- Alarm 3
  - **bit 3** -- Alarm 4

  Bit values:
  - **0** -- Alarm off
  - **1** -- Alarm on

<br>

### ALARM
---

Shows the status for a particular alarm on an input channel. Records are
linked to the channel interrupt.

- *mbbi record:* Uses any hardware, calculation, or communication
  addresses, with sub-address of 1-4. The possible values for a record
  are determined by the module and channel mode.
  - **0** -- No Alarm
  - **1** -- High Limit
  - **2** -- Low Limit
  - **3** -- Differential High Limit
  - **4** -- Differential Low Limit
  - **5** -- Rate-of-Change High Limit
  - **6** -- Rate-of-Change Low Limit
  - **7** -- Delay High Limit
  - **8** -- Delay Low Limit

<br>

### ALARM\_FLAG
---

Shows if an alarm for any channel is set. Linked to channel interrupt.

- *bi record:* No address used.

<br>

### ALARM\_ACK
---

Tells the GM10 to acknowledge alarms. This record just needs to be
processed to work.

- *bo record:* No address used.

<br>

### CH\_STATUS
---

Shows the configuration for a channel. Linked to information interrupt.

- *mbbi record:* Uses any hardware, calculation, or communication
  addresses.
  - **0** -- Skip
  - **1** -- Normal
  - **2** -- Differential Input

<br>

### CH\_MODE
---

Shows the operational mode for a channel, which differs by channel type.
Linked to information interrupt.

- *mbbi record:* Uses hardware addresses for supported modules.

  Analog Output Channel:
  - **0** -- Skip
  - **1** -- Retransmit
  - **2** -- Manual

  Digital Output Channel:
  - **0** -- Alarm
  - **1** -- Manual
  - **2** -- Failure

<br>

### UNIT
---

Shows the engineering unit string for a channel. Linked to information
interrupt.

- *stringin record:* Uses any hardware, calculation, or communication
  addresses.

<br>

### EXPR
---

Shows the expression used for a calculation channel. Linked to
information interrupt.

- *stringin record:* Uses any calculation addresses.

<br>

### IP\_ADDR
---

Shows the IP address for the GM10. Linked to information interrupt.

- *stringin record:* No address used.

<br>

### SETTINGS
---

Shows the status of the GM10 settings. Linked to status interrupt.

- *bi record:* No address used.
  - **0** -- Frozen
  - **1** -- Unfrozen

<br>

### RECORDING\_MODE
---

Shows or sets whether the GM10 is in recording mode. Linked to status
interrupt.

- *bi record:* No address used.
  - **0** -- Running
  - **1** -- Stopped

- *bo record:* No address used.
  - **0** -- Start
  - **1** -- Stop

<br>

### COMPUTE\_MODE
---

Shows whether the GM10 has computations running. Linked to status
interrupt.

- *bi record:* No address used.
  - **0** -- Running
  - **1** -- Stopped

<br>

### COMPUTE\_CMD
---

Sends a command regarding the computational mode of the GM10.

- *mbbo record:* No address used.
  - **0** -- Start
  - **1** -- Stop
  - **2** -- Reset
  - **3** -- Clear

<br>

### CHAN\_TRIG
---

Triggers reading the channel records for the GM10, which then triggers
the channel interrupt. This record just needs to be processed to work.
Typically the SCAN field is set to a time interval.

- *bo record:* No address used.

<br>

### CONST\_TRIG
---

Triggers reading the constant records for the GM10, which then triggers
the constant interrupt. This record just needs to be processed to work.
Typically the SCAN field is set to a time interval.

- *bo record:* No address used.

<br>

### STAT\_TRIG
---

Triggers reading the GM10 status, which then triggers the status
interrupt. If an operational mode change to measurements is seen, it
causes an information read which then triggers the information
interrupt. This record just needs to be processed to work. Typically the
SCAN field is set to a time interval.

- *bo record:* No address used.

<br>

### INFO\_TRIG
---

Triggers reading the information for channels and the GM10 that cannot
be changed during measurement mode. This command does not need to be
used if the STAT\_TRIG command is called periodically. This record just
needs to be processed to work.

- *bo record:* No address used.

<br>

### ERROR\_FLAG
---

Shows if an error has happened. Linked to error interrupt.

- *bi record:* No address used.

<br>

### ERROR
---

Retrieves one of the three error strings used to return vendor-supplied
error messages (broken into 40-character lengths). Linked to error
interrupt.

- *stringin record:* Uses an address of 1-3.

<br>

### ERROR\_CLEAR
---

Clears the GM10 error. This record just needs to be processed to work.

- *bo record:* No address used.
