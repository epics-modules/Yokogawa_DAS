<html>

<head>
<meta http-equiv="Content-Type"
content="text/html; charset=iso-8859-1">
<title>GM10 Docs</title>
</head>

<body bgcolor="#FFFFFF">

<h1 align="center">GM10 Support Documentation</h1>

<p>
This module has support for the Yokogawa GM10 Digital Acquisition
System.  It was written for Linux IOCs, where dynamic memory
allocation is not a problem, while others might work as well. The /E1
Ethernet option is mandatory. The /MT mathematic option is higly
recommended, while the /MC option for communication input channel
options is recommended as it allows the GM10 to connect to Modbus
devices.  The amount of available channels for certain channel types
depends on the memory option, GM10-1 or GM10-2.  Support for chaining
expansion units is not included in this driver.
</p>
<p> This device uses a network interface to communicate, and has a
modular design to allow the mixing of DAC, ADC, relay, TTL, and other
modules.  It is also an industrial device primarily used for
monitoring, so its design is not optimized for high performance
operation.  The system is actually quite complex, as it can: read
modbus channels over both serial and the network, perform calculations
on the measurements, set alarms, do logging, throw relays, etc.  Some
subset of the functionality is supported, and features have been added
as needed. However, there is no attempt made to utilize the full
capability of the device from EPICS, although the device can be
simultaneously controlled by vendor software or the built-in web
server, as it supports multiple simultaneous and independent
connections.
</p>
<p>
I assume that the support works for all modules (other than
GX90UT-02-11), but this has been tested only with the following:
GX90XA-10-U2, GX90YA-04-C1, and GX90WD-0806-01.
</p>

<h2>Channel descriptions</h2>

<p>
As chaining expansion units is not supported, the GM10 has an a
measurement channel address space of 0001-0999, supporting up to 100
channels.  The /MT math option adds either 100 or 200 computation
channels: A001-A100 for GM10-1, and A001-A200 for GM10-2.  The /MC
communications option adds either 300 or 500 communication input
channels: C001-C300 for GM10-1, and C001-C500 for GM10-2.  There are
100 channels for constants (K001-K100), and 100 channels for variable
constants (W001-W100).
</p>
<p>
The module bus can have up to 10 modules on it. Each of the possible
ten modules has an address slice like 0X01-0X99 (where X is 0 to 9).
The modules are numbered, starting at 0, in the order the are attached
to the GM10 module.
</p>

<h2>Setting up the unit</h2>

<p>
Due to the complexity of the GM10, the configuration of the unit
currently needs to be done through its web interface.  This includes
not only service configuration, but also channel configuration.  The
way the unit works is that there are modes of operation.  Any
configuration needs to be done while the "Recording" and "Computing"
modes are disabled, while measuring the channels doesn't stop
configuration.  The modes can be toggled from EPICS or through the
native web server.
</p>
<p>
Upon getting the unit, get it on your network, as described in the
manual.
</p>
<p>
Plug your modules into the module bus, realizing that each module
location to the left of the main module has an address starting at
0,and increases as you go left.
</p>


<h2>Operation</h2>

<p>
Once the unit is configured and running on the network, the EPICS
support in this module will constantly read data, and compute
calculations when math is enabled.
</p>
<p>
The default way the measurements are done is that there is a record
that trigger reading all of the channel values on the device: input,
output, math, and communication.  By default, the records use a time
value in their SCAN fields to periodically poll values and put them
into a cache, and the record for these values are set to <b>I/O
Intr</b> which allow them to be updated by an interrupt call.  If a
measurement record is decoupled from the master poll rates by setting
their SCAN field to a time value, the measurement data associated with
a record will be retrieved, with all the associated information
(status and alarms) put into the cache, and the new value will be
returned; however, the associated status and alarm records for the
measurement will return cached values only.  This means that to do
read an independent channel is to read the measurement value, then
forward link to all the associated records to grab the cached values
from that read.  A similar polling record is used for reading the
constants and variable constants.
</p>
<p>
The calculations (which need to be defined through the web
interface) allow the transformation of measurements with the
communication input values and predefined constants.  The calculation
channels allow for expression strings of 120 characters.
</p>
<p>
When an error happpens, the error string is displayed, split into
three 40-character string records.  The error can be cleared, which
also clears it from the units LED display.
</p>

<h3>Modules</h3>

<p>
  The GX90XA analog input modules are of the typical voltage/current
  measuring type. It has many configuration options, such as
  configured as a thermocouple reader, giving a result through EPICS
  as a temperature value and units.
</p>
<p>
  The GX90YA analog output modules are only directly controlled by
  EPICS if they are put into manual mode.  If a channel is put into
  the restransmit mode, it is controlled by other GM10 channels,
  although the value can still be read by EPICS.

  These channels do not provide a voltage, but either a 0-20 mA or
  4-20 mA current.  To provide a voltage, one has to use a resistor of
  known resistance, and use the voltage that occurs across the
  resistor for a given current.
</p>
<p>
  The GX90XD and GX90WD binary digital input modules can be used to
  sense a circuit closure or a voltage level.
</p>
<p>
  The GX90YD and GX90WD digital output modules act as relays, and are
  typically used in one of two modes.  One is direct control from
  EPICS ("Manual"), and the other is for it to be controlled by a GM10
  alarm ("Alarm").
</p>

<h3>Driver</h3>

<p>
  The EPICS driver for the GM10 needs to be loaded using the command
  gm10Init.
</p>
<p>
  <code>
    gm10Init( <i>instrument handle</i>, <i>IP address</i>)
  </code>
</p>
<p>
  The <i>instrument handle</i> is a string, provided by the user, that
  is used by records to refer to this particular GM10.  For each GM10
  loaded, a different handle needs to be used.
</p>


<h2><i>gm10_probe</i></h2>

<p>
  Included with the module is the source code for <i>gm10_probe</i>
  utility, that will be built alongside the module.  This program will
  generate automatically a substitutions
  file <b>auto_gm10.substitutions</b> and an autosave
  file <b>auto_gm10.req</b>.
  The substitutions file that can
  be loaded at boot time or flattened by <i>msi</i> into a database
  file.
</p>
<p>
  To use it, one needs values for: the IP address of the instrument;
  the desired IOC PV prefix, <b>P</b>; the desired IOC PV
  name, <b>DAU</b>; and the handle that will be given to the driver
  for this instrument, <b>HANDLE</b>.  The help can be seen by running
  <i>gm10_probe</i> with no arguments.
</p>
<p>
  The resultant substitutions file will have entries for all the
  hardware channels, as well as 100 each of the constant, variable
  constant, calculation, and communication channels.  In the future, an
  option will be added to changing the number of communication and
  calculation channels included.
</p>

<h2>Database Loading</h2>

<p>
  The simplest way to load databases associated with a populated GM10
  is by using the autogenerated substitutions file
  from <i>gm10_probe</i>.
</p>
<p>
  This substitutions file references the
  database files using a relative path off of the <b>YOKOGAWA_DAS</b>
  variable, which needs to be set to the top of this module's
  installed directory.  The synApps distribution does this
  automatically, but it can be done by hand by adding to the startup
  script (such as st.cmd) before loading the substitutions file a line
  like this:
  <pre>epicsEnvSet("YOKOGAWA_DAS", "/my/path/to/YOKOGAWA_DAS")</pre>
</p>

<p>To load the substitutions file, in the startup script, use
<pre>dbLoadTemplate("auto_gm10.substitutions")</pre>
</p>

<h2>MEDM display files</h2>

<p><b>not finished</b></p>


<h2>Records</h2>

<p>
There are currently 7 records supported for various purposes: ai, ao,
bi, bo, mbbi, mbbo, and stringin.     Record commands are also
provided to determine the record function, and for record commands
that can refer to multiple items or locations, addressing is used; a
few commands also need sub-addressing.
</p>
<h3>DTYP format</h3>
<p>
  When using the supported records with the commands, <b>DTYP</b> needs
  to be set to "Yokogawa GM10".
</p>
<h3>INP and OUT format</h3>
<p>
  For each GM10, the handle provided to the driver is used with record
  device support.  The general format of the <b>INP</b> or <b>OUT</b>
  field is one of the following.
<ul>
  <li>"@handle command"
  <li>"@handle command:address"
  <li>"@handle command:address.sub_address"
</ul>
</p>
<h3>Interrupts</h3>
<p>
The input records will let you use interrupt scan mode (I/O Intr), and
there are several interrupts used.  The first four interrupts can be
indirectly invoked after causing a read with their TRIG commands,
although only the first three are necessary, as an information read is
typically called when the status read notices a mode change into
measurements mode.
<ul>
  <li><b>Channels</b> - This interrupt is called after reading all the
    channels.
  <li><b>Miscellaneous</b> - This interrupt is called after reading
    all the constants and variable constants.
  <li><b>Status</b> - This interrupt is called after reading the
    operational statuses of the GM10.
  <li><b>Information</b> - This interrupt is called after reading the
    GM10 and channel information that can't be changed during
    measurement mode.
  <li><b>Error</b> - This interrupt is called after the error state of
    the GM10 occurs.
</ul>
</p>
<h3>Addresses</h3>
<p>
The type of address needed depends on the record, but here are all the
options.  The format of the addresses comes directly from the GM10,
and this is how they are used internally.
<ul>
  <li>Module addresses: <b>0-9</b>
  <li>Hardware channel addresses: <b>0001-0999</b><br> Each module (of
    the ten possible modules) can use at most 99 hardware addresses.
    If module 3 has ten channels, it uses addresses 0301-0310, etc.
  <li>Calculation channel addresses: <b>A001-A100</b> or <b>A001-A200</b>
  <li>Communication channel addresses: <b>C001-C300</b> or <b>C001-C500</b>
  <li>Constant channel addresses: <b>K001-K100 </b>
  <li>Variable constant channel addresses: <b>W001-W100 </b>
</ul>
</p>
<h3>Commands</h3>
<p>
<dl>
  <dt><b>MODULE_STRING</b> - Shows the module identification string
    for a location, or "empty" if one doesn't exist.  Linked to
    information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> Uses module address.
  </dd>
  <dt><b>MODULE_PRESENCE</b> - Shows whether a module is at a
    location. Linked to information interrupt.
  </dt>
  <dd>
    <b>bi record:</b> Uses module address.
    <ul>
      <li><b>0</b> - Not present
      <li><b>1</b> - Present
    </ul>
  </dd>

  <dt><b>VAL</b> - Send or read a numeric value for a channel. Input
  records are linked to either the input interrupt (input channels) or
  the output interrupt (output channels).</dt>
  <dd>
    This command is associated with several record types.
    <ul> 
      <li><b>bi record:</b> Uses hardware addresses for binary digital
      input and output modules.
      <li><b>bo record:</b> Uses hardware addresses for binary digital
      output modules.
      <li><b>longin record:</b> Uses hardware addresses for binary
      integer input modules.
      <li><b>ai record:</b> Uses hardware addresses for analog input and
      output modules, as well as calculation, communication, and
      contant addresses.
      <li><b>ao record:</b> Uses hardware addresses for analog output
      modules, as well as calculation, communication, and contant
      addresses.
    </ul>
  </dd>
  <dt><b>VAL_STATUS</b> - Shows any special status for a value read
    from a channel.  Records are linked to either the input interrupt
    (input channels) or the output interrupt (output channels).
  </dt>
  <dd>
    <b>mbbi record:</b> Uses any hardware, calculation, or
    communication addresses.
    <ul>
      <li><b>0</b> - Normal
      <li><b>1</b> - Skip
      <li><b>2</b> - Positive Overrange
      <li><b>3</b> - Negative Overrange
      <li><b>4</b> - Positive Burnout
      <li><b>5</b> - Negative Burnout
      <li><b>6</b> - A/D Conversion Error
      <li><b>7</b> - Invalid Data
      <li><b>16</b> - Math Result NAN
      <li><b>17</b> - Communication Error
      <li><b>32</b> - Unknown
    </ul>
  </dd>
  <dt><b>ALARMS</b> - Shows the alarms set for an input channel.
    Records are linked to the channel interrupt.
  </dt>
  <dd>
    <b>mbbi record:</b> Uses any hardware or calculation addresses.
    Each bit of the 4-bit value corresponds to the status of an alarm,
    as multiple alarms can occur simultaneously; bit 0 is defined as
    the least significant bit.
    <ul>
      <li><b>bit 0</b> - Alarm 1
      <li><b>bit 1</b> - Alarm 2
      <li><b>bit 2</b> - Alarm 3
      <li><b>bit 3</b> - Alarm 4
    </ul>
    The bit value is defined as the following.
    <ul>
      <li><b>value 0</b> - Alarm off
      <li><b>value 1</b> - Alarm on
    </ul>
  </dd>
  <dt><b>ALARM</b> - Shows the status for a particular alarm for an
    input channel.  Records are linked to the channel interrupt.
  </dt>
  <dd>
    <b>mbbi record:</b> Uses any hardware, calculation, or
    communication addresses; with sub-address of 1-4. The value
    definitions follow, although the possible values for a record are
    actually determined by the module and the channel mode.
    <ul>
      <li><b>0</b> - No Alarm
      <li><b>1</b> - High Limit
      <li><b>2</b> - Low Limit
      <li><b>3</b> - Differential High Limit
      <li><b>4</b> - Differential Low Limit
      <li><b>5</b> - Rate-of-Change High Limit
      <li><b>6</b> - Rate-of-Change Low Limit
      <li><b>7</b> - Delay High Limit
      <li><b>8</b> - Delay Low Limit
    </ul>
  </dd>
  <dt><b>ALARM_FLAG</b> - Shows if an alarm for any channel is set.
    Linked to channel interrupt.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
  </dd>
  <dt><b>ALARM_ACK</b> - Tells GM10 to acknowledge alarms.  This record
    just needs to be PROC'd to work.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>
  <dt><b>CH_STATUS</b> - Shows the configuration for a channel.
    Linked to information interrupt.
  </dt>
  <dd>
    <b>mbbi record:</b> Uses any hardware, calculation, or communication 
    addresses.
    <ul>
      <li><b>0</b> - Skip
      <li><b>1</b> - Normal
      <li><b>2</b> - Differential Input
    </ul>
  </dd>
  <dt><b>CH_MODE</b> - Shows the operational mode for a channel, which
  is different depending on the type.  Linked to information
  interrupt.
  </dt>
  <dd>
    <b>mbbi record:</b> Uses hardware addresses for supported modules.
    <dl>
      <dt>Analog Output Channel
      </dt>
      <dd>
        <ul>
          <li><b>0</b> - Skip
          <li><b>1</b> - Retransmit
          <li><b>2</b> - Manual
        </ul>
      </dd>
    </dl>
    <dl>
      <dt>Digital Output Channel
      </dt>
      <dd>
        <ul>
          <li><b>0</b> - Alarm
          <li><b>1</b> - Manual
          <li><b>2</b> - Failure
        </ul>
      </dd>
    </dl>

  </dd>
  <dt><b>UNIT</b> - Shows the unit for a channel.  Linked to
    information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> Uses any hardware, calculation, or communication 
    addresses.
  </dd>
  <dt><b>EXPR</b> - Shows the expression used for a calculation
    channel.  Linked to information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> Uses any calculation addresses.
  </dd>


  <dt><b>IP_ADDR</b> - Shows the IP address for the GM10.  Linked to
    information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> No address used.
  </dd>
  <dt><b>SETTINGS</b> - Shows the status of the GM10 settings.
    Linked to status interrupt.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
    <ul>
      <li><b>0</b> - Frozen
      <li><b>1</b> - Unfrozen
    </ul>
  </dd>
  <dt><b>RECORDING_MODE</b> - Shows whether the GM10 is in recording
    mode.  Linked to status interrupt.  
  </dt>
  <dd>
    <b>bi record:</b> No address used.
    <ul>
      <li><b>0</b> - Running
      <li><b>1</b> - Stopped
    </ul>
    <b>bo record:</b> No address used.
    <ul>
      <li><b>0</b> - Start
      <li><b>1</b> - Stop
    </ul>
  </dd>
  <dt><b>COMPUTE_MODE</b> - Shows whether the GM10 has computations
  running.  Linked to status interrupt.  This can only be true when
  MEASURE_MODE is true.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
    <ul>
      <li><b>0</b> - Running
      <li><b>1</b> - Stopped
    </ul>
  </dd>
  <dt><b>COMPUTE_CMD</b> - Send a command regarding the computational
  mode of the GM10.
  </dt>
  <dd>
    <b>mbbo record:</b> No address used.
    <ul>
      <li><b>0</b> - Start
      <li><b>1</b> - Stop
      <li><b>2</b> - Reset
      <li><b>3</b> - Clear
    </ul>
  </dd>
  <dt><b>CHAN_TRIG</b> - Trigger reading the channel records for the
    GM10, which will then trigger the channel interrupt.  This record
    just needs to be PROC'd to work; typically the SCAN field is set
    to a time interval.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>
  <dt><b>CONST_TRIG</b> - Trigger reading the constant records for the
    GM10, which will then trigger the constant interrupt.  This record
    just needs to be PROC'd to work; typically the SCAN field is set
    to a time interval.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>
  <dt><b>STAT_TRIG</b> - Trigger reading the GM10 status, which will
    then trigger the status interrupt; if an operational mode change
    to measurements is seen, it will cause a information read which
    will then trigger the information interrupt. This record just
    needs to be PROC'd to work; typically the SCAN field is set to a
    time interval.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>
  <dt><b>INFO_TRIG</b> - Trigger reading the information for channels
    and the GM10 that can't be changed during measurement mode.  This
    command doesn't really need to be used if the STAT_TRIG command is
    called periodically. This record just needs to be PROC'd to work;
    typically the SCAN field is set to a time interval.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>

  <dt><b>ERROR_FLAG</b> - Shows if an error has happened.
    Linked to error interrupt.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
  </dd>
  <dt><b>ERROR</b> - Retrieve one of the three error strings used to
    return vendor-supplied error messages (broken to 40 character
    lengths). Linked to error interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> Uses an address of 1-3.
  </dd>
  <dt><b>ERROR_CLEAR</b> - Clears the GM10 error.  This record
    just needs to be PROC'd to work.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>

</dl>
</p>


<h2>EPICS Databases</h2>

<p><b>not finished</b></p>


<address> 
    Dohn Arms<br>
    Advanced Photon Source
</address>
</body>
</html>
