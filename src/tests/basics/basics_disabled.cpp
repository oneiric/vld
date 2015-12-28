// basics.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vld.h"
#include "Allocs.h"

#include <gtest/gtest.h>

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
    LeakMemoryMalloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, New)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryNew(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, NewArray)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryNewArray(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, Calloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryCalloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, Realloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryRealloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, CoTaskMem)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryCoTaskMem(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, AlignedMalloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryAlignedMalloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, AlignedRealloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryAlignedRealloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, Strdup)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryStrdup(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, HeapAlloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryHeapAlloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, GetProcMalloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryGetProcMalloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}

TEST_F(TestBasicsDisabled, IMalloc)
{
    int prev = static_cast<int>(VLDGetLeaksCount());
    LeakMemoryIMalloc(repeats, false);
    int total = static_cast<int>(VLDGetLeaksCount());
    int leaks = total - prev;
    ASSERT_EQ(0, leaks);
}
