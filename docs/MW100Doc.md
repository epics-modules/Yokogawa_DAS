---
layout: default
title: MW100
nav_order: 3
---

# MW100 Support
{: .no_toc}

## Table of contents
{: .no_toc .text-delta}

- TOC
{:toc}

## Overview

This module provides support for the Yokogawa MW100 Digital Acquisition
Unit. The MW100 communicates over Ethernet using a proprietary TCP
protocol and has a modular design that allows mixing DAC, ADC, relay,
TTL, and other I/O modules.

{: .important }
> The driver is written for **Linux** IOCs where dynamic memory
> allocation is not a problem. Other platforms may work but are not
> tested. The driver assumes the **/M1 math option** is present.

The MW100 is an industrial device primarily used for monitoring, so its
design is not optimized for high-performance operation. The system
supports reading Modbus channels over serial and network, performing
calculations on measurements, setting alarms, and logging data. This
driver exposes a subset of the full device functionality. The MW100 can
be simultaneously controlled by vendor software or its built-in web
server, as it supports multiple simultaneous and independent
connections.

The support has been tested with the following modules: MX110-UNV-M10,
MX115-D05-H10, MX120-VAO-M08, and MX125-MKC-M10.

## Channel Descriptions

The MW100 can handle up to 60 measurement channels, referred to as
channels 001-060. The math option adds 300 computation channels
(A001-A300), 300 communication input channels (C001-C300), and 60
constant channels (K01-K60).

The module bus can have up to 6 modules on it. The maximum number of I/O
channels for the MW100 is 60 (referred to as channels 001-060), with a
maximum of 10 channels per module. Each module has a dedicated slice of
the 60 channels, with the first module owning 001-010, the second owning
011-020, etc. The first module is the one closest to the main module.

## Setting Up the Unit

Due to the complexity of the MW100, the configuration of the unit
currently needs to be done through its web interface. This includes not
only service configuration, but also channel configuration.

{: .note }
> The MW100 has two modes of operation. Configuration must be done while
> in "Settings" mode with computations stopped. All data acquisition is
> done in "Measurements" mode with computations started. The mode can be
> toggled from EPICS or through the native web server.

Upon getting the unit, get it on your network as described in the
manual. Plug your modules into the module bus, noting that each module
location to the left of the main module has an address starting at 1,
and the address increases as you go left.

## Operation

Once the unit is configured and running on the network, the EPICS
support in this module reads data while in measurement mode and reads
calculations when math is enabled.

The default way measurements are done is that there are two records that
trigger reading all of the values on the device: one for input values
(such as ADC and TTL) and another for output values (such as DAC and
relay). By default, both records use a time value in their SCAN fields to
periodically poll values and put them into a cache. The records for
channel values are set to **I/O Intr**, which allows them to be updated
by an interrupt call.

If a measurement record is decoupled from the master poll rates by
setting its SCAN field to a time value, the measurement data associated
with that record will be retrieved with all the associated information
(status and alarms) put into the cache, and the new value will be
returned. However, the associated status and alarm records for the
measurement will return cached values only. This means that to read an
independent channel is to read the measurement value, then forward link
to all the associated records to grab the cached values from that read.

The device calculations, communication inputs, and constants are read by
the pollers. The calculations (which need to be defined through the web
interface) allow the transformation of measurements with the
communication input values and predefined constants. The first 60
calculation channels allow for a long calculation (string length of 120
characters), while the last 240 channels allow for short calculations (8
or 10 characters, depending on firmware version).

When an error happens, the error string is displayed, split into three
40-character string records. The error can be cleared, which also clears
it from the unit's LED display.

### Modules

The analog input modules (110 and 112 series) are of the typical
voltage/current measuring or strain gauge measuring types. The first
type has many configuration options, such as being configured as a
thermocouple reader, giving a result through EPICS as a temperature
value and units.

The binary digital input modules (115 series) can be used to sense a
circuit closure or a voltage level.

The analog output modules (120 series) are only directly controlled by
EPICS if they are put into communication mode ("Comm. Input"). If a
channel is put into the other mode ("Trans"), it is controlled by other
MW100 channels, although the value can still be read by EPICS.

The digital output modules (125 series) act as relays and are typically
used in one of two modes: direct control from EPICS ("Comm. Input") or
controlled by an MW100 alarm ("Alarm").

## Driver Initialization

The EPICS driver for the MW100 is loaded using the `mw100Init` command:

```
mw100Init(instrument_handle, IP_address)
```

| Parameter | Type | Description |
| - | - | - |
| instrument_handle | string | A user-defined string used by records to refer to this MW100 instance. Each MW100 must have a unique handle. |
| IP_address | string | The IP address of the MW100. |

Example:

```bash
mw100Init("hdl", "164.54.54.138")
```

## mw100\_probe Utility

Included with the module is the `mw100_probe` utility, built alongside
the module. This program queries a connected MW100 and automatically
generates a substitutions file (`auto_mw100.substitutions`) and an
autosave request file (`auto_mw100.req`). The substitutions file can be
loaded at boot time or flattened by `msi` into a database file.

To use it, the following values are needed: the IP address of the
instrument; the desired IOC PV prefix (**P**); the desired IOC PV name
(**DAU**); and the handle that will be given to the driver for this
instrument (**HANDLE**). Run `mw100_probe` with no arguments to see
usage help.

The generated substitutions file will have entries for all hardware
channels, all constant channels, and 60 (out of 300) of both the
calculation and communication channels.

## Database Loading

The simplest way to load databases for a populated MW100 is by using
the autogenerated substitutions file from `mw100_probe`.

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
dbLoadTemplate("auto_mw100.substitutions")
```

## Display Files

Autoconverted `.bob` display files are included with the module for use
with Phoebus. Legacy `.adl` files for MEDM may also be available.

## EPICS Databases

The following database files are provided in `yokogawaDasApp/Db/` for
use with the MW100. All database files use the macros **P** (PV
prefix), **DAU** (instrument name), **ADDRESS** (channel address),
and **HANDLE** (driver handle).

| Database File | Description |
| - | - |
| `MW100_system.db` | System-level records: IP address, module information, errors, alarms, polling triggers, operation mode, and computation control. |
| `MW100_MX110_channel.db` | ADC analog input channel (universal type). |
| `MW100_MX112_channel.db` | ADC analog input channel (strain gauge type). |
| `MW100_MX114_channel.db` | Integer digital input channel (counter). |
| `MW100_MX115_channel.db` | Binary digital input channel. |
| `MW100_MX120_channel.db` | DAC analog output channel with tweak support. |
| `MW100_MX125_channel.db` | Relay digital output channel. |
| `MW100_calculation_channel.db` | Calculation channel with expression and alarm support. |
| `MW100_communication_channel.db` | Communication input channel. |
| `MW100_constant_channel.db` | Constant channel. |

## Records

There are currently 7 record types supported: ai, ao, bi, bo, mbbi,
mbbo, and stringin. Record commands determine the record function, and
for commands that can refer to multiple items or locations, addressing
is used. A few commands also require sub-addressing.

### DTYP

When using the supported records with the commands, **DTYP** must be
set to "Yokogawa MW100".

### INP and OUT Format

For each MW100, the handle provided to the driver is used with record
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
| Input | Called after reading all input channels, input modules, and calculation addresses. |
| Output | Called after reading all output channels, output modules, communication addresses, and constant addresses. |
| Status | Called after reading the operational statuses of the MW100. |
| Information | Called after reading MW100 and channel information that cannot be changed during measurement mode. |
| Error | Called after the error state of the MW100 occurs. |

### Addresses

The type of address needed depends on the record. The format of the
addresses comes directly from the MW100.

| Address Type | Range | Description |
| - | - | - |
| Module | 0-5 | Module slot position. |
| Hardware channel | 001-060 | Each of the six possible modules uses at most ten addresses. Module 0 uses 001-010, module 1 uses 011-020, etc. |
| Calculation channel | A001-A300 | Computation channels (requires /M1 math option). |
| Communication channel | C001-C300 | Communication input channels. |
| Constant channel | K01-K60 | Constant channels. |

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

### MODULE\_MODEL
---

Shows the module model number (110, 112, 114, 115, 120, or 125). Linked
to information interrupt.

- *longin record:* Uses module address.

<br>

### MODULE\_CODE
---

Shows the 3-character module code. Linked to information interrupt.

- *stringin record:* Uses module address.

<br>

### MODULE\_SPEED
---

Shows the module speed. Linked to information interrupt.

- *mbbi record:* Uses module address.
  - **0** -- Low
  - **1** -- Medium
  - **2** -- High

<br>

### MODULE\_NUMBER
---

Shows the number of module channels. Linked to information interrupt.

- *longin record:* Uses module address.

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

- *mbbi record:* Uses any hardware or calculation addresses.
  - **0** -- Normal
  - **1** -- Overrange
  - **2** -- Underrange
  - **3** -- Measurement Skip / Computation Off
  - **4** -- Error
  - **5** -- Value Uncertain

<br>

### ALARMS
---

Shows the alarms set for an input channel. Records are linked to either
the input interrupt (input channels) or the output interrupt (output
channels).

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
linked to either the input interrupt (input modules) or the output
interrupt (output modules).

- *mbbi record:* Uses any hardware or calculation addresses, with
  sub-address of 1-4. The possible values for a record are determined
  by the module and channel mode.
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

Shows if an alarm for any channel is set. Linked to input interrupt.

- *bi record:* No address used.

<br>

### ALARM\_ACK
---

Tells the MW100 to acknowledge alarms. This record just needs to be
processed to work.

- *bo record:* No address used.

<br>

### CH\_STATUS
---

Shows the configuration for a channel. Linked to information interrupt.

- *mbbi record:* Uses any hardware or calculation addresses.
  - **0** -- Skip
  - **1** -- Normal
  - **2** -- Differential Input

<br>

### CH\_MODE
---

Shows the operational mode for a channel, which differs by module type
(if applicable). Linked to information interrupt.

- *mbbi record:* Uses hardware addresses for supported modules.

  Series **120** module (DAC):
  - **0** -- Skip
  - **1** -- Transmission
  - **2** -- Communication

  Series **125** module (relay):
  - **0** -- Skip
  - **1** -- Alarm
  - **2** -- Communication
  - **3** -- Media
  - **4** -- Failure
  - **5** -- Error

<br>

### UNIT
---

Shows the engineering unit string for a channel. Linked to information
interrupt.

- *stringin record:* Uses any hardware or calculation addresses.

<br>

### EXPR
---

Shows the expression used for a calculation channel. Linked to
information interrupt.

- *stringin record:* Uses any calculation addresses.

<br>

### IP\_ADDR
---

Shows the IP address for the MW100. Linked to information interrupt.

- *stringin record:* No address used.

<br>

### SETTINGS\_MODE
---

Shows whether the MW100 is in settings mode. Linked to status interrupt.

- *bi record:* No address used.
  - **0** -- False
  - **1** -- True

<br>

### MEASURE\_MODE
---

Shows whether the MW100 is in measurement mode. Linked to status
interrupt.

- *bi record:* No address used.
  - **0** -- False
  - **1** -- True

<br>

### OPMODE
---

Sets the operational mode of the MW100.

- *bo record:* No address used.
  - **0** -- Measurement
  - **1** -- Settings

<br>

### COMPUTE\_MODE
---

Shows whether the MW100 has computations running. Linked to status
interrupt. This can only be true when MEASURE\_MODE is true.

- *bi record:* No address used.
  - **0** -- False
  - **1** -- True

<br>

### COMPUTE\_CMD
---

Sends a command regarding the computational mode of the MW100.

- *mbbo record:* No address used.
  - **0** -- Start
  - **1** -- Stop
  - **2** -- Reset
  - **3** -- Clear

<br>

### INP\_TRIG
---

Triggers reading the input records for the MW100, which then triggers
the input interrupt. This record just needs to be processed to work.
Typically the SCAN field is set to a time interval.

- *bo record:* No address used.

<br>

### OUT\_TRIG
---

Triggers reading the output records for the MW100, which then triggers
the output interrupt. This record just needs to be processed to work.
Typically the SCAN field is set to a time interval.

- *bo record:* No address used.

<br>

### STAT\_TRIG
---

Triggers reading the MW100 status, which then triggers the status
interrupt. If an operational mode change to measurements is seen, it
causes an information read which then triggers the information
interrupt. This record just needs to be processed to work. Typically the
SCAN field is set to a time interval.

- *bo record:* No address used.

<br>

### INFO\_TRIG
---

Triggers reading the information for channels and the MW100 that cannot
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

Clears the MW100 error. This record just needs to be processed to work.

- *bo record:* No address used.
