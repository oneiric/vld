#pragma once


enum LeakOption
{
	eMalloc,  // "malloc"
	eNew,     // "new"
	eNewArray,// "new_array"
	eCalloc,  // "calloc"
	eRealloc, // "realloc"
	eCoTaskMem,  // For COM, use "CoTaskMem"
	eAlignedMalloc,  // "_aligned_malloc"
	eCount,
};



