////////////////////////////////////////////////////////////////////////////////
//  $Id: main.cpp,v 1.2 2005/03/29 05:25:36 db Exp $
//
//  A simple C++ example program for testing the Visual Leak Detector.
//
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include "vld.h"

using namespace std;

int main (int argc, char *argv [])
{
    string *s = new string("Hello World!\n");

    cout << *s;

    //delete s;

    return 0;
}