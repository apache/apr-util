# Microsoft Developer Studio Project File - Name="aprutil" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=aprutil - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aprutil.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aprutil.mak" CFG="aprutil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aprutil - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "aprutil - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "aprutil - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "LibR"
# PROP BASE Intermediate_Dir "LibR"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "LibR"
# PROP Intermediate_Dir "LibR"
# PROP Target_Dir ""
RSC=rc.exe
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./include" /I "../apr/include" /D "NDEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "LibD"
# PROP BASE Intermediate_Dir "LibD"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "LibD"
# PROP Intermediate_Dir "LibD"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
RSC=rc.exe
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
# ADD BASE CPP /nologo /MTd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /I "./include" /I "../apr/include" /D "_DEBUG" /D "APR_DECLARE_EXPORT" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "aprutil - Win32 Release"
# Name "aprutil - Win32 Debug"
# Begin Group "buckets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\buckets\ap_buckets.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_eos.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_file.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_flush.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_heap.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_mmap.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_pipe.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_pool.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_refcount.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_simple.c
# End Source File
# Begin Source File

SOURCE=.\src\buckets\ap_buckets_socket.c
# End Source File
# End Group
# Begin Group "crypto"

# PROP Default_Filter ""
# End Group
# Begin Group "dbm"

# PROP Default_Filter ""
# End Group
# Begin Group "encoding"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\encoding\ap_base64.c
# End Source File
# Begin Source File

SOURCE=.\src\encoding\ap_sha1.c
# End Source File
# End Group
# Begin Group "hooks"

# PROP Default_Filter ""
# End Group
# Begin Group "uri"

# PROP Default_Filter ""
# End Group
# Begin Group "xml"

# PROP Default_Filter ""
# End Group
# End Target
# End Project
