////////////////////////////////////////////////////////////////////////////////
//  $Id: main.cpp,v 1.3 2005/03/31 02:11:08 db Exp $
//
//  A simple C++ example program for testing the Visual Leak Detector.
//
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

// Include Visual Leak Detector
#include <vld.h>

using namespace std;

int main (int argc, char *argv [])
{
    string *s = new string("Hello World!\n");

    cout << *s;

    //delete s;

    return 0;
}