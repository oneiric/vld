#pragma once

extern void LeakMemoryMalloc(int repeat, bool bFree = false);
extern void LeakMemoryNew(int repeat, bool bFree = false);
extern void LeakMemoryNewArray(int repeat, bool bFree = false);
extern void LeakMemoryCalloc(int repeat, bool bFree = false);
extern void LeakMemoryRealloc(int repeat, bool bFree = false);
extern void LeakMemoryCoTaskMem(int repeat, bool bFree = false);
extern void LeakMemoryAlignedMalloc(int repeat, bool bFree = false);
extern void LeakMemoryAlignedRealloc(int repeat, bool bFree = false);
extern void LeakMemoryStrdup(int repeat, bool bFree = false);
extern void LeakMemoryHeapAlloc(int repeat, bool bFree = false);
extern void LeakMemoryIMalloc(int repeat, bool bFree = false);
extern void LeakMemoryGetProcMalloc(int repeat, bool bFree = false);

extern const int repeats;
