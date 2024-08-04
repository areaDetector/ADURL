/* URLDriver.cpp
 *
 * This is a driver for reading images from a URL using GraphicsMagick.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  October 12, 2010
 *
 */

#include <stdio.h>
#include <string.h>
#include <string>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <Magick++.h>
using namespace Magick;

#include <epicsExport.h>
#include <URLDriver.h>

static const char *driverName = "URLDriver";


asynStatus URLDriver::readImage()
{
    char URLString[MAX_FILENAME_LEN];
    size_t dims[3];
    int ndims;
    int nrows, ncols;
    ImageType imageType;
    StorageType storageType;
    NDDataType_t dataType;
    NDColorMode_t colorMode;
    NDArrayInfo_t arrayInfo;
    NDArray *pImage = this->pArrays[0];
    int depth;
    const char *map;
    static const char *functionName = "readImage";
    
    getStringParam(URLName, sizeof(URLString), URLString);
    if (strlen(URLString) == 0) return(asynError);
    try {
        image.read(URLString);
        imageType = image.type();
        depth = image.depth();
        nrows = image.rows();
        ncols = image.columns();
        switch(imageType) {
        case GrayscaleType:
            ndims = 2;
            dims[0] = ncols;
            dims[1] = nrows;
            dims[2] = 0;
            map = "R";
            colorMode = NDColorModeMono;
            break;
        case TrueColorType:
            ndims = 3;
            dims[0] = 3;
            dims[1] = ncols;
            dims[2] = nrows;
            map = "RGB";
            colorMode = NDColorModeRGB1;
            break;
        default:
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: unknown ImageType=%d\n", 
                driverName, functionName, imageType);
            return(asynError);
            break;
        }
        switch(depth) {
        case 1:
        case 8:
            dataType = NDUInt8;
            storageType = CharPixel;
            break;
        case 16:
            dataType = NDUInt16;
            storageType = ShortPixel;
            break;
        case 32:
            dataType = NDUInt32;
            storageType = IntegerPixel;
            break;
        default:
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: unsupported depth=%d\n", 
                driverName, functionName, depth);
            return(asynError);
            break;
        }
        if (pImage) pImage->release();
        this->pArrays[0] = this->pNDArrayPool->alloc(ndims, dims, dataType, 0, NULL);
        pImage = this->pArrays[0];
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
            "%s:%s: reading URL=%s, dimensions=[%lu,%lu,%lu], ImageType=%d, depth=%d\n",
            driverName, functionName, URLString, 
            (unsigned long)dims[0], (unsigned long)dims[1], (unsigned long)dims[2], imageType, depth);
        image.write(0, 0, ncols, nrows, map, storageType, pImage->pData);
        pImage->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
        setIntegerParam(ADSizeX, ncols);
        setIntegerParam(NDArraySizeX, ncols);
        setIntegerParam(ADSizeY, nrows);
        setIntegerParam(NDArraySizeY, nrows);
        pImage->getInfo(&arrayInfo);
        setIntegerParam(NDArraySize,  (int)arrayInfo.totalBytes);
        setIntegerParam(NDDataType, dataType);
        setIntegerParam(NDColorMode, colorMode);
    }
    catch(std::exception &error)
    {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: error reading URL=%s\n", 
            driverName, functionName, error.what());
        return(asynError);
    }
         
    return(asynSuccess);
}


static void URLTaskC(void *drvPvt)
{
    URLDriver *pPvt = (URLDriver *)drvPvt;

    pPvt->URLTask();
}

/** This thread calls computeImage to compute new image data and does the callbacks to send it to higher layers.
  * It implements the logic for single, multiple or continuous acquisition. */
void URLDriver::URLTask()
{
    asynStatus imageStatus;
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    int acquire;
    NDArray *pImage;
    double acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;
    const char *functionName = "URLTask";

    this->lock();
    /* Loop forever */
    while (1) {
        /* Is acquisition active? */
        getIntegerParam(ADAcquire, &acquire);

        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) {
            setIntegerParam(ADStatus, ADStatusIdle);
            callParamCallbacks();
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            this->unlock();
            epicsEventWait(this->startEventId);
            this->lock();
            setIntegerParam(ADNumImagesCounter, 0);
        }

        /* We are acquiring. */
        /* Get the current time */
        epicsTimeGetCurrent(&startTime);

        /* Get the exposure parameters */
        getDoubleParam(ADAcquirePeriod, &acquirePeriod);

        setIntegerParam(ADStatus, ADStatusAcquire);

        /* Open the shutter */
        setShutter(ADShutterOpen);

        /* Call the callbacks to update any changes */
        callParamCallbacks();

        /* Read the image */
        imageStatus = readImage();

        /* Close the shutter */
        setShutter(ADShutterClosed);
        /* Call the callbacks to update any changes */
        callParamCallbacks();

        if (imageStatus == asynSuccess) {
            pImage = this->pArrays[0];

            /* Get the current parameters */
            getIntegerParam(NDArrayCounter, &imageCounter);
            getIntegerParam(ADNumImages, &numImages);
            getIntegerParam(ADNumImagesCounter, &numImagesCounter);
            getIntegerParam(ADImageMode, &imageMode);
            getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
            imageCounter++;
            numImagesCounter++;
            setIntegerParam(NDArrayCounter, imageCounter);
            setIntegerParam(ADNumImagesCounter, numImagesCounter);

            /* Put the frame number and time stamp into the buffer */
            pImage->uniqueId = imageCounter;
            pImage->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
            updateTimeStamp(&pImage->epicsTS);

            /* Get any attributes that have been defined for this driver */
            this->getAttributes(pImage->pAttributeList);

            if (arrayCallbacks) {
                /* Call the NDArray callback */
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                     "%s:%s: calling imageData callback\n", driverName, functionName);
                doCallbacksGenericPointer(pImage, NDArrayData, 0);
            }
        }

        /* See if acquisition is done */
        if ((imageStatus != asynSuccess) ||
            (imageMode == ADImageSingle) ||
            ((imageMode == ADImageMultiple) &&
             (numImagesCounter >= numImages))) {
            setIntegerParam(ADAcquire, 0);
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                  "%s:%s: acquisition completed\n", driverName, functionName);
        }

        /* Call the callbacks to update any changes */
        callParamCallbacks();
        getIntegerParam(ADAcquire, &acquire);

        /* If we are acquiring then sleep for the acquire period minus elapsed time. */
        if (acquire) {
            epicsTimeGetCurrent(&endTime);
            elapsedTime = epicsTimeDiffInSeconds(&endTime, &startTime);
            delay = acquirePeriod - elapsedTime;
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                     "%s:%s: delay=%f\n",
                      driverName, functionName, delay);
            // Need at least a short delay to release the lock
            if (delay <= 0.0) delay = 0.001;
            /* We set the status to readOut to indicate we are in the period delay */
            setIntegerParam(ADStatus, ADStatusWaiting);
            callParamCallbacks();
            this->unlock();
            epicsEventWaitWithTimeout(this->stopEventId, delay);
            this->lock();
        }
    }
}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire, ADColorMode, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus URLDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int adstatus;
    asynStatus status = asynSuccess;

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setIntegerParam(function, value);

    if (function == ADAcquire) {
        getIntegerParam(ADStatus, &adstatus);
        if (value && (adstatus == ADStatusIdle)) {
            /* Send an event to wake up the acquisition task.
             * It won't actually start generating new images until we release the lock below */
            epicsEventSignal(this->startEventId);
        }
        if (!value && (adstatus != ADStatusIdle)) {
            /* This was a command to stop acquisition */
            /* Send the stop event */
            epicsEventSignal(this->stopEventId);
        }
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_URL_DRIVER_PARAM) status = ADDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:writeInt32 error, status=%d function=%d, value=%d\n",
              driverName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:writeInt32: function=%d, value=%d\n",
              driverName, function, value);
    return status;
}




/** Report status of the driver.
  * Prints details about the driver if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >0 then driver details are printed.
  */
void URLDriver::report(FILE *fp, int details)
{

    fprintf(fp, "URL Driver %s\n", this->portName);
    if (details > 0) {
        int nx, ny, dataType;
        std::string urlName;
        getIntegerParam(ADSizeX, &nx);
        getIntegerParam(ADSizeY, &ny);
        getIntegerParam(NDDataType, &dataType);
        getStringParam(URLName, urlName);
        fprintf(fp, "  URL:               %s\n", urlName.c_str());
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    /* Invoke the base class method */
    ADDriver::report(fp, details);
}

/** Constructor for URLDriver; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to read the URL images,
  * and sets reasonable default values for parameters defined in this class, asynNDArrayDriver and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
URLDriver::URLDriver(const char *portName, int maxBuffers, size_t maxMemory, 
                     int priority, int stackSize)

    : ADDriver(portName, 1, 0, maxBuffers, maxMemory,
               0, 0, /* No interfaces beyond those set in ADDriver.cpp */
               0, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize)

{
    int status = asynSuccess;
    char versionString[20];
    const char *functionName = "URLDriver";

    /* Create the epicsEvents for signaling to the acquisition task when acquisition starts and stops */
    this->startEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->startEventId) {
        printf("%s:%s epicsEventCreate failure for start event\n",
            driverName, functionName);
        return;
    }
    this->stopEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->stopEventId) {
        printf("%s:%s epicsEventCreate failure for stop event\n",
            driverName, functionName);
        return;
    }

    createParam(URLNameString,      asynParamOctet, &URLName);

    /* Set some default values for parameters */
    status =  setStringParam (ADManufacturer, "URL Driver");
    status |= setStringParam (ADModel, "GraphicsMagick");
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", 
                  DRIVER_VERSION, DRIVER_REVISION, DRIVER_MODIFICATION);
    setStringParam(NDDriverVersion, versionString);
    setStringParam(ADSDKVersion, MagickLibVersionText);
    setStringParam(ADSerialNumber, "No serial number");
    setStringParam(ADFirmwareVersion, "No firmware");
    if (status) {
        printf("%s: unable to set camera parameters\n", functionName);
        return;
    }

    /* Create the thread that updates the images */
    status = (epicsThreadCreate("URLDriverTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)URLTaskC,
                                this) == NULL);
    if (status) {
        printf("%s:%s epicsThreadCreate failure for image task\n",
            driverName, functionName);
        return;
    }
}

/** Configuration command, called directly or from iocsh */
extern "C" int URLDriverConfig(const char *portName, int maxBuffers, size_t maxMemory, 
                               int priority, int stackSize)
{
    /* Initialize GraphicsMagick */
    InitializeMagick(NULL);

    new URLDriver(portName, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg URLDriverConfigArg0 = {"Port name", iocshArgString};
static const iocshArg URLDriverConfigArg1 = {"maxBuffers", iocshArgInt};
static const iocshArg URLDriverConfigArg2 = {"maxMemory", iocshArgInt};
static const iocshArg URLDriverConfigArg3 = {"priority", iocshArgInt};
static const iocshArg URLDriverConfigArg4 = {"stackSize", iocshArgInt};
static const iocshArg * const URLDriverConfigArgs[] =  {&URLDriverConfigArg0,
                                                          &URLDriverConfigArg1,
                                                          &URLDriverConfigArg2,
                                                          &URLDriverConfigArg3,
                                                          &URLDriverConfigArg4};
static const iocshFuncDef configURLDriver = {"URLDriverConfig", 5, URLDriverConfigArgs};
static void configURLDriverCallFunc(const iocshArgBuf *args)
{
    URLDriverConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival, args[4].ival);
}


static void URLDriverRegister(void)
{

    iocshRegister(&configURLDriver, configURLDriverCallFunc);
}

extern "C" {
epicsExportRegistrar(URLDriverRegister);
}
