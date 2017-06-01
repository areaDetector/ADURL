start medm -x -macro "P=13URL1:, R=cam1:" URLDriver.adl
SET MAGICK_DEBUG=configure,exception
..\..\bin\%EPICS_HOST_ARCH%\URLDriverApp st.cmd.win32

