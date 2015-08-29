REM Copying over Visual Leak Detector Dependencies
xcopy %~p0\..\..\vld.ini %~p0\..\bin\ /y /d
xcopy %~p0\..\bin\%1\vld_%2.dll %3\vld_%2.dll /y
xcopy %~p0\..\bin\%1\vld_%2.pdb %3\vld_%2.pdb /y
xcopy %~p0\..\..\setup\dbghelp\%2\dbghelp.dll %3\dbghelp.dll /y
xcopy %~p0\..\..\setup\dbghelp\%2\Microsoft.DTfW.DHL.manifest %3\Microsoft.DTfW.DHL.manifest /y