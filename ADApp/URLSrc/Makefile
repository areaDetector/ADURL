TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for miscellaneous/asynPortDriver.cpp
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif

LIBRARY_IOC_Linux = URLDriver
LIBRARY_IOC_WIN32 = URLDriver
LIBRARY_IOC_cygwin32 = URLDriver
LIBRARY_IOC_Darwin = URLDriver
LIB_SRCS += URLDriver.cpp

#==================================
PROD_IOC_Linux += URLTest
PROD_IOC_WIN32 += URLTest
PROD_IOC_cygwin32 += URLTest
URLTest_SRCS += URLTest.cpp

PROD_IOC_Linux += URLDriverApp
PROD_IOC_WIN32 += URLDriverApp
PROD_IOC_cygwin32 += URLDriverApp
PROD_IOC_Darwin += URLDriverApp
# <name>.dbd will be created from <name>Include.dbd
DBD += URLDriverApp.dbd
DBD += URLDriverSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
URLDriverApp_SRCS += URLDriverApp_registerRecordDeviceDriver.cpp URLDriverAppMain.cpp

PROD_LIBS += URLDriver

include $(TOP)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

