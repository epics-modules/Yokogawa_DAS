#!../../bin/linux-x86_64/mw100ex

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/mw100ex.dbd"
mw100ex_registerRecordDeviceDriver pdbbase


cd "${TOP}/iocBoot/${IOC}"

# bcdadau1
mw100Init( "hdl", "164.54.54.138")
# fsdau1
#mw100Init( "hdl", "164.54.107.78") 

# With synApps, this variable is set in master RELEASE file
epicsEnvSet("YOKOGAWA_DAS", "${TOP}")

# Generate the following subsitutions file using
# ../../bin/linux-x86_64/mw100_probe (IP address) mwtest: dau hdl
dbLoadTemplate("auto_mw100.substitutions")

iocInit

