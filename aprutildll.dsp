# Microsoft Developer Studio Project File - Name="aprutildll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=aprutildll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aprutildll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aprutildll.mak" CFG="aprutildll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aprutildll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aprutildll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aprutildll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./include" /I "./include/arch" /I "./include/arch/win32" /I "./include/arch/unix" /D "NDEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 aprutil.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /base:"0x6EE0000" /subsystem:windows /dll /map /machine:I386 /out:"Release/aprutil.dll" /libpath:"LibR"
# ADD LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo /base:"0x6EE0000" /subsystem:windows /dll /map /machine:I386 /out:"Release/aprutil.dll"

!ELSEIF  "$(CFG)" == "aprutildll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /I "./include" /I "./include/arch" /I "./include/arch/win32" /I "./include/arch/unix" /D "_DEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 aprutil.lib kernel32.lib advapi32.lib ws2_32.lib mswsock.lib /nologo /base:"0x6EE0000" /subsystem:windows /dll /map /debug /machine:I386 /out:"Debug/aprutil.dll" /libpath:"LibD"
# ADD LINK32 kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo /base:"0x6EE00000" /subsystem:windows /dll /debug /machine:I386 /out:"Debug/aprutil.dll"
# SUBTRACT LINK32 /incremental:no /map

!ENDIF 

# Begin Target

# Name "aprutildll - Win32 Release"
# Name "aprutildll - Win32 Debug"
# Begin Source File

SOURCE=.\src\misc\win32\aprutil.c
# End Source File
# Begin Source File

SOURCE=.\aprutil.def
# End Source File
# End Target
# End Project
