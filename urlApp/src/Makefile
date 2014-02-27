TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#======== DETECTOR LIBRARY ==============

ifeq ($(USE_GRAPHICSMAGICK), YES)
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

DBD += URLDriverSupport.dbd

#====== DETECTOR APPLICATION ============

ifeq ($(BUILD_APPS), YES)
#PROD_IOC_Linux += URLTest
#PROD_IOC_Linux += URLTest
#PROD_IOC_WIN32 += URLTest
#PROD_IOC_cygwin32 += URLTest
#URLTest_SRCS += URLTest.cpp

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

include $(TOP)/ADApp/commonDriverMakefile
endif
endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

