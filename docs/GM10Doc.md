GM10 Support Documentation {#gm10-support-documentation align="center"}
==========================

This module has support for the Yokogawa GM10 Digital Acquisition
System. It was written for Linux IOCs, where dynamic memory allocation
is not a problem, while others might work as well. The /E1 Ethernet
option is mandatory. The /MT mathematic option is higly recommended,
while the /MC option for communication input channel options is
recommended as it allows the GM10 to connect to Modbus devices. The
amount of available channels for certain channel types depends on the
memory option, GM10-1 or GM10-2. Support for chaining expansion units is
not included in this driver.

This device uses a network interface to communicate, and has a modular
design to allow the mixing of DAC, ADC, relay, TTL, and other modules.
It is also an industrial device primarily used for monitoring, so its
design is not optimized for high performance operation. The system is
actually quite complex, as it can: read modbus channels over both serial
and the network, perform calculations on the measurements, set alarms,
do logging, throw relays, etc. Some subset of the functionality is
supported, and features have been added as needed. However, there is no
attempt made to utilize the full capability of the device from EPICS,
although the device can be simultaneously controlled by vendor software
or the built-in web server, as it supports multiple simultaneous and
independent connections.

I assume that the support works for all modules (other than
GX90UT-02-11), but this has been tested only with the following:
GX90XA-10-U2, GX90YA-04-C1, and GX90WD-0806-01.

Channel descriptions
--------------------

As chaining expansion units is not supported, the GM10 has an a
measurement channel address space of 0001-0999, supporting up to 100
channels. The /MT math option adds either 100 or 200 computation
channels: A001-A100 for GM10-1, and A001-A200 for GM10-2. The /MC
communications option adds either 300 or 500 communication input
channels: C001-C300 for GM10-1, and C001-C500 for GM10-2. There are 100
channels for constants (K001-K100), and 100 channels for variable
constants (W001-W100).

The module bus can have up to 10 modules on it. Each of the possible ten
modules has an address slice like 0X01-0X99 (where X is 0 to 9). The
modules are numbered, starting at 0, in the order the are attached to
the GM10 module.

Setting up the unit
-------------------

Due to the complexity of the GM10, the configuration of the unit
currently needs to be done through its web interface. This includes not
only service configuration, but also channel configuration. The way the
unit works is that there are modes of operation. Any configuration needs
to be done while the \"Recording\" and \"Computing\" modes are disabled,
while measuring the channels doesn\'t stop configuration. The modes can
be toggled from EPICS or through the native web server.

Upon getting the unit, get it on your network, as described in the
manual.

Plug your modules into the module bus, realizing that each module
location to the left of the main module has an address starting at 0,and
increases as you go left.

Operation
---------

Once the unit is configured and running on the network, the EPICS
support in this module will constantly read data, and compute
calculations when math is enabled.

The default way the measurements are done is that there is a record that
trigger reading all of the channel values on the device: input, output,
math, and communication. By default, the records use a time value in
their SCAN fields to periodically poll values and put them into a cache,
and the record for these values are set to **I/O Intr** which allow them
to be updated by an interrupt call. If a measurement record is decoupled
from the master poll rates by setting their SCAN field to a time value,
the measurement data associated with a record will be retrieved, with
all the associated information (status and alarms) put into the cache,
and the new value will be returned; however, the associated status and
alarm records for the measurement will return cached values only. This
means that to do read an independent channel is to read the measurement
value, then forward link to all the associated records to grab the
cached values from that read. A similar polling record is used for
reading the constants and variable constants.

The calculations (which need to be defined through the web interface)
allow the transformation of measurements with the communication input
values and predefined constants. The calculation channels allow for
expression strings of 120 characters.

When an error happpens, the error string is displayed, split into three
40-character string records. The error can be cleared, which also clears
it from the units LED display.

### Modules

The GX90XA analog input modules are of the typical voltage/current
measuring type. It has many configuration options, such as configured as
a thermocouple reader, giving a result through EPICS as a temperature
value and units.

The GX90YA analog output modules are only directly controlled by EPICS
if they are put into manual mode. If a channel is put into the
restransmit mode, it is controlled by other GM10 channels, although the
value can still be read by EPICS. These channels do not provide a
voltage, but either a 0-20 mA or 4-20 mA current. To provide a voltage,
one has to use a resistor of known resistance, and use the voltage that
occurs across the resistor for a given current.

The GX90XD and GX90WD binary digital input modules can be used to sense
a circuit closure or a voltage level.

The GX90YD and GX90WD digital output modules act as relays, and are
typically used in one of two modes. One is direct control from EPICS
(\"Manual\"), and the other is for it to be controlled by a GM10 alarm
(\"Alarm\").

### Driver

The EPICS driver for the GM10 needs to be loaded using the command
gm10Init.

`     gm10Init( instrument handle, IP address)   `

The *instrument handle* is a string, provided by the user, that is used
by records to refer to this particular GM10. For each GM10 loaded, a
different handle needs to be used.

*gm10\_probe*
-------------

Included with the module is the source code for *gm10\_probe* utility,
that will be built alongside the module. This program will generate
automatically a substitutions file **auto\_gm10.substitutions** and an
autosave file **auto\_gm10.req**. The substitutions file that can be
loaded at boot time or flattened by *msi* into a database file.

To use it, one needs values for: the IP address of the instrument; the
desired IOC PV prefix, **P**; the desired IOC PV name, **DAU**; and the
handle that will be given to the driver for this instrument, **HANDLE**.
The help can be seen by running *gm10\_probe* with no arguments.

The resultant substitutions file will have entries for all the hardware
channels, as well as 100 each of the constant, variable constant,
calculation, and communication channels. In the future, an option will
be added to changing the number of communication and calculation
channels included.

Database Loading
----------------

The simplest way to load databases associated with a populated GM10 is
by using the autogenerated substitutions file from *gm10\_probe*.

This substitutions file references the database files using a relative
path off of the **YOKOGAWA\_DAS** variable, which needs to be set to the
top of this module\'s installed directory. The synApps distribution does
this automatically, but it can be done by hand by adding to the startup
script (such as st.cmd) before loading the substitutions file a line
like this:

    epicsEnvSet("YOKOGAWA_DAS", "/my/path/to/YOKOGAWA_DAS")

To load the substitutions file, in the startup script, use

    dbLoadTemplate("auto_gm10.substitutions")

MEDM display files
------------------

**not finished**

Records
-------

There are currently 7 records supported for various purposes: ai, ao,
bi, bo, mbbi, mbbo, and stringin. Record commands are also provided to
determine the record function, and for record commands that can refer to
multiple items or locations, addressing is used; a few commands also
need sub-addressing.

### DTYP format

When using the supported records with the commands, **DTYP** needs to be
set to \"Yokogawa GM10\".

### INP and OUT format

For each GM10, the handle provided to the driver is used with record
device support. The general format of the **INP** or **OUT** field is
one of the following.

-   \"\@handle command\"
-   \"\@handle command:address\"
-   \"\@handle command:address.sub\_address\"

### Interrupts

The input records will let you use interrupt scan mode (I/O Intr), and
there are several interrupts used. The first four interrupts can be
indirectly invoked after causing a read with their TRIG commands,
although only the first three are necessary, as an information read is
typically called when the status read notices a mode change into
measurements mode.

-   **Channels** - This interrupt is called after reading all the
    channels.
-   **Miscellaneous** - This interrupt is called after reading all the
    constants and variable constants.
-   **Status** - This interrupt is called after reading the operational
    statuses of the GM10.
-   **Information** - This interrupt is called after reading the GM10
    and channel information that can\'t be changed during measurement
    mode.
-   **Error** - This interrupt is called after the error state of the
    GM10 occurs.

### Addresses

The type of address needed depends on the record, but here are all the
options. The format of the addresses comes directly from the GM10, and
this is how they are used internally.

-   Module addresses: **0-9**
-   Hardware channel addresses: **0001-0999**\
    Each module (of the ten possible modules) can use at most 99
    hardware addresses. If module 3 has ten channels, it uses addresses
    0301-0310, etc.
-   Calculation channel addresses: **A001-A100** or **A001-A200**
-   Communication channel addresses: **C001-C300** or **C001-C500**
-   Constant channel addresses: **K001-K100**
-   Variable constant channel addresses: **W001-W100**

### Commands

**MODULE\_STRING** - Shows the module identification string for a location, or \"empty\" if one doesn\'t exist. Linked to information interrupt.
:   **stringin record:** Uses module address.

**MODULE\_PRESENCE** - Shows whether a module is at a location. Linked to information interrupt.
:   **bi record:** Uses module address.
    -   **0** - Not present
    -   **1** - Present

**VAL** - Send or read a numeric value for a channel. Input records are linked to either the input interrupt (input channels) or the output interrupt (output channels).
:   This command is associated with several record types.
    -   **bi record:** Uses hardware addresses for binary digital input
        and output modules.
    -   **bo record:** Uses hardware addresses for binary digital output
        modules.
    -   **longin record:** Uses hardware addresses for binary integer
        input modules.
    -   **ai record:** Uses hardware addresses for analog input and
        output modules, as well as calculation, communication, and
        contant addresses.
    -   **ao record:** Uses hardware addresses for analog output
        modules, as well as calculation, communication, and contant
        addresses.

**VAL\_STATUS** - Shows any special status for a value read from a channel. Records are linked to either the input interrupt (input channels) or the output interrupt (output channels).
:   **mbbi record:** Uses any hardware, calculation, or communication
    addresses.
    -   **0** - Normal
    -   **1** - Skip
    -   **2** - Positive Overrange
    -   **3** - Negative Overrange
    -   **4** - Positive Burnout
    -   **5** - Negative Burnout
    -   **6** - A/D Conversion Error
    -   **7** - Invalid Data
    -   **16** - Math Result NAN
    -   **17** - Communication Error
    -   **32** - Unknown

**ALARMS** - Shows the alarms set for an input channel. Records are linked to the channel interrupt.
:   **mbbi record:** Uses any hardware or calculation addresses. Each
    bit of the 4-bit value corresponds to the status of an alarm, as
    multiple alarms can occur simultaneously; bit 0 is defined as the
    least significant bit.
    -   **bit 0** - Alarm 1
    -   **bit 1** - Alarm 2
    -   **bit 2** - Alarm 3
    -   **bit 3** - Alarm 4

    The bit value is defined as the following.
    -   **value 0** - Alarm off
    -   **value 1** - Alarm on

**ALARM** - Shows the status for a particular alarm for an input channel. Records are linked to the channel interrupt.
:   **mbbi record:** Uses any hardware, calculation, or communication
    addresses; with sub-address of 1-4. The value definitions follow,
    although the possible values for a record are actually determined by
    the module and the channel mode.
    -   **0** - No Alarm
    -   **1** - High Limit
    -   **2** - Low Limit
    -   **3** - Differential High Limit
    -   **4** - Differential Low Limit
    -   **5** - Rate-of-Change High Limit
    -   **6** - Rate-of-Change Low Limit
    -   **7** - Delay High Limit
    -   **8** - Delay Low Limit

**ALARM\_FLAG** - Shows if an alarm for any channel is set. Linked to channel interrupt.
:   **bi record:** No address used.

**ALARM\_ACK** - Tells GM10 to acknowledge alarms. This record just needs to be PROC\'d to work.
:   **bo record:** No address used.

**CH\_STATUS** - Shows the configuration for a channel. Linked to information interrupt.
:   **mbbi record:** Uses any hardware, calculation, or communication
    addresses.
    -   **0** - Skip
    -   **1** - Normal
    -   **2** - Differential Input

**CH\_MODE** - Shows the operational mode for a channel, which is different depending on the type. Linked to information interrupt.
:   **mbbi record:** Uses hardware addresses for supported modules.

    Analog Output Channel

    :   -   **0** - Skip
        -   **1** - Retransmit
        -   **2** - Manual

    Digital Output Channel

    :   -   **0** - Alarm
        -   **1** - Manual
        -   **2** - Failure

**UNIT** - Shows the unit for a channel. Linked to information interrupt.
:   **stringin record:** Uses any hardware, calculation, or
    communication addresses.

**EXPR** - Shows the expression used for a calculation channel. Linked to information interrupt.
:   **stringin record:** Uses any calculation addresses.

**IP\_ADDR** - Shows the IP address for the GM10. Linked to information interrupt.
:   **stringin record:** No address used.

**SETTINGS** - Shows the status of the GM10 settings. Linked to status interrupt.
:   **bi record:** No address used.
    -   **0** - Frozen
    -   **1** - Unfrozen

**RECORDING\_MODE** - Shows whether the GM10 is in recording mode. Linked to status interrupt.
:   **bi record:** No address used.
    -   **0** - Running
    -   **1** - Stopped

    **bo record:** No address used.
    -   **0** - Start
    -   **1** - Stop

**COMPUTE\_MODE** - Shows whether the GM10 has computations running. Linked to status interrupt. This can only be true when MEASURE\_MODE is true.
:   **bi record:** No address used.
    -   **0** - Running
    -   **1** - Stopped

**COMPUTE\_CMD** - Send a command regarding the computational mode of the GM10.
:   **mbbo record:** No address used.
    -   **0** - Start
    -   **1** - Stop
    -   **2** - Reset
    -   **3** - Clear

**CHAN\_TRIG** - Trigger reading the channel records for the GM10, which will then trigger the channel interrupt. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.
:   **bo record:** No address used.

**CONST\_TRIG** - Trigger reading the constant records for the GM10, which will then trigger the constant interrupt. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.
:   **bo record:** No address used.

**STAT\_TRIG** - Trigger reading the GM10 status, which will then trigger the status interrupt; if an operational mode change to measurements is seen, it will cause a information read which will then trigger the information interrupt. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.
:   **bo record:** No address used.

**INFO\_TRIG** - Trigger reading the information for channels and the GM10 that can\'t be changed during measurement mode. This command doesn\'t really need to be used if the STAT\_TRIG command is called periodically. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.
:   **bo record:** No address used.

**ERROR\_FLAG** - Shows if an error has happened. Linked to error interrupt.
:   **bi record:** No address used.

**ERROR** - Retrieve one of the three error strings used to return vendor-supplied error messages (broken to 40 character lengths). Linked to error interrupt.
:   **stringin record:** Uses an address of 1-3.

**ERROR\_CLEAR** - Clears the GM10 error. This record just needs to be PROC\'d to work.
:   **bo record:** No address used.

EPICS Databases
---------------

**not finished**

Dohn Arms\
Advanced Photon Source
