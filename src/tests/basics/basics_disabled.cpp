// basics.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vld.h"
#include "Allocs.h"

#include <gtest/gtest.h>

extern void LeakMemory(LeakOption type, int repeat, bool bFree);

static const int repeats = 1;

class TestBasicsDisabled : public ::testing::Test
{
    virtual void SetUp()
    {
        VLDDisable();
    }

    virtual void TearDown()
    {
        VLDEnable();
    }
};

TEST_F(TestBasicsDisabled, Malloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eMalloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, New)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eNew, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, NewArray)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eNewArray, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, Calloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eCalloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, Realloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eRealloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, CoTaskMem)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eCoTaskMem, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, AlignedMalloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eAlignedMalloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, AlignedRealloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eAlignedRealloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, Strdup)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eStrdup, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, HeapAlloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eHeapAlloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, GetProcMalloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eGetProcMalloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, IMalloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemory(eIMalloc, repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}
