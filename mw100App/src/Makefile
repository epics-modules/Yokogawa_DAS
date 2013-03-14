TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

LIBRARY_IOC += mw100


DBD += drvAsynMW100.dbd
DBD += mw100Support.dbd

mw100_SRCS += drvAsynMW100.cpp

mw100_LIBS += asyn
mw100_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC = mw100Ex

DBD += mw100Ex.dbd

mw100Ex_DBD += base.dbd

mw100Ex_DBD += asyn.dbd
mw100Ex_DBD += drvAsynIPPort.dbd

mw100Ex_DBD += drvAsynMW100.dbd

mw100Ex_LIBS += mw100
mw100Ex_LIBS += asyn

mw100Ex_SRCS += mw100Ex_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
mw100Ex_SRCS_DEFAULT += mw100Main.cpp
mw100Ex_SRCS_vxWorks += -nil-

# Finally link to the EPICS Base libraries
mw100Ex_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

