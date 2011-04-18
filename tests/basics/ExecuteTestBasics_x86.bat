@echo off

.\Win32\Debug\test_basics 
.\Win32\Debug\test_basics invalid_argument 5
.\Win32\Debug\test_basics malloc 5
.\Win32\Debug\test_basics new 5
.\Win32\Debug\test_basics new_array 5
.\Win32\Debug\test_basics calloc 5
.\Win32\Debug\test_basics realloc 5
.\Win32\Debug\test_basics CoTaskMem 5

pause

@echo on