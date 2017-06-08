start medm -x -macro "P=13URL1:, R=cam1:" URLDriver.adl
call dllPath.bat
SET MAGICK_DEBUG=configure,exception
..\..\bin\%EPICS_HOST_ARCH%\URLDriverApp st.cmd.windows

