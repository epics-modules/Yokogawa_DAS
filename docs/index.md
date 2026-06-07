---
layout: default
title: Yokogawa DAS
nav_order: 1
---

# Yokogawa DAS
{: .no_toc}

## Table of contents
{: .no_toc .text-delta}

- TOC
{:toc}

## Overview

The Yokogawa\_DAS module provides EPICS support for two Yokogawa data
acquisition systems:

- **[MW100](MW100Doc.md)** -- Digital Acquisition Unit
- **[GM10](GM10Doc.md)** -- Digital Acquisition System

Both instruments use a modular design that allows mixing ADC, DAC,
relay, TTL, and other I/O modules on a shared module bus. They
communicate over Ethernet using a proprietary TCP protocol.

{: .note }
> This module is part of the
> [synApps](https://www.aps.anl.gov/BCDA/synApps) collection.

## Architecture

The module implements custom C driver and device support for each
instrument. The architecture follows the same pattern for both the
MW100 and GM10:

- **Driver layer** (`drvMW100` / `drvGM10`) -- Manages TCP socket
  connections to the instrument, sends commands, parses responses,
  and maintains a data cache of all channel values, statuses, and
  alarms.
- **Device support layer** -- Provides EPICS device support for
  eight record types: ai, ao, bi, bo, mbbi, mbbo, longin, and
  stringin. Records use `DTYP` values of "Yokogawa MW100" or
  "Yokogawa GM10" with `INST_IO` link type.
- **Polling and interrupts** -- Dedicated polling records
  periodically trigger bulk reads from the instrument. Channel
  records use I/O Intr scanning to update from the driver cache
  when new data arrives.
- **Probe utilities** (`mw100_probe` / `gm10_probe`) -- Standalone
  host programs that query a connected instrument and automatically
  generate EPICS substitution files and autosave request files
  matching the installed hardware configuration.

## Dependencies

This module requires:

- [EPICS base](https://epics-controls.org/) 3.14.12 or later

{: .important }
> The driver uses POSIX socket APIs and dynamic memory allocation.
> It is written for **Linux** IOCs. Other platforms may work but are
> not tested.

## Supported Hardware

### MW100

Up to 6 I/O modules with 10 channels each (60 hardware channels).
With the /M1 math option: 300 computation, 300 communication, and
60 constant channels.

Supported module families: MX110 (analog input), MX112 (strain
gauge input), MX114 (integer digital input), MX115 (binary digital
input), MX120 (analog output), MX125 (relay digital output).

### GM10

Up to 10 I/O modules with up to 99 channels each (up to 100
hardware channels). With /MT math and /MC communication options:
up to 200 computation, 500 communication, 100 constant, and 100
variable constant channels (quantities depend on memory option
GM10-1 or GM10-2).

Supported module families: GX90XA (analog input), GX90YA (analog
output), GX90XD/GX90WD (digital I/O).
