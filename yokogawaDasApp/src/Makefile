TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install cfg declaration file for downstream IOC dependency resolution
CFG += YOKOGAWA_DAS_DEPS

# Build an IOC support library
ifeq ($(OS_CLASS), Linux)
LIBRARY_IOC += mw100
LIBRARY_IOC += gm10
PROD_HOST += mw100_probe
PROD_HOST += gm10_probe
endif

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord

################ MW100 ###########################

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
mw100_SRCS += drvMW100_comm.c
mw100_SRCS += drvMW100.c

mw100_LIBS += $(EPICS_BASE_IOC_LIBS)

mw100_probe_SRCS += mw100_probe.c
#mw100_probe_CFLAGS += -DDBPATH="\"\""
#mw100_probe_LIBS += 


################ GM10 #########################

# Install xxxSupport.dbd into <top>/dbd
DBD += gm10Support.dbd

# Compile and add the code to the support library
gm10_SRCS += devGM10_common.c
gm10_SRCS += devGM10_ai.c
gm10_SRCS += devGM10_ao.c
gm10_SRCS += devGM10_bi.c
gm10_SRCS += devGM10_bo.c
gm10_SRCS += devGM10_mbbi.c
gm10_SRCS += devGM10_mbbo.c
gm10_SRCS += devGM10_longin.c
gm10_SRCS += devGM10_stringin.c
gm10_SRCS += drvGM10_comm.c
gm10_SRCS += drvGM10.c

gm10_LIBS += $(EPICS_BASE_IOC_LIBS)

gm10_probe_SRCS += gm10_probe.c
#gm10_probe_CFLAGS += -DDBPATH="\"\""
#gm10_probe_LIBS += 

#################################################

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

