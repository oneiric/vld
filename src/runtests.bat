@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET tests_path=%~dp0\bin\%1\%~2
IF NOT EXIST "!tests_path!\" GOTO nodir

CD %~dp0\bin

ECHO -------------------------------------------------------------------------------
ECHO [ RUNNING  ] %tests_path%\test_basics.exe
ECHO -------------------------------------------------------------------------------
!tests_path!\test_basics.exe --gtest_output="xml:!tests_path!\test_basics.exe.xml"
ECHO -------------------------------------------------------------------------------
ECHO [ RUNNING  ] %tests_path%\dynamic_app.exe
ECHO -------------------------------------------------------------------------------
!tests_path!\dynamic_app.exe --gtest_output="xml:!tests_path!\dynamic_app.exe.xml"
ECHO -------------------------------------------------------------------------------
ECHO [ RUNNING  ] %tests_path%\testsuite.exe
ECHO -------------------------------------------------------------------------------
!tests_path!\testsuite.exe --gtest_output="xml:!tests_path!\testsuite.exe.xml"
ECHO -------------------------------------------------------------------------------
ECHO [ RUNNING  ] %tests_path%\corruption.exe
ECHO -------------------------------------------------------------------------------
!tests_path!\corruption.exe --gtest_output="xml:!tests_path!\corruption.exe.xml"
ECHO -------------------------------------------------------------------------------
ECHO [ RUNNING  ] %tests_path%\vld_unload.exe
ECHO -------------------------------------------------------------------------------
!tests_path!\vld_unload.exe --gtest_output="xml:!tests_path!\vld_unload.exe.xml"
ECHO -------------------------------------------------------------------------------
ECHO [ RUNNING  ] %tests_path%\vld_main.exe
ECHO -------------------------------------------------------------------------------
!tests_path!\vld_main.exe
if %errorlevel% neq 0 exit /b %errorlevel%
ECHO -------------------------------------------------------------------------------
EXIT /b 0

:nodir
ECHO No test dir: "!tests_path!\"
EXIT /b 1