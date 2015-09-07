@ECHO OFF
CLS
CMD /C "runtests Win32 Release-%1"
CMD /C "runtests Win32 Release_StaticCrt-%1"
CMD /C "runtests Win32 Debug-%1"
CMD /C "runtests Win32 Debug_StaticCrt-%1"
CMD /C "runtests Win32 Debug(Release)-%1"
CMD /C "runtests Win32 Debug(Release)_StaticCrt-%1"

CMD /C "runtests x64 Release-%1"
CMD /C "runtests x64 Release_StaticCrt-%1"
CMD /C "runtests x64 Debug-%1"
CMD /C "runtests x64 Debug_StaticCrt-%1"
CMD /C "runtests x64 Debug(Release)-%1"
CMD /C "runtests x64 Debug(Release)_StaticCrt-%1"
