#include "ADDriver.h"

#ifdef ADURL_USE_CURL
    #include <curl/curl.h>
#endif

#define DRIVER_VERSION      2
#define DRIVER_REVISION     3
#define DRIVER_MODIFICATION 1

/** URL driver; reads images from URLs, such as Web cameras and Axis video servers, but also files, etc. */
class URLDriver : public ADDriver {
public:
    URLDriver(const char *portName, int maxBuffers, size_t maxMemory,
              int priority, int stackSize);

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual void report(FILE *fp, int details);
    void URLTask(); /**< Should be private, but gets called from C, so must be public */

protected:
    int URLName;
    #define FIRST_URL_DRIVER_PARAM URLName

    #ifdef ADURL_USE_CURL
    int useCurl;
    #endif

private:
    /* These are the methods that are new to this class */
    virtual asynStatus readImage();

    /* Our data */
    Image image;
    epicsEventId startEventId;
    epicsEventId stopEventId;
};

#define URLNameString "URL_NAME"

#ifdef ADURL_USE_CURL
    #define UseCurlString "USE_CURL"
#endif