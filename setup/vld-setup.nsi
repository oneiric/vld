################################################################################
#  $Id: vld-setup.nsi,v 1.2 2006/10/31 17:16:36 db Exp $
#  Visual Leak Detector (Version 1.9c) - NSIS Installation Script
#  Copyright (c) 2006 Dan Moulding
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
#
#  See COPYING.txt for the full terms of the GNU General Public License.
#
################################################################################

!include "Library.nsh"  # Provides the dynamic link library installation system
!include "LogicLib.nsh" # Provides useable conditional script syntax
!include "MUI.nsh"      # Provides the modern user-interface
!include "path-env.nsh" # Provides path environment variable manipulation

# Version number
!define VLD_VERSION "1.9c"

# Define build system paths
!define DTFW_PATH "C:\Program Files\Debugging Tools for Windows"

# Define installer paths
!define BIN_PATH     "$INSTDIR\bin"
!define INCLUDE_PATH "$INSTDIR\include"
!define LIB_PATH     "$INSTDIR\lib"
!define SRC_PATH     "$INSTDIR\src"

# Define registry keys
!define REG_KEY_PRODUCT   "Software\Visual Leak Detector"
!define REG_KEY_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\Visual Leak Detector"

# Define installer attributes
InstallDir       "$PROGRAMFILES\Visual Leak Detector"
InstallDirRegKey HKLM "${REG_KEY_PRODUCT}" "InstallPath"
Name             "Visual Leak Detector ${VLD_VERSION}"
OutFile          "vld-${VLD_VERSION}-setup.exe"
SetCompressor    /SOLID lzma

# Declare global variables
Var INSTALLED_VERSION
	
# Define the installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\COPYING.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_SHOWREADME      "$INSTDIR\README.html"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View Documentation"
!insertmacro MUI_PAGE_FINISH

# Define the uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Set the modern UI language
!insertmacro MUI_LANGUAGE "English"

################################################################################
#
# Installation
#
Function .onInit
	ReadRegStr $INSTALLED_VERSION HKLM "${REG_KEY_PRODUCT}" "InstalledVersion"
	${IF} $INSTALLED_VERSION = ${VLD_VERSION}
		MessageBox MB_OKCANCEL "Setup has detected that Visual Leak Detector version $INSTALLED_VERSION is already installed on this computer.$\n$\nClick 'OK' if you want to continue and repair the existing installation. Click 'Cancel' if you want to abort installation." \
			IDOK continue IDCANCEL abort
        abort:
			Abort
        continue:
	${ENDIF}
FunctionEnd

Section "Uninstaller"
	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${REG_KEY_UNINSTALL}" "DisplayName" "Visual Leak Detector ${VLD_VERSION}"
	WriteRegStr HKLM "${REG_KEY_UNINSTALL}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${REG_KEY_UNINSTALL}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${REG_KEY_UNINSTALL}" "Publisher" "Dan Moulding"
	WriteRegStr HKLM "${REG_KEY_UNINSTALL}" "URLInfoAbout" "http://www.danm.net"
	WriteRegStr HKLM "${REG_KEY_UNINSTALL}" "DisplayVersion" "${VLD_VERSION}"
	WriteRegDWORD HKLM "${REG_KEY_UNINSTALL}" "NoModify" 1
	WriteRegDWORD HKLM "${REG_KEY_UNINSTALL}" "NoRepair" 1
SectionEnd

Section "Registry Keys"
	WriteRegStr HKLM "${REG_KEY_PRODUCT}" "InstallPath" "$INSTDIR"
	WriteRegStr HKLM "${REG_KEY_PRODUCT}" "InstalledVersion" "${VLD_VERSION}"
	WriteRegStr HKLM "${REG_KEY_PRODUCT}" "IniFile" "$INSTDIR\vld.ini"
SectionEnd

Section "Header File"
	SetOutPath "${INCLUDE_PATH}"
	File "..\vld.h"
SectionEnd

Section "Import Library"
	SetOutPath "${LIB_PATH}"
	File "..\Release\vld.lib"
SectionEnd

Section "Dynamic Link Libraries"
	SetOutPath "${BIN_PATH}"
	!insertmacro InstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "..\Release\vld.dll" "${BIN_PATH}\vld.dll" $INSTDIR
	MessageBox MB_YESNO "Visual Leak Detector needs the location of vld.dll to be added to your PATH environment variable.$\n$\nWould you like the installer to add it to the path now? If you select No, you'll need to add it to the path manually." \
		IDYES addtopath IDNO skipaddtopath
	addtopath:
	Push "${BIN_PATH}"
	Call AddToPath
	skipaddtopath:
	!insertmacro InstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "${DTFW_PATH}\dbghelp.dll" "${BIN_PATH}\dbghelp.dll" $INSTDIR
SectionEnd

Section "Configuration File"
	SetOutPath "$INSTDIR"
	File "..\vld.ini"
SectionEnd

Section "Source Code"
	SetOutPath "${SRC_PATH}"
	File "..\*.cpp"
	File "..\*.h"
	File "..\vld.vcproj"
SectionEnd

Section "Documentation"
	SetOutPath "$INSTDIR"
	File "..\CHANGES.txt"
	File "..\COPYING.txt"
	File "..\README.html"
SectionEnd


################################################################################
#
# Uninstallation
#
Section "un.Registry Keys"
	DeleteRegKey HKLM "${REG_KEY_PRODUCT}"
SectionEnd

Section "un.Header File"
	Delete "${INCLUDE_PATH}\vld.h"
	RMDir "${INCLUDE_PATH}"
SectionEnd

Section "un.Import Library"
	Delete "${LIB_PATH}\vld.lib"
	RMDir "${LIB_PATH}"
SectionEnd

Section "un.Dynamic Link Library"
	!insertmacro UnInstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "${BIN_PATH}\vld.dll"
	!insertmacro UnInstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "${BIN_PATH}\dbghelp.dll"
	RMDir "${BIN_PATH}"
	Push "${BIN_PATH}"
	Call un.RemoveFromPath
SectionEnd

Section "un.Configuration File"
	Delete "$INSTDIR\vld.ini"
SectionEnd

Section "un.Source Code"
	Delete "${SRC_PATH}\*.cpp"
	Delete "${SRC_PATH}\*.h"
	Delete "${SRC_PATH}\vld.vcproj"
	RMDir "${SRC_PATH}"
SectionEnd

Section "un.Documentation"
	Delete "$INSTDIR\CHANGES.txt"
	Delete "$INSTDIR\COPYING.txt"
	Delete "$INSTDIR\README.html"
SectionEnd

Section "un.Uninstaller"
	Delete "$INSTDIR\uninstall.exe"
	RMDir "$INSTDIR"
	DeleteRegKey HKLM "${REG_KEY_UNINSTALL}"
SectionEnd
