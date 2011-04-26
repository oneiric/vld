#include "stdafx.h"
#include "Allocs.h"

#include <new> // for placement new
#include <crtdbg.h>
#include <ObjBase.h>

void AllocF(LeakOption type)
{
	int* leaked_memory = NULL;
	int* leaked_memory_dbg = NULL;
	if (type == eMalloc)
	{
		leaked_memory     = (int*)malloc(78);
		leaked_memory_dbg = (int*)_malloc_dbg(80, _NORMAL_BLOCK,__FILE__,__LINE__);
	} 
	else if (type == eNew)
	{
		leaked_memory = new int(4);
		leaked_memory_dbg = new (_NORMAL_BLOCK, __FILE__, __LINE__) int(7);
	}
	else if (type == eNewArray)
	{
		leaked_memory = new int[3];
		leaked_memory_dbg = new (_NORMAL_BLOCK,__FILE__,__LINE__) int[4];

		// placement new operator
		int temp[3];
		void* place = temp;
		float* placed_mem = new (place) float[3]; // doesn't work. Nothing gets patched by vld
	}
	else if (type == eCalloc)
	{
		leaked_memory     = (int*)calloc(47,sizeof(int));
		leaked_memory_dbg = (int*)_calloc_dbg(39, sizeof(int), _NORMAL_BLOCK, __FILE__, __LINE__);
	}
	else if (type == eRealloc)
	{
		int* temp = (int*)malloc(17);
		leaked_memory = (int*)realloc(temp, 23);
		int* temp_dbg = (int*)malloc(9);
		leaked_memory_dbg = (int*)_realloc_dbg(temp_dbg, 21, _NORMAL_BLOCK, __FILE__, __LINE__);
	}
	else if (type == eCoTaskMem)
	{
		void* leaked = CoTaskMemAlloc(7);
		void* realloced = CoTaskMemRealloc(leaked, 29);
	}
}

void AllocE(LeakOption type)
{
	AllocF(type);
}

void AllocD(LeakOption type)
{
	AllocE(type);
}

void AllocC(LeakOption type)
{
	AllocD(type);
}

void AllocB(LeakOption type)
{
	AllocC(type);
}

void AllocA(LeakOption type)
{
	AllocB(type);
}

void Alloc(LeakOption type)
{
	AllocA(type);
}