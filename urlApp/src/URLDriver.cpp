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

#ifdef ADURL_USE_CURL
void URLDriver::initializeCurl(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    this->curl = curl_easy_init();
    if (!curl){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s:%s: ERROR, cannot initialize curl pointer.\n", driverName, __func__);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlBuffer);

}
#endif

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
    #ifdef ADURL_USE_CURL
    int use_curl;
    getIntegerParam(useCurl, &use_curl);
    #endif
    if (strlen(URLString) == 0) return(asynError);
    try {
        #ifdef ADURL_USE_CURL
        if (use_curl) {
            this->curlBuffer.clear();
            this->res = curl_easy_perform(curl);
            if (res != CURLE_OK){
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: curl read error %d\n",
                        driverName, functionName, res);
                return(asynError);
            }
            Blob blob(&curlBuffer[0], curlBuffer.size());
            image.read(blob);
        } else {
            image.read(URLString);
        }
        #else
        image.read(URLString);
        #endif
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
    #ifdef ADURL_USE_CURL
    } else if (function==curlOptHttpAuth) {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CurlHttpOptions[value]);
    } else if (function==curlOptSSLVerifyHost) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)value);
    } else if (function==curlOptSSLVerifyPeer) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, value);
    } else if (function==curlLoadConfig) {
        this->loadConfigFile();
    #endif
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

#ifdef ADURL_USE_CURL
asynStatus URLDriver::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{

    int addr = 0;
    int function = pasynUser->reason;
    int status = 0;
    char param[MAXCURLSTRCHARS] = {'\0'};

    status |= setStringParam(addr, function, (char *)value);

    if (function < FIRST_URL_DRIVER_PARAM) {

        if (function==NDFileName or function==NDFilePath){
            this->completeFullPath();
        }

        status |= ADDriver::writeOctet(pasynUser, value, nChars, nActual);

    } else if (function == curlOptUserName) {
        getStringParam(curlOptUserName, MAXCURLSTRCHARS, param);
        curl_easy_setopt(curl, CURLOPT_USERNAME, param);
    } else if (function == curlOptPassword) {
        getStringParam(curlOptPassword, MAXCURLSTRCHARS, param);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, param);
    } else if (function == URLName) {
        getStringParam(URLName, MAXCURLSTRCHARS, param);
        curl_easy_setopt(curl, CURLOPT_URL, param);
    }

    callParamCallbacks(addr);
    *nActual = nChars;
    return (asynStatus)status;

}

/* Called each time filePath or fileName is changed to check if file is accessible.
   Should only be called inside writeOctet() so needn't call callbacks. */
asynStatus URLDriver::completeFullPath()
{

    char fullFileName[MAX_FILENAME_LEN];
    int status = 0;
    const char * functionName = "completeFullPath";
    struct stat file;

    status = ADDriver::createFileName(2*MAX_FILENAME_LEN, fullFileName);

    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s:%s: Failed to create full filename.\n", driverName, functionName);
        return (asynStatus)status;
    }

    setStringParam(NDFullFileName, fullFileName);

    /* Check if file is accessible */
    if (stat(fullFileName, &(file)) == 0 &&
    S_ISREG(file.st_mode) &&
    access(fullFileName, R_OK) == 0){setIntegerParam(fileIsValid, 1);}
    else {setIntegerParam(fileIsValid, 0);}

    return asynSuccess;

}

asynStatus URLDriver::loadConfigFile()
{

    const char * functionName = "loadConfigFile";
    char fullFileName[MAX_FILENAME_LEN];
    std::ifstream file;
    std::string line, key, value; int valueInt;
    asynParamType type;
    int param, status = 0;
    size_t nActual = 0;

    getStringParam(NDFullFileName, MAX_FILENAME_LEN, fullFileName);

    file.open(fullFileName);
    if (!file) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: ERROR, cannot open file %s.\n", driverName, functionName, fullFileName);
        return asynError;
    }

    while (getline(file, line)) {
        key = line.substr(0,line.find("="));
        value = line.substr(line.find("=")+1, line.back());

        /* Taking spaces out of strings */
        key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
        value.erase(std::remove(value.begin(), value.end(), ' '), value.end());

        /* Finding which asyn parameter corersponds to option */
        key = "ASYN_" + key;
        asynPortDriver::findParam(key.c_str(), &param);
        /* asynUser to call writeOctet or writeInt32 later */
        asynUser tempUser{.reason = param};

        /* If param is credential, set curlOption but don't set asyn record*/
        if (param == curlOptUserName) {
            curl_easy_setopt(curl, CURLOPT_USERNAME, value.c_str());
            continue;
        } else if (param == curlOptPassword) {
            curl_easy_setopt(curl, CURLOPT_PASSWORD, value.c_str());
            continue;
        }

        if (param == -1){
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: ERROR, cannot find parameter %s from config file."
                        " Is this parameter implemented?\n",
                        driverName, functionName, key.c_str());

            return asynError;
        }

        asynPortDriver::getParamType(param, &type);
        switch (type) {
            case asynParamInt32:
                try {
                    valueInt = stoi(value);
                } catch (std::invalid_argument&) {
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                              "%s:%s: ERROR, cannot convert value %s to int for"
                              " parameter %s\n",
                              driverName, functionName, value.c_str(), key.c_str());
                    return asynError;
                }
                status |= this->writeInt32(&tempUser, (epicsInt32)valueInt);
                break;
            case asynParamOctet:
                status |= this->writeOctet(&tempUser, value.c_str(), value.size(), &nActual);
                break;
            default:
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: ERROR, parameter %s is of asynParam type"
                        " %d. This function only deals with asynParamOctet (%d)"
                        " and asynParamInt32 (%d).\n",
                        driverName, functionName, key.c_str(), type,
                        asynParamOctet, asynParamInt32);
                return asynError;
        }


    }

    file.close();
    return (asynStatus)status;

}

size_t URLDriver::curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    int totalSize = size * nmemb;

    ((std::vector<char>*)userp)->insert(((std::vector<char>*)userp)->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
}

#endif


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

    #ifdef ADURL_USE_CURL
    createParam(UseCurlString,              asynParamInt32, &useCurl);
    createParam(CurlLoadConfigString,       asynParamInt32, &curlLoadConfig);
    createParam(CurlFileIsValidString,      asynParamInt32, &fileIsValid);
    createParam(CurlOptHttpAuthString,      asynParamInt32, &curlOptHttpAuth);
    createParam(CurlOptSSLVerifyHostString, asynParamInt32, &curlOptSSLVerifyHost);
    createParam(CurlOptSSLVerifyPeerString, asynParamInt32, &curlOptSSLVerifyPeer);
    createParam(CurlOptUserNameString,      asynParamOctet, &curlOptUserName);
    createParam(CurlOptPasswordString,      asynParamOctet, &curlOptPassword);

    setIntegerParam(useCurl,              0);
    setIntegerParam(curlOptHttpAuth,      0);
    setIntegerParam(curlOptSSLVerifyHost, 2L);
    setIntegerParam(curlOptSSLVerifyPeer, 1);
    setStringParam(curlOptUserName,       "\0");
    setStringParam(curlOptPassword,       "\0");

    /* FileTEmplate parameter won't use complicated templates here */
    setStringParam(NDFileTemplate, "%s%s");
    this->initializeCurl();
    #endif

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
