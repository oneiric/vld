// dynamic.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "dynamic.h"
#include "../basics/Allocs.h"
#include "../basics/LeakOption.h"

extern "C" {
// This is an example of an exported variable
DYNAMIC_API int ndynamic = 0;

void LeakMemory(LeakOption type, int repeat)
{
	for (int i = 0; i < repeat; i++)
	{
		Alloc(type);
	}
}

// This is an example of an exported function.
DYNAMIC_API int SimpleLeak_New(void)
{

	LeakMemory(eMalloc, 3);
	LeakMemory(eNew, 3);
	LeakMemory(eNewArray, 3);
	LeakMemory(eCalloc, 3);
	LeakMemory(eRealloc, 3);


	int* pint = new int(5);

	return 42;
}

// This is the constructor of a class that has been exported.
// see dynamic.h for the class definition
Cdynamic::Cdynamic()
{
	return;
}

}