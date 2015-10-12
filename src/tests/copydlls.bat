REM Copying over Visual Leak Detector Dependencies
xcopy "%~dp0\..\..\vld.ini" "%~dp0\..\bin\" /y /d
copy "%~dp0\..\bin\%1\vld_%2.dll" "%~3\vld_%2.dll" /y
copy "%~dp0\..\bin\%1\vld_%2.pdb" "%~3\vld_%2.pdb" /y
copy "%~dp0\..\..\setup\dbghelp\%2\dbghelp.dll" "%~3\dbghelp.dll" /y
copy "%~dp0\..\..\setup\dbghelp\%2\Microsoft.DTfW.DHL.manifest" "%~3\Microsoft.DTfW.DHL.manifest" /y