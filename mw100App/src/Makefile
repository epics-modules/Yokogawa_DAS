TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Build an IOC support library and application
ifneq ($(OS_CLASS), WIN32)
LIBRARY_IOC += mw100
PROD_HOST += mw100_probe
PROD_IOC = mw100ex
endif

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += mw100Support.dbd

# Compile and add the code to the support library
mw100_SRCS += devMW100_common.c
mw100_SRCS += devMW100_ai.c
mw100_SRCS += devMW100_ao.c
mw100_SRCS += devMW100_bi.c
mw100_SRCS += devMW100_bo.c
mw100_SRCS += devMW100_mbbi.c
mw100_SRCS += devMW100_mbbo.c
mw100_SRCS += devMW100_longin.c
mw100_SRCS += devMW100_stringin.c
mw100_SRCS += drvMW100.c

mw100_LIBS += $(EPICS_BASE_IOC_LIBS)


mw100_probe_SRCS += mw100_probe.c
#mw100_probe_CFLAGS += -DDBPATH="\"\""
#mw100_probe_LIBS += 


# mw100ex.dbd will be created and installed
DBD += mw100ex.dbd

# mw100ex.dbd will include these files:
mw100ex_DBD += base.dbd
mw100ex_DBD += mw100Support.dbd

# mw100_registerRecordDeviceDriver.cpp derives from mw100ex.dbd
mw100ex_SRCS += mw100ex_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
mw100ex_SRCS_DEFAULT += mw100exMain.cpp
mw100ex_SRCS_vxWorks += -nil-

# Link in the code from our support library
mw100ex_LIBS += mw100

# Finally link IOC to the EPICS Base libraries
mw100ex_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

