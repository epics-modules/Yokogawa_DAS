<html>

<head>
<meta http-equiv="Content-Type"
content="text/html; charset=iso-8859-1">
<title>MW100 Docs</title>
</head>

<body bgcolor="#FFFFFF">

<h1 align="center">MW100 Support Documentation</h1>

<p>
This module has support for the Yokogawa MW100 Digital Acquisition
Unit.  It was written for Linux IOCs, where dynamic memory allocation
is not a problem, while others might work as well. It was also written
assuming the /M1 math option is present.
</p>
<p>
This device uses a network interface to communicate, and has a modular
design to allow the mixing of DAC, ADC, relay, TTL, and other modules.
It is also an industrial device primarily used for monitoring, so its
design is not optimized for high performance operation.  The system is
actually quite complex, as it can: read modbus channels over both
serial and the network, perform calculations on the measurements, set
alarms, do logging, etc.  Some subset of the functionality is
supported, and features have been added as needed. However, there is
no attempt made to utilize the full capability of the device from
EPICS, although the device can be simultaneously controlled by vendor
software or the built-in web server, as it supports multiple
simultaneous and independent connections.
</p>
<p>
I assume that the support works for all modules, but this has been
tested only with the following: MX110-UNV-M10, MX115-D05-H10,
MX120-VAO-M08, and MX125-MKC-M10.
</p>

<h2>Channel descriptions</h2>

<p>
The MW100 can handle up to 60 measurement channels, referred to as
channels 001-060.  The math option adds 300 computation channels
(A001-A300), 300 communication input channels (C001-C300), and 60
constant channels (K01-K60).
</p>
<p>
The module bus can have up to 6 modules on it.  The maximum number
of I/O channels for the MW100 is 60 (referred to as channels 001-060),
with a maximum of 10 channels per modules.  Each module has a
dedicated slice of the 60 channels, with the first module owning
001-010, the second owning 011-020, etc.  The first module is the one
closest to the main module.
</p>

<h2>Setting up the unit</h2>

<p>
Due to the complexity of the MW100, the configuration of the unit
currently needs to be done through its web interface.  This includes
not only service configuration, but also channel configuration.  The
way the unit works is that there are modes of operation.  Any
configuration needs to be done while in the "Settings" mode and
computations are stopped, while all data acquisition is done in the
"Meaurements" mode and the computations should be started.  The mode
can be toggled from EPICS or through the native web server.  So to do
configuration changes, make sure the unit is in Settings mode and do
the changes through the web server.
</p>
<p>
Upon getting the unit, get it on your network, as described in the
manual.
</p>
<p>
Plug your modules into the module bus, realizing that each module
location to the left of the main module has an address starting at
1,and increases as you go left.
</p>


<h2>Operation</h2>

<p>
Once the unit is configured and running on the network, the EPICS
support in this module will work for reading data while in measurement
mode, and reading calculations when math is enabled.
</p>
<p>
The default way the measurements are done is that there are two
records that trigger reading all of the values on the device, one for
input values (such as ADC and TTL) and another for output values (such
as DAC and relay).  By default, both records use a time value in their
SCAN fields to periodically poll values and put them into a cache, and
the record for these values are set to <b>I/O Intr</b> which allow
them to be updated by an interrupt call.  If a measurement record is
decoupled from the master poll rates by setting their SCAN field to a
time value, the measurement data associated with a record will be
retrieved, with all the associated information (status and alarms) put
into the cache, and the new value will be returned; however, the
associated status and alarm records for the measurement will
return cached values only.  This means that to do read an independent
channel is to read the measurement value, then forward link to all the
associated records to grab the cached values from that read.
</p>
<p>
The device calculations, communication inputs, and constants are read
by the pollers. The calculations (which need to be defined through the
web interface) allow the transformation of measurements with the
communication input values and predefined constants.  The first 60
calculation channels allow for a long calculation (string length of
120 characters), while the last 240 channels allow for short
calculations (8 or 10 characters, depending on firmware version).
</p>
<p>
When an error happpens, the error string is displayed, split into
three 40-character string records.  The error can be cleared, which
also clears it from the units LED display.
</p>

<h3>Modules</h3>

<p>
  The analog input modules (110 and 112 series) are of the typical
  voltage/current measuring or the strain gauge measuring types. The
  first type has many configuration options, such as configured as a
  thermocouple reader, giving a result through EPICS as a temperature
  value and units.
</p>
<p>
  I have had no access to integer digital input modules (114 series),
  but hopefully the support works for them.
</p>
<p>
  The binary digital input modules (115 series) can be used to sense a
  circuit closure or a voltage level.
</p>
<p>
  The analog output modules (120 series) are only directly controlled by
  EPICS if they are put into communication mode ("Comm. Input").  If a
  channel is put into the other mode ("Trans"), it is controlled by
  other MW100 channels, although the value can still be read by EPICS.
</p>
<p>
  The digital output modules (125 series) act as relays, and are
  typically used in one of two modes.  One is direct control from EPICS
  ("Comm. Input"), and the other is for it to be controlled by a MW100
  alarm ("Alarm").
</p>

<h3>Driver</h3>

<p>
  The EPICS driver for the MW100 needs to be loaded using the command
  mw100Init.
</p>
<p>
  <code>
    mw100Init( <i>instrument handle</i>, <i>IP address</i>)
  </code>
</p>
<p>
  The <i>instrument handle</i> is a string, provided by the user, that
  is used by records to refer to this particular MW100.  For each
  MW100 loaded, a different handle needs to be used.
</p>


<h2><i>mw100_probe</i></h2>

<p>
  Included with the module is the source code for <i>mw100_probe</i>
  utility, that will be built alongside the module.  This program will
  generate automatically a substitutions
  file <b>auto_mw100.substitutions</b> and an autosave
  file <b>auto_mw100.req</b>.
  The substitutions file that can
  be loaded at boot time or flattened by <i>msi</i> into a database
  file.
</p>
<p>
  To use it, one needs values for: the IP address of the instrument;
  the desired IOC PV prefix, <b>P</b>; the desired IOC PV
  name, <b>DAU</b>; and the handle that will be given to the driver
  for this instrument, <b>HANDLE</b>.  The help can be seen by running
  <i>mw100_probe</i> with no arguments.
</p>
<p>
  The resultant substitutions file will have entries for all the
  hardware channels, as well as all the constant channels and 60 (out
  of 300) of both the calculation and communication channels.  In the
  future, an option will be added to changing the number of
  communication and calculation channels included.
</p>

<h2>Database Loading</h2>

<p>
  The simplest way to load databases associated with a populated MW100
  is by using the autogenerated substitutions file
  from <i>mw100_probe</i>.
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
<pre>dbLoadTemplate("auto_mw100.substitutions")</pre>
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
  to be set to "Yokogawa MW100".
</p>
<h3>INP and OUT format</h3>
<p>
  For each MW100, the handle provided to the driver is used with record
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
  <li><b>Input</b> - This interrupt is called after reading all the
    input channels: input modules and calculation addresses.
  <li><b>Output</b> - This interrupt is called after reading all the
    output channels: output modules, communication addresses, and
    constant addresses.
  <li><b>Status</b> - This interrupt is called after reading the
    operational statuses of the MW100.
  <li><b>Information</b> - This interrupt is called after reading the
    MW100 and channel information that can't be changed during
    measurement mode.
  <li><b>Error</b> - This interrupt is called after the error state of
  the MW100 occurs.
</ul>
</p>
<h3>Addresses</h3>
<p>
The type of address needed depends on the record,
but here are all the options.  The format of the addresses comes
directly from the MW100, and this is how they are used internally.
<ul>
  <li>Module addresses: <b>0-5</b>
  <li>Hardware channel addresses: <b>001-060</b><br> Each module (of
    the six possible modules) can use at most ten hardware addresses.
    If module 0 has ten channels, it uses addresses 001-010, etc.
  <li>Calculation channel addresses: <b>A001-A300 </b>
  <li>Communication channel addresses: <b>C001-C300 </b>
  <li>Constant channel addresses: <b>K01-K60 </b>
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
  <dt><b>MODULE_MODEL</b> - Shows the module model number,
    with values being 110, 112, 114, 115, 120, or 125.  Linked to
    information interrupt.
  </dt>
  <dd>
    <b>longin record:</b> Uses module address.
  </dd>
  <dt><b>MODULE_CODE</b> - Shows the 3 character module code.  Linked
  to information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> Uses module address.
  </dd>
  <dt><b>MODULE_SPEED</b> - Shows the module speed.
    Linked to information interrupt.
  </dt>
  <dd>
    <b>mbbi record:</b> Uses module address.
    <ul>
      <li><b>0</b> - Low
      <li><b>1</b> - Medium
      <li><b>2</b> - High
    </ul>
  </dd>
  <dt><b>MODULE_NUMBER</b> - Shows the number of module channels.
  Linked to information interrupt.
  </dt>
  <dd>
    <b>longin record:</b> Uses module address.
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
    <b>mbbi record:</b> Uses any hardware or calculation
    addresses.
    <ul>
      <li><b>0</b> - Normal
      <li><b>1</b> - Overrange
      <li><b>2</b> - Underrange
      <li><b>3</b> - Measurement Skip/Computation Off
      <li><b>4</b> - Error
      <li><b>5</b> - Value Uncertain
    </ul>
  </dd>
  <dt><b>ALARMS</b> - Shows the alarms set for an input channel.
    Records are linked to either the input interrupt (input channels)
    or the output interrupt (output channels).
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
    input channel.  Records are linked to either the input interrupt
    (input modules) or the output interrupt (output modules).
  </dt>
  <dd>
    <b>mbbi record:</b> Uses any hardware or calculation addresses,
    with sub-address of 1-4. The value definitions follow, although
    the possible values for a record are actually determined by the
    module and the channel mode.
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
    Linked to input interrupt, as it is assumed to be set more often
    than the output interrupt.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
  </dd>
  <dt><b>ALARM_ACK</b> - Tells MW100 to acknowledge alarms.  This record
    just needs to be PROC'd to work.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>
  <dt><b>CH_STATUS</b> - Shows the configuration for a channel.
    Linked to information interrupt.
  </dt>
  <dd>
    <b>mbbi record:</b> Uses any hardware or calculation
    addresses.
    <ul>
      <li><b>0</b> - Skip
      <li><b>1</b> - Normal
      <li><b>2</b> - Differential Input
    </ul>
  </dd>
  <dt><b>CH_MODE</b> - Shows the operational mode for a channel, which
  is different each type of module (if it exists).  Linked to
    information interrupt.
  </dt>
  <dd>
    <b>mbbi record:</b> Uses hardware addresses for supported modules.
    <dl>
      <dt>Series <b>120</b> module (DAC)
      </dt>
      <dd>
        <ul>
          <li><b>0</b> - Skip
          <li><b>1</b> - Transmission
          <li><b>2</b> - Communication
        </ul>
      </dd>
    </dl>
    <dl>
      <dt>Series <b>125</b> module (relay)
      </dt>
      <dd>
        <ul>
          <li><b>0</b> - Skip
          <li><b>1</b> - Alarm
          <li><b>2</b> - Communication
          <li><b>3</b> - Media
          <li><b>4</b> - Failure
          <li><b>5</b> - Error
        </ul>
      </dd>
    </dl>

  </dd>
  <dt><b>UNIT</b> - Shows the unit for a channel.  Linked to
    information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> Uses any hardware or calculation
    addresses.
  </dd>
  <dt><b>EXPR</b> - Shows the expression used for a calculation
    channel.  Linked to information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> Uses any calculation addresses.
  </dd>


  <dt><b>IP_ADDR</b> - Shows the IP address for the MW100.  Linked to
    information interrupt.
  </dt>
  <dd>
    <b>stringin record:</b> No address used.
  </dd>
  <dt><b>SETTINGS_MODE</b> - Shows whether the MW100 is in settings
    mode.  Linked to status interrupt.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
    <ul>
      <li><b>0</b> - False
      <li><b>1</b> - True
    </ul>
  </dd>
  <dt><b>MEASURE_MODE</b> - Shows whether the MW100 is in measurement
    mode.  Linked to status interrupt.  This command is somewhat
    redundant to SETTINGS_MODE as they seem to always be the opposite
    of each other, but they come from different bits being set, so not
    sure.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
    <ul>
      <li><b>0</b> - False
      <li><b>1</b> - True
    </ul>
  </dd>
  <dt><b>OPMODE</b> - Set the operational mode of the MW100.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
    <ul>
      <li><b>0</b> - Measurement
      <li><b>1</b> - Settings
    </ul>
  </dd>
  <dt><b>COMPUTE_MODE</b> - Shows whether the MW100 has computations
  running.  Linked to status interrupt.  This can only be true when
  MEASURE_MODE is true.
  </dt>
  <dd>
    <b>bi record:</b> No address used.
    <ul>
      <li><b>0</b> - False
      <li><b>1</b> - True
    </ul>
  </dd>
  <dt><b>COMPUTE_CMD</b> - Send a command regarding the computational
  mode of the MW100.
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
  <dt><b>INP_TRIG</b> - Trigger reading the input records for the
    MW100, which will then trigger the input interrupt.  This record
    just needs to be PROC'd to work; typically the SCAN field is set
    to a time interval.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>
  <dt><b>OUT_TRIG</b> - Trigger reading the output records for the
    MW100, which will then trigger the input interrupt.  This record
    just needs to be PROC'd to work; typically the SCAN field is set
    to a time interval.
  </dt>
  <dd>
    <b>bo record:</b> No address used.
  </dd>
  <dt><b>STAT_TRIG</b> - Trigger reading the MW100 status, which will
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
    and the MW100 that can't be changed during measurement mode.  This
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
  <dt><b>ERROR_CLEAR</b> - Clears the MW100 error.  This record
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
