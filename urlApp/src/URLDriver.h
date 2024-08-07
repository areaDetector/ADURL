#include "ADDriver.h"

#ifdef ADURL_USE_CURL
    #include <curl/curl.h>
    #include <fstream>
    #include <sys/stat.h>
    #include <unistd.h>
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

    #ifdef ADURL_USE_CURL
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);
    asynStatus completeFullPath();
    void initializeCurl();
    #endif

protected:
    int URLName;
    #define FIRST_URL_DRIVER_PARAM URLName

    #ifdef ADURL_USE_CURL
    int useCurl;
    int curlLoadConfig;
    int fileIsValid;
    int curlOptHttpAuth;
    int curlOptSSLVerifyHost;
    int curlOptSSLVerifyPeer;
    int curlOptUserName;
    int curlOptPassword;
    #define MAXCURLSTRCHARS 128
    CURL *curl = NULL;
    CURLcode res;

    /*Array to translate CurlHttpAuth options*/
    long unsigned int CurlHttpOptions [11] = {CURLAUTH_BASIC, CURLAUTH_DIGEST, CURLAUTH_DIGEST_IE, CURLAUTH_BEARER,
                         CURLAUTH_NEGOTIATE, CURLAUTH_NTLM, CURLAUTH_NTLM_WB, CURLAUTH_ANY,
                         CURLAUTH_ANYSAFE, CURLAUTH_ONLY, CURLAUTH_AWS_SIGV4};
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
    #define UseCurlString              "USE_CURL"
    #define CurlLoadConfigString       "CURL_LOAD_CONFIG"
    #define CurlFileIsValidString      "FILE_IS_VALID"
    #define CurlOptHttpAuthString      "ASYN_CURLOPT_HTTPAUTH"
    #define CurlOptSSLVerifyHostString "ASYN_CURLOPT_SSL_VERIFYHOST"
    #define CurlOptSSLVerifyPeerString "ASYN_CURLOPT_SSL_VERIFYPEER"
    #define CurlOptUserNameString      "ASYN_CURLOPT_USERNAME"
    #define CurlOptPasswordString      "ASYN_CURLOPT_PASSWORD"
#endif
