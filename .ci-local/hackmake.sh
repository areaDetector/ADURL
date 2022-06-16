sed -i -e '/^include \$(TOP)\/ADApp\/commonLibraryMakefile/i ifeq (mingw, \$(findstring mingw, \$(T_A)))' /builds/DATAnet/adurl/.cache/adcore-R3-11/ADApp/pluginSrc/Makefile
sed -i -e '/^include \$(TOP)\/ADApp\/commonLibraryMakefile/i \ \ USR_LDFLAGS += -Wl,-allow-multiple-definition' /builds/DATAnet/adurl/.cache/adcore-R3-11/ADApp/pluginSrc/Makefile
sed -i -e '/^include \$(TOP)\/ADApp\/commonLibraryMakefile/i endif' /builds/DATAnet/adurl/.cache/adcore-R3-11/ADApp/pluginSrc/Makefile

