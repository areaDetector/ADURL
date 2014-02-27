TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

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

  include $(ADCORE)/ADApp/commonLibraryMakefile
endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

