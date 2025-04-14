TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

ifeq ($(WITH_GRAPHICSMAGICK), YES)
  # The following gets rid of the -fno-implicit-templates flag on vxWorks, 
  # so we get automatic template instantiation.
  # This is what we want for miscellaneous/asynPortDriver.cpp
  ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
    CODE_CXXFLAGS=
  endif

  ifeq ($(GRAPHICSMAGICK_PREFIX_SYMBOLS),YES)
    USR_CXXFLAGS += -DPREFIX_MAGICK_SYMBOLS
  endif

  ifdef GRAPHICSMAGICK_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(GRAPHICSMAGICK_INCLUDE))
  endif

  ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
    USR_CXXFLAGS_WIN32 += -DWIN64
  endif
 
  ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
    USR_CXXFLAGS_WIN32 += -DWIN32
  endif
 
  ifeq (mingw, $(findstring mingw, $(T_A)))
    # multiple definition of `_Unwind_Resume on Linux. I don't know how to fix it.
    USR_LDFLAGS += -Wl,-allow-multiple-definition
  endif
 
  ifeq ($(SHARED_LIBRARIES),YES)
    USR_CXXFLAGS_WIN32 += -D_DLL
  else
    USR_CXXFLAGS_WIN32 += -D_LIB
  endif
  
  USR_CXXFLAGS_WIN32 += -D_VISUALC_

  LIBRARY_IOC = URLDriver
  LIB_SRCS += URLDriver.cpp

  DBD += URLDriverSupport.dbd

  include $(ADCORE)/ADApp/commonLibraryMakefile
endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

