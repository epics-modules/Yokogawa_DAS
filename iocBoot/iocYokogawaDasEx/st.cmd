#!../../bin/linux-x86_64/yokogawaDasExample

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/yokogawaDasExample.dbd"
yokogawaDasExample_registerRecordDeviceDriver pdbbase


cd "${TOP}/iocBoot/${IOC}"

# bcdadau1
#mw100Init( "hdl", "164.54.54.138")
# fsdau1
#mw100Init( "hdl", "164.54.107.78") 

gm10Init("hdl", "164.54.52.228");

# With synApps, this variable is set in master RELEASE file
epicsEnvSet("YOKOGAWA_DAS", "${TOP}")

# Generate the following subsitutions file using
# ../../bin/linux-x86_64/mw100_probe (IP address) dasTest: dau hdl
#dbLoadTemplate("auto_mw100.substitutions")

# Generate the following subsitutions file using
# ../../bin/linux-x86_64/gm10_probe (IP address) dasTest: dau hdl
dbLoadTemplate("auto_gm10.substitutions")

iocInit

