////////////////////////////////////////////////////////////////////////////////
//  $Id: main.c,v 1.4 2005/03/31 02:10:54 db Exp $
//
//  A simple C example program for testing the Visual Leak Detector.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include Visual Leak Detector
#include <vld.h>

int main (int argc, char *argv [])
{
    char *s = (char*)malloc(14);

    strncpy(s, "Hello World!\n", 14);
    printf(s);

    //free(s);

    return 0;
}