ADURL Releases
==============

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADURL.

Tagged source code and pre-built binary releases prior to R2-0 are included
in the areaDetector releases available via links at
https://cars.uchicago.edu/software/epics/areaDetector.html.

Tagged source code releases from R2-0 onward can be obtained at 
https://github.com/areaDetector/ADURL/releases.

Tagged prebuilt binaries from R2-0 onward can be obtained at
https://cars.uchicago.edu/software/pub/ADURL.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

R2-3 (17-May-2021)
----
* Converted documentation to ReST, include in documentation on github.io.
* Added autoconverted OPI files for css/BOY, css/Phoebus, edm, and caQtDM.
* Fix to release the lock at least briefly between each image, otherwise EPICS puts don't get processed.
* Move call to InitializeMagick() from the constructor to URLDriverConfig() because it was being called too late, 
  resulting in assertion failure on newer versions of GraphicsMagick.
* Remove unlock() and lock() around call to doCallbacksGenericPointer. It is not needed and can cause problems.
* Set ADSDKVersion to GraphicsMagick version, set NDDriverVersion to release of this driver.
* Remove parameter counting, not needed in current versions of asynPortDriver.
* Add URL to report().


R2-2 (4-July-2017)
----
* Changed Makefiles to set flags for new version of GraphicsMagick in ADSupport.
* Set SDKVersion and NDDriverVersion in driver.
* Added example GraphicsMagick configuration files (.mgk) to example IOC directory.
* Added some new test TIFF images in the iocURLDriver/images directory.
* Fixed medm files for compatibility with ADCore R3-0.


R2-1 (18-April-2015)
----
* Changes for compatibility with ADCore R2-2.


R2-0 (12-March-2014)
----
* Moved the repository to [Github](https://github.com/areaDetector/ADURL).
* Re-organized the directory structure to separate the driver library from the example IOC application.


R1-9-1 and earlier
------------------
Release notes are part of the
[areaDetector Release Notes](https://cars.uchicago.edu/software/epics/areaDetectorReleaseNotes.html).
