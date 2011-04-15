@echo off

.\x64\Debug\test_basics 
.\x64\Debug\test_basics invalid_argument 5
.\x64\Debug\test_basics malloc 5
.\x64\Debug\test_basics new 5
.\x64\Debug\test_basics new_array 5
.\x64\Debug\test_basics calloc 5
.\x64\Debug\test_basics realloc 5

pause

@echo on