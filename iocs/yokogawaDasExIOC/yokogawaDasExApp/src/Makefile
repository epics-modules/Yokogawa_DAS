TOP=../..
include $(TOP)/configure/CONFIG

PROD_NAME = yokogawaDasExApp
PROD_IOC_Linux = $(PROD_NAME)

DBD += $(PROD_NAME).dbd

$(PROD_NAME)_DBD += base.dbd
$(PROD_NAME)_DBD += $(YOKOGAWA_DAS_IOC_DBDS)

$(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp
$(PROD_NAME)_SRCS_DEFAULT += $(PROD_NAME)Main.cpp
$(PROD_NAME)_SRCS_vxWorks += -nil-

$(PROD_NAME)_LIBS += $(YOKOGAWA_DAS_IOC_LIBS)
$(PROD_NAME)_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
