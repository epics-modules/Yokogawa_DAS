TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install databases, templates & substitutions

#DB += $(patsubst ../%, %, $(wildcard ../*.template))
DB += $(patsubst ../%, %, $(wildcard ../*.db))

#REQ += $(patsubst ../%, %, $(wildcard ../*.req))

# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

