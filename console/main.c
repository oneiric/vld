////////////////////////////////////////////////////////////////////////////////
//  $Id: main.c,v 1.3 2005/03/29 05:25:16 db Exp $
//
//  A simple C example program for testing the Visual Leak Detector.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vld.h"

int main (int argc, char *argv [])
{
    char *s = (char*)malloc(14);

    strncpy(s, "Hello World!\n", 14);
    printf(s);

    //free(s);

    return 0;
}