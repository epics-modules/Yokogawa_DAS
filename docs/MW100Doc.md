MW100 Support Documentation
===========================

This module has support for the Yokogawa MW100 Digital Acquisition Unit.
It was written for Linux IOCs, where dynamic memory allocation is not a
problem, while others might work as well. It was also written assuming
the /M1 math option is present.

This device uses a network interface to communicate, and has a modular
design to allow the mixing of DAC, ADC, relay, TTL, and other modules.
It is also an industrial device primarily used for monitoring, so its
design is not optimized for high performance operation. The system is
actually quite complex, as it can: read modbus channels over both serial
and the network, perform calculations on the measurements, set alarms,
do logging, etc. Some subset of the functionality is supported, and
features have been added as needed. However, there is no attempt made to
utilize the full capability of the device from EPICS, although the
device can be simultaneously controlled by vendor software or the
built-in web server, as it supports multiple simultaneous and
independent connections.

I assume that the support works for all modules, but this has been
tested only with the following: MX110-UNV-M10, MX115-D05-H10,
MX120-VAO-M08, and MX125-MKC-M10.

Channel descriptions
--------------------

The MW100 can handle up to 60 measurement channels, referred to as
channels 001-060. The math option adds 300 computation channels
(A001-A300), 300 communication input channels (C001-C300), and 60
constant channels (K01-K60).

The module bus can have up to 6 modules on it. The maximum number of I/O
channels for the MW100 is 60 (referred to as channels 001-060), with a
maximum of 10 channels per modules. Each module has a dedicated slice of
the 60 channels, with the first module owning 001-010, the second owning
011-020, etc. The first module is the one closest to the main module.

Setting up the unit
-------------------

Due to the complexity of the MW100, the configuration of the unit
currently needs to be done through its web interface. This includes not
only service configuration, but also channel configuration. The way the
unit works is that there are modes of operation. Any configuration needs
to be done while in the \"Settings\" mode and computations are stopped,
while all data acquisition is done in the \"Meaurements\" mode and the
computations should be started. The mode can be toggled from EPICS or
through the native web server. So to do configuration changes, make sure
the unit is in Settings mode and do the changes through the web server.

Upon getting the unit, get it on your network, as described in the
manual.

Plug your modules into the module bus, realizing that each module
location to the left of the main module has an address starting at 1,and
increases as you go left.

Operation
---------

Once the unit is configured and running on the network, the EPICS
support in this module will work for reading data while in measurement
mode, and reading calculations when math is enabled.

The default way the measurements are done is that there are two records
that trigger reading all of the values on the device, one for input
values (such as ADC and TTL) and another for output values (such as DAC
and relay). By default, both records use a time value in their SCAN
fields to periodically poll values and put them into a cache, and the
record for these values are set to **I/O Intr** which allow them to be
updated by an interrupt call. If a measurement record is decoupled from
the master poll rates by setting their SCAN field to a time value, the
measurement data associated with a record will be retrieved, with all
the associated information (status and alarms) put into the cache, and
the new value will be returned; however, the associated status and alarm
records for the measurement will return cached values only. This means
that to do read an independent channel is to read the measurement value,
then forward link to all the associated records to grab the cached
values from that read.

The device calculations, communication inputs, and constants are read by
the pollers. The calculations (which need to be defined through the web
interface) allow the transformation of measurements with the
communication input values and predefined constants. The first 60
calculation channels allow for a long calculation (string length of 120
characters), while the last 240 channels allow for short calculations (8
or 10 characters, depending on firmware version).

When an error happpens, the error string is displayed, split into three
40-character string records. The error can be cleared, which also clears
it from the units LED display.

### Modules

The analog input modules (110 and 112 series) are of the typical
voltage/current measuring or the strain gauge measuring types. The first
type has many configuration options, such as configured as a
thermocouple reader, giving a result through EPICS as a temperature
value and units.

I have had no access to integer digital input modules (114 series), but
hopefully the support works for them.

The binary digital input modules (115 series) can be used to sense a
circuit closure or a voltage level.

The analog output modules (120 series) are only directly controlled by
EPICS if they are put into communication mode (\"Comm. Input\"). If a
channel is put into the other mode (\"Trans\"), it is controlled by
other MW100 channels, although the value can still be read by EPICS.

The digital output modules (125 series) act as relays, and are typically
used in one of two modes. One is direct control from EPICS (\"Comm.
Input\"), and the other is for it to be controlled by a MW100 alarm
(\"Alarm\").

### Driver

The EPICS driver for the MW100 needs to be loaded using the command
mw100Init.

    mw100Init( instrument handle, IP address)

The *instrument handle* is a string, provided by the user, that is used
by records to refer to this particular MW100. For each MW100 loaded, a
different handle needs to be used.

*mw100\_probe*
--------------

Included with the module is the source code for *mw100\_probe* utility,
that will be built alongside the module. This program will generate
automatically a substitutions file **auto\_mw100.substitutions** and an
autosave file **auto\_mw100.req**. The substitutions file that can be
loaded at boot time or flattened by *msi* into a database file.

To use it, one needs values for: the IP address of the instrument; the
desired IOC PV prefix, **P**; the desired IOC PV name, **DAU**; and the
handle that will be given to the driver for this instrument, **HANDLE**.
The help can be seen by running *mw100\_probe* with no arguments.

The resultant substitutions file will have entries for all the hardware
channels, as well as all the constant channels and 60 (out of 300) of
both the calculation and communication channels. In the future, an
option will be added to changing the number of communication and
calculation channels included.

Database Loading
----------------

The simplest way to load databases associated with a populated MW100 is
by using the autogenerated substitutions file from *mw100\_probe*.

This substitutions file references the database files using a relative
path off of the **YOKOGAWA\_DAS** variable, which needs to be set to the
top of this module\'s installed directory. The synApps distribution does
this automatically, but it can be done by hand by adding to the startup
script (such as st.cmd) before loading the substitutions file a line
like this:

    epicsEnvSet("YOKOGAWA_DAS", "/my/path/to/YOKOGAWA_DAS")

To load the substitutions file, in the startup script, use

    dbLoadTemplate("auto_mw100.substitutions")

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
set to \"Yokogawa MW100\".

### INP and OUT format

For each MW100, the handle provided to the driver is used with record
device support. The general format of the **INP** or **OUT** field is
one of the following.

-   "@handle command"
-   "@handle command:address"
-   "@handle command:address.sub\_address"

### Interrupts

The input records will let you use interrupt scan mode (I/O Intr), and
there are several interrupts used. The first four interrupts can be
indirectly invoked after causing a read with their TRIG commands,
although only the first three are necessary, as an information read is
typically called when the status read notices a mode change into
measurements mode.

-   **Input**       - This interrupt is called after reading all the input  
                      channels, input modules and calculation addresses.
-   **Output**      - This interrupt is called after reading all the output
                      channels, output modules, communication addresses, and constant
                      addresses.
-   **Status**      - This interrupt is called after reading the operational
                      statuses of the MW100.
-   **Information** - This interrupt is called after reading the MW100
                      and channel information that can't be changed during measurement
                      mode.
-   **Error**       - This interrupt is called after the error state of the
                      MW100 occurs.

### Addresses

The type of address needed depends on the record, but here are all the
options. The format of the addresses comes directly from the MW100, and
this is how they are used internally.

-   Module addresses: **0-5**
-   Hardware channel addresses: **001-060**
    Each module (of the six possible modules) can use at most ten
    hardware addresses. If module 0 has ten channels, it uses addresses
    001-010, etc.
-   Calculation channel addresses: **A001-A300**
-   Communication channel addresses: **C001-C300**
-   Constant channel addresses: **K01-K60**

### Commands

**MODULE\_STRING** - Shows the module identification string for a location, or \"empty\" if one doesn\'t exist. Linked to information interrupt.  
    **stringin record:** Uses module address.

**MODULE\_PRESENCE** - Shows whether a module is at a location. Linked to information interrupt.  
    **bi record:** Uses module address.
    -   **0** - Not present
    -   **1** - Present

**MODULE\_MODEL** - Shows the module model number, with values being 110, 112, 114, 115, 120, or 125. Linked to information interrupt.  
    **longin record:** Uses module address.

**MODULE\_CODE** - Shows the 3 character module code. Linked to information interrupt.  
    **stringin record:** Uses module address.

**MODULE\_SPEED** - Shows the module speed. Linked to information interrupt.  
    **mbbi record:** Uses module address.
    -   **0** - Low
    -   **1** - Medium
    -   **2** - High

**MODULE\_NUMBER** - Shows the number of module channels. Linked to information interrupt.  
    **longin record:** Uses module address.  

**VAL** - Send or read a numeric value for a channel. Input records are linked to either the input interrupt (input channels) or the output interrupt (output channels).  
    This command is associated with several record types.
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
    **mbbi record:** Uses any hardware or calculation addresses.
    -   **0** - Normal
    -   **1** - Overrange
    -   **2** - Underrange
    -   **3** - Measurement Skip/Computation Off
    -   **4** - Error
    -   **5** - Value Uncertain

**ALARMS** - Shows the alarms set for an input channel. Records are linked to either the input interrupt (input channels) or the output interrupt (output channels).  
    **mbbi record:** Uses any hardware or calculation addresses. Each
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

**ALARM** - Shows the status for a particular alarm for an input channel. Records are linked to either the input interrupt (input modules) or the output interrupt (output modules).  
    **mbbi record:** Uses any hardware or calculation addresses, with
    sub-address of 1-4. The value definitions follow, although the
    possible values for a record are actually determined by the module
    and the channel mode.
    -   **0** - No Alarm
    -   **1** - High Limit
    -   **2** - Low Limit
    -   **3** - Differential High Limit
    -   **4** - Differential Low Limit
    -   **5** - Rate-of-Change High Limit
    -   **6** - Rate-of-Change Low Limit
    -   **7** - Delay High Limit
    -   **8** - Delay Low Limit

**ALARM\_FLAG** - Shows if an alarm for any channel is set. Linked to input interrupt, as it is assumed to be set more often than the output interrupt.  
    **bi record:** No address used.

**ALARM\_ACK** - Tells MW100 to acknowledge alarms. This record just needs to be PROC\'d to work.  
    **bo record:** No address used.

**CH\_STATUS** - Shows the configuration for a channel. Linked to information interrupt.  
    **mbbi record:** Uses any hardware or calculation addresses.
    -   **0** - Skip
    -   **1** - Normal
    -   **2** - Differential Input

**CH\_MODE** - Shows the operational mode for a channel, which is different each type of module (if it exists). Linked to information interrupt.  
    **mbbi record:** Uses hardware addresses for supported modules.

    Series **120** module (DAC)

        -   **0** - Skip
        -   **1** - Transmission
        -   **2** - Communication

    Series **125** module (relay)

        -   **0** - Skip
        -   **1** - Alarm
        -   **2** - Communication
        -   **3** - Media
        -   **4** - Failure
        -   **5** - Error

**UNIT** - Shows the unit for a channel. Linked to information interrupt.  
    **stringin record:** Uses any hardware or calculation addresses.

**EXPR** - Shows the expression used for a calculation channel. Linked to information interrupt.  
    **stringin record:** Uses any calculation addresses.

**IP\_ADDR** - Shows the IP address for the MW100. Linked to information interrupt.  
    **stringin record:** No address used.

**SETTINGS\_MODE** - Shows whether the MW100 is in settings mode. Linked to status interrupt.  
    **bi record:** No address used.
    -   **0** - False
    -   **1** - True

**MEASURE\_MODE** - Shows whether the MW100 is in measurement mode. Linked to status interrupt. This command is somewhat redundant to SETTINGS\_MODE as they seem to always be the opposite of each other, but they come from different bits being set, so not sure.  
    **bi record:** No address used.
    -   **0** - False
    -   **1** - True

**OPMODE** - Set the operational mode of the MW100.  
    **bo record:** No address used.
    -   **0** - Measurement
    -   **1** - Settings

**COMPUTE\_MODE** - Shows whether the MW100 has computations running. Linked to status interrupt. This can only be true when MEASURE\_MODE is true.  
    **bi record:** No address used.
    -   **0** - False
    -   **1** - True

**COMPUTE\_CMD** - Send a command regarding the computational mode of the MW100.  
    **mbbo record:** No address used.
    -   **0** - Start
    -   **1** - Stop
    -   **2** - Reset
    -   **3** - Clear

**INP\_TRIG** - Trigger reading the input records for the MW100, which will then trigger the input interrupt. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.  
    **bo record:** No address used.

**OUT\_TRIG** - Trigger reading the output records for the MW100, which will then trigger the input interrupt. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.  
    **bo record:** No address used.

**STAT\_TRIG** - Trigger reading the MW100 status, which will then trigger the status interrupt; if an operational mode change to measurements is seen, it will cause a information read which will then trigger the information interrupt. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.  
    **bo record:** No address used.

**INFO\_TRIG** - Trigger reading the information for channels and the MW100 that can\'t be changed during measurement mode. This command doesn\'t really need to be used if the STAT\_TRIG command is called periodically. This record just needs to be PROC\'d to work; typically the SCAN field is set to a time interval.  
    **bo record:** No address used.

**ERROR\_FLAG** - Shows if an error has happened. Linked to error interrupt.  
    **bi record:** No address used.

**ERROR** - Retrieve one of the three error strings used to return vendor-supplied error messages (broken to 40 character lengths). Linked to error interrupt.  
    **stringin record:** Uses an address of 1-3.

**ERROR\_CLEAR** - Clears the MW100 error. This record just needs to be PROC\'d to work.  
    **bo record:** No address used.

EPICS Databases
---------------

**not finished**

Dohn Arms\
Advanced Photon Source
