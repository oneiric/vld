////////////////////////////////////////////////////////////////////////////////
//  $Id: main.cpp,v 1.1.1.1 2005/03/11 22:56:50 dmouldin Exp $
//
//  A simple C++ example program for testing the Visual Leak Detector.
//
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

using namespace std;

int main (int argc, char *argv [])
{
    string *s = new string("Hello World!\n");

    cout << *s;

    //delete s;

    return 0;
}