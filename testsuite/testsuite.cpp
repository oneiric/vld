#include <cassert>
#include <cstdio>
#include <windows.h>

#include "vld.h"

enum action_e {
    a_malloc,
    a_new,
    numactions
};

#define MAXALLOC     1000
#define MAXBLOCKS    (MAXALLOC * numactions)
#define MAXDEPTH     10
#define MAXSIZE      64
#define NUMTHREADS   16
#define ONCEINAWHILE 10

typedef struct blockholder_s {
    action_e action;
    PVOID    block;
} blockholder_t;

typedef struct threadcontext_s {
    UINT index;
    BOOL leaky;
    BOOL terminated;
} threadcontext_t;

__declspec(thread) blockholder_t blocks [MAXBLOCKS];
__declspec(thread) ULONG         counts [numactions] = { 0 };
__declspec(thread) ULONG         total_allocs = 0;

ULONG random (ULONG max)
{
    FLOAT d;
    FLOAT r;
    ULONG v;

    r = ((FLOAT)rand()) / ((FLOAT)RAND_MAX);
    r *= ((FLOAT)max);
    d = r - ((ULONG)r);
    if (d >= 0.5) {
        v = ((ULONG)r) + 1;
    }
    else {
        v = (ULONG)r;
    }

    return v;
}

VOID allocateblock (action_e action, SIZE_T size)
{
    ULONG index;

    // Find the first unused index.
    for (index = 0; index < MAXBLOCKS; index++) {
        if (blocks[index].block == NULL) {
            break;
        }
    }
    blocks[index].action = action;

    // Now do the randomized allocation.        
    switch (action) {
        case a_malloc:
            blocks[index].block = malloc(size);
            break;

        case a_new:
            blocks[index].block = new BYTE [size];
            break;
    }
    counts[action]++;
    total_allocs++;
}

VOID freeblock (ULONG index)
{
    switch (blocks[index].action) {
        case a_malloc:
            free(blocks[index].block);
            break;

        case a_new:
            delete [] blocks[index].block;
            break;
    }
    blocks[index].block = NULL;
    counts[blocks[index].action]--;
    total_allocs--;
}

VOID recursivelyallocate (UINT depth, action_e action, SIZE_T size)
{
    if (depth == 0) {
        allocateblock(action, size);
    }
    else {
        recursivelyallocate(depth - 1, action, size);
    }
}

DWORD __stdcall runtestsuite (LPVOID param)
{
    action_e         action;
    BOOL             allocate_more = TRUE;
    threadcontext_t *context = (threadcontext_t*)param;
    UINT             depth;
    ULONG            index;
    SIZE_T           size;

    for (index = 0; index < MAXBLOCKS; index++) {
        blocks[index].block = NULL;
    }

    while (allocate_more == TRUE) {
        // Select a random allocation action and a random size.
        action = (action_e)random(numactions - 1);
        size = random(MAXSIZE);
        if (counts[action] == MAXALLOC) {
            // We've done enough of this type of allocation. Select another.
            continue;
        }

        // Allocate a block, using recursion to build up a stack of random
        // depth.
        depth = random(MAXDEPTH);
        recursivelyallocate(depth, action, size);

        // Every once in a while, free a random block.
        if (random(ONCEINAWHILE) == ONCEINAWHILE) {
            index = random(total_allocs);
            if (blocks[index].block != NULL) {
                freeblock(index);
            }
        }

        // See if we have allocated enough blocks using each type of action.
        for (index = 0; index < (numactions - 1); index++) {
            if (counts[index] < MAXALLOC) {
                allocate_more = TRUE;
                break;
            }
            allocate_more = FALSE;
        }
    }

    if (context->leaky == TRUE) {
        // This is the leaky thread. Free all blocks, except for one from each
        // action type.
        for (index = 0; index < MAXBLOCKS; index++) {
            if ((counts[blocks[index].action] > 1) &&
                (blocks[index].block != NULL)) {
                    freeblock(index);
            }
        }
        assert(total_allocs == numactions);
    }
    else {
        // Free all blocks.
        for (index = 0; index < MAXBLOCKS; index++) {
            if (blocks[index].block != NULL) {
                freeblock(index);
            }
        }
        assert(total_allocs == 0);
    }

    context->terminated = TRUE;
    return 0;
}

int main (int argc, char *argv [])
{
    threadcontext_t contexts [NUMTHREADS];
    DWORD           end;
    UINT            index;
#define MESSAGESIZE 512
    char            message [512];
    DWORD           start;
    UINT            leakythread;

    start = GetTickCount();

    //srand(start);
    srand(0);

    leakythread = random(NUMTHREADS - 1);

    for (index = 0; index < NUMTHREADS; ++index) {
        contexts[index].index = index;
        if (index == leakythread) {
            contexts[index].leaky = TRUE;
        }
        contexts[index].terminated = FALSE;
        CreateThread(NULL, 0, runtestsuite, &contexts[index], 0, NULL);
    }

    // Wait for all threads to terminate.
    for (index = 0; index < NUMTHREADS; ++index) {
        while (contexts[index].terminated == FALSE) {
            Sleep(10);
        }
    }

    end = GetTickCount();
    _snprintf_s(message, 512, _TRUNCATE, "Elapsed Time = %ums\n", end - start);
    OutputDebugString(message);

    return 0;
}
