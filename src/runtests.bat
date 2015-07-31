rem @echo off
setlocal enabledelayedexpansion
set tests_path=%~dp0\bin\%1\%~2
echo %tests_path%
if not exist !tests_path!\ goto nodir
!tests_path!\test_basics.exe --gtest_output="xml:!tests_path!\test_basics.exe.xml"
!tests_path!\dynamic_app.exe --gtest_output="xml:!tests_path!\dynamic_app.exe.xml"
!tests_path!\testsuite.exe --gtest_output="xml:!tests_path!\testsuite.exe.xml"
exit /b 0
:nodir

echo No test dir "!tests_path!\"
exit /b 1