TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

ifeq ($(WITH_GRAPHICSMAGICK), YES)
  PROD_NAME = URLDriverApp
  PROD_IOC += $(PROD_NAME)
  
  # <name>.dbd will be created from <name>Include.dbd
  DBD += $(PROD_NAME).dbd
  $(PROD_NAME)_DBD += URLDriverSupport.dbd
  
  # <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
  $(PROD_NAME)_SRCS += URLDriverApp_registerRecordDeviceDriver.cpp URLDriverAppMain.cpp
  $(PROD_NAME)_SYS_LIBS_Linux += X11

  PROD_LIBS += URLDriver

  include $(ADCORE)/ADApp/commonDriverMakefile
endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

