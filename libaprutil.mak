# Microsoft Developer Studio Generated NMAKE File, Based on libaprutil.dsp
!IF "$(CFG)" == ""
CFG=libaprutil - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libaprutil - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libaprutil - Win32 Release" && "$(CFG)" !=\
 "libaprutil - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libaprutil.mak" CFG="libaprutil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libaprutil - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libaprutil - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libaprutil - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libaprutil.dll"

!ELSE 

ALL : "aprutil - Win32 Release" "libapr - Win32 Release"\
 "$(OUTDIR)\libaprutil.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 ReleaseCLEAN" "aprutil - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\libaprutil.idb"
	-@erase "$(INTDIR)\libaprutil.obj"
	-@erase "$(OUTDIR)\libaprutil.dll"
	-@erase "$(OUTDIR)\libaprutil.exp"
	-@erase "$(OUTDIR)\libaprutil.lib"
	-@erase "$(OUTDIR)\libaprutil.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /I "./include" /I "../apr/include" /I\
 "./include/private" /I "./src/dbm/sdbm" /D "NDEBUG" /D "APR_DECLARE_EXPORT" /D\
 "WIN32" /D "_WINDOWS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\libaprutil" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libaprutil.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo\
 /base:"0x6ED0000" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\libaprutil.pdb" /map:"$(INTDIR)\libaprutil.map" /machine:I386\
 /def:".\libaprutil.def" /out:"$(OUTDIR)\libaprutil.dll"\
 /implib:"$(OUTDIR)\libaprutil.lib" 
DEF_FILE= \
	".\libaprutil.def"
LINK32_OBJS= \
	"$(INTDIR)\libaprutil.obj" \
	"..\apr\Release\libapr.lib" \
	".\LibR\aprutil.lib"

"$(OUTDIR)\libaprutil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libaprutil - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libaprutil.dll"

!ELSE 

ALL : "aprutil - Win32 Debug" "libapr - Win32 Debug" "$(OUTDIR)\libaprutil.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 DebugCLEAN" "aprutil - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\libaprutil.idb"
	-@erase "$(INTDIR)\libaprutil.obj"
	-@erase "$(OUTDIR)\libaprutil.dll"
	-@erase "$(OUTDIR)\libaprutil.exp"
	-@erase "$(OUTDIR)\libaprutil.lib"
	-@erase "$(OUTDIR)\libaprutil.map"
	-@erase "$(OUTDIR)\libaprutil.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /I "./include" /I "../apr/include" /I\
 "./include/private" /I "./src/dbm/sdbm" /D "_DEBUG" /D "APR_DECLARE_EXPORT" /D\
 "WIN32" /D "_WINDOWS" /D "APU_USE_SDBM" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\libaprutil" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libaprutil.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib advapi32.lib ws2_32.lib mswsock.lib ole32.lib /nologo\
 /base:"0x6ED00000" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\libaprutil.pdb" /map:"$(INTDIR)\libaprutil.map" /debug\
 /machine:I386 /def:".\libaprutil.def" /out:"$(OUTDIR)\libaprutil.dll"\
 /implib:"$(OUTDIR)\libaprutil.lib" 
DEF_FILE= \
	".\libaprutil.def"
LINK32_OBJS= \
	"$(INTDIR)\libaprutil.obj" \
	"..\apr\Debug\libapr.lib" \
	".\LibD\aprutil.lib"

"$(OUTDIR)\libaprutil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "libaprutil - Win32 Release" || "$(CFG)" ==\
 "libaprutil - Win32 Debug"

!IF  "$(CFG)" == "libaprutil - Win32 Release"

"libapr - Win32 Release" : 
   cd "\test\httpd-2.0\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 Release" 
   cd "..\apr-util"

"libapr - Win32 ReleaseCLEAN" : 
   cd "\test\httpd-2.0\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\libapr.mak" CFG="libapr - Win32 Release"\
 RECURSE=1 
   cd "..\apr-util"

!ELSEIF  "$(CFG)" == "libaprutil - Win32 Debug"

"libapr - Win32 Debug" : 
   cd "\test\httpd-2.0\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 Debug" 
   cd "..\apr-util"

"libapr - Win32 DebugCLEAN" : 
   cd "\test\httpd-2.0\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\libapr.mak" CFG="libapr - Win32 Debug"\
 RECURSE=1 
   cd "..\apr-util"

!ENDIF 

!IF  "$(CFG)" == "libaprutil - Win32 Release"

"aprutil - Win32 Release" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) /F ".\aprutil.mak" CFG="aprutil - Win32 Release" 
   cd "."

"aprutil - Win32 ReleaseCLEAN" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\aprutil.mak" CFG="aprutil - Win32 Release"\
 RECURSE=1 
   cd "."

!ELSEIF  "$(CFG)" == "libaprutil - Win32 Debug"

"aprutil - Win32 Debug" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) /F ".\aprutil.mak" CFG="aprutil - Win32 Debug" 
   cd "."

"aprutil - Win32 DebugCLEAN" : 
   cd "."
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\aprutil.mak" CFG="aprutil - Win32 Debug"\
 RECURSE=1 
   cd "."

!ENDIF 

SOURCE=.\src\misc\win32\libaprutil.c

"$(INTDIR)\libaprutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

