/*  httpTest.cpp
 *  Mark Rivers
 *  Based on httpclient by Uwe Flechsig
 *  but using Magick++ interface */


/* #define URL         "http://x07ma-cam-4/axis-cgi/jpg/image.cgi?resolution=4CIF" */
/* #define URL         "http://slsbl.web.psi.ch/httpclient-default-image.jpg"  */
#define URL            "http://www.solarstrahlung-dietzenbach.de/images/sonne.jpg"

#include <Magick++.h>
#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;
using namespace Magick;

int main(int argc, char **argv) 
{
    static const char *infile = URL;
    static const char *outfile = NULL;

    InitializeMagick(NULL);
 
    if (argc > 1) infile= argv[1];
    if (argc > 2) outfile= argv[2];
    printf("infile: %s\n", infile);
    printf("outfile: %s\n", outfile);
    try {
        Image image(infile);
        //image.rotate(90.);
        if (outfile) image.write(outfile);
    }
    catch( exception &error_ )
    {
        cout << "Caught exception: " << error_.what() << endl;
        return 1;
    }

    return 0;
}
