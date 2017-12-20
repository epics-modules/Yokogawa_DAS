#!../../bin/linux-x86_64/mw100ex

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/mw100ex.dbd"
mw100ex_registerRecordDeviceDriver pdbbase


# bcdadau1
mw100Init( "net1", "164.54.54.138")
# fsdau1
#mw100Init( "net2", "164.54.107.78") 


dbLoadRecords "mw100App/Db/MW100_main.db", "P=mwtest:,DAU=dau1"
dbLoadRecords "mw100App/Db/MW100_ADC_slot1.db", "P=mwtest:,DAU=dau1"
dbLoadRecords "mw100App/Db/MW100_DAC_slot2.db", "P=mwtest:,DAU=dau1"
####dbLoadRecords "mw100App/Db/MW100_DI_slot3.db", "P=mwtest:,DAU=dau1"
dbLoadRecords "mw100App/Db/MW100_relay_slot3.db", "P=mwtest:,DAU=dau1"


cd "${TOP}/iocBoot/${IOC}"
iocInit

