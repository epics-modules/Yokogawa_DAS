#!../../bin/linux-x86_64/mw100

## You may have to change mw100 to something else
## everywhere it appears in this file

< envPaths



# save_restore.cmd needs the full path to the startup directory, which
# envPaths currently does not provide
#epicsEnvSet(STARTUP,$(TOP)/iocBoot/$(IOC))

# Increase size of buffer for error logging from default 1256
#errlogInit(20000)


cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/mw100Ex.dbd"
mw100Ex_registerRecordDeviceDriver pdbbase

## Load record instances

cd ${TOP}/iocBoot/${IOC}

drvAsynMW100( "DAU1", "164.54.54.138", "dau-tcp1", 0, 0);
dbLoadTemplate "mw100.substitutions"

## Run this to trace the stages of iocInit
#traceIocInit

iocInit

## Start any sequence programs
