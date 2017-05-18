REM start medm -x -macro "P=13URL1:, R=cam1:" URLDriver.adl

REM for URLDriver.dll
PATH=%PATH%;..\..\..\..\bin\%EPICS_HOST_ARCH%

REM for ADbase.dll
PATH=%PATH%;..\..\..\..\..\ADCore\bin\%EPICS_HOST_ARCH%

REM for asyn.dll
PATH=%PATH%;..\..\..\..\..\..\asyn\bin\%EPICS_HOST_ARCH%

REM for Magick++.dll
PATH=%PATH%;..\..\..\..\..\ADSupport\bin\%EPICS_HOST_ARCH%

REM for autosave.dll
PATH=%PATH%;..\..\..\..\..\..\autosave\bin\%EPICS_HOST_ARCH%

REM for busy.dll
PATH=%PATH%;..\..\..\..\..\..\busy\bin\%EPICS_HOST_ARCH%

REM for calc.dll
PATH=%PATH%;..\..\..\..\..\..\calc\bin\%EPICS_HOST_ARCH%

REM for sscan.dll
PATH=%PATH%;..\..\..\..\..\..\sscan\bin\%EPICS_HOST_ARCH%

SET MAGICK_DEBUG=configure,exception
..\..\bin\%EPICS_HOST_ARCH%\URLDriverApp st.cmd.win32


