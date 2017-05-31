TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

ifeq ($(WITH_GRAPHICSMAGICK), YES)
  PROD_NAME = URLDriverApp
  PROD_IOC_Linux    += $(PROD_NAME)
  PROD_IOC_WIN32    += $(PROD_NAME)
  PROD_IOC_cygwin32 += $(PROD_NAME)
  PROD_IOC_Darwin   += $(PROD_NAME)

  # <name>.dbd will be created from <name>Include.dbd
  DBD += $(PROD_NAME).dbd
  $(PROD_NAME)_DBD += URLDriverSupport.dbd

  # <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
  $(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp

  PROD_LIBS += URLDriver

  include $(ADCORE)/ADApp/commonDriverMakefile
endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

