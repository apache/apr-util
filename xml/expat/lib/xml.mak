# Microsoft Developer Studio Generated NMAKE File, Based on xml.dsp
!IF "$(CFG)" == ""
CFG=xml - Win32 Debug
!MESSAGE No configuration specified. Defaulting to xml - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "xml - Win32 Release" && "$(CFG)" != "xml - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xml.mak" CFG="xml - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xml - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xml - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe

!IF  "$(CFG)" == "xml - Win32 Release"

OUTDIR=.\LibR
INTDIR=.\LibR
# Begin Custom Macros
OutDir=.\LibR
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\xml.lib"

!ELSE 

ALL : "$(OUTDIR)\xml.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\xmlparse.obj"
	-@erase "$(INTDIR)\xmlrole.obj"
	-@erase "$(INTDIR)\xmltok.obj"
	-@erase "$(OUTDIR)\xml.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

RSC=rc.exe
CPP_PROJ=/nologo /MD /W3 /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /D VERSION=\"expat_1.95.1\" /D XML_MAJOR_VERSION=1 /D\
 XML_MINOR_VERSION=95 /D XML_MICRO_VERSION=1 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\LibR/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xml.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\xml.lib" 
LIB32_OBJS= \
	"$(INTDIR)\xmlparse.obj" \
	"$(INTDIR)\xmlrole.obj" \
	"$(INTDIR)\xmltok.obj"

"$(OUTDIR)\xml.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xml - Win32 Debug"

OUTDIR=.\LibD
INTDIR=.\LibD
# Begin Custom Macros
OutDir=.\LibD
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\xml.lib"

!ELSE 

ALL : "$(OUTDIR)\xml.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\xmlparse.obj"
	-@erase "$(INTDIR)\xmlrole.obj"
	-@erase "$(INTDIR)\xmltok.obj"
	-@erase "$(OUTDIR)\xml.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

RSC=rc.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /I "." /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_MBCS" /D VERSION=\"expat_1.95.1\" /D XML_MAJOR_VERSION=1 /D\
 XML_MINOR_VERSION=95 /D XML_MICRO_VERSION=1 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\LibD/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xml.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\xml.lib" 
LIB32_OBJS= \
	"$(INTDIR)\xmlparse.obj" \
	"$(INTDIR)\xmlrole.obj" \
	"$(INTDIR)\xmltok.obj"

"$(OUTDIR)\xml.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

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


!IF "$(CFG)" == "xml - Win32 Release" || "$(CFG)" == "xml - Win32 Debug"
SOURCE=.\xmlparse.c

!IF  "$(CFG)" == "xml - Win32 Release"

DEP_CPP_XMLPA=\
	".\config.h"\
	".\expat.h"\
	".\xmlrole.h"\
	".\xmltok.h"\
	

"$(INTDIR)\xmlparse.obj" : $(SOURCE) $(DEP_CPP_XMLPA) "$(INTDIR)" ".\config.h"\
 ".\expat.h"


!ELSEIF  "$(CFG)" == "xml - Win32 Debug"

DEP_CPP_XMLPA=\
	".\xmlrole.h"\
	".\xmltok.h"\
	
NODEP_CPP_XMLPA=\
	".\config.h"\
	".\expat.h"\
	

"$(INTDIR)\xmlparse.obj" : $(SOURCE) $(DEP_CPP_XMLPA) "$(INTDIR)" ".\config.h"\
 ".\expat.h"


!ENDIF 

SOURCE=.\xmlrole.c

!IF  "$(CFG)" == "xml - Win32 Release"

DEP_CPP_XMLRO=\
	".\ascii.h"\
	".\config.h"\
	".\xmlrole.h"\
	".\xmltok.h"\
	

"$(INTDIR)\xmlrole.obj" : $(SOURCE) $(DEP_CPP_XMLRO) "$(INTDIR)" ".\config.h"


!ELSEIF  "$(CFG)" == "xml - Win32 Debug"

DEP_CPP_XMLRO=\
	".\ascii.h"\
	".\xmlrole.h"\
	".\xmltok.h"\
	
NODEP_CPP_XMLRO=\
	".\config.h"\
	

"$(INTDIR)\xmlrole.obj" : $(SOURCE) $(DEP_CPP_XMLRO) "$(INTDIR)" ".\config.h"


!ENDIF 

SOURCE=.\xmltok.c

!IF  "$(CFG)" == "xml - Win32 Release"

DEP_CPP_XMLTO=\
	".\ascii.h"\
	".\asciitab.h"\
	".\config.h"\
	".\iasciitab.h"\
	".\latin1tab.h"\
	".\nametab.h"\
	".\utf8tab.h"\
	".\xmltok.h"\
	".\xmltok_impl.c"\
	".\xmltok_impl.h"\
	".\xmltok_ns.c"\
	

"$(INTDIR)\xmltok.obj" : $(SOURCE) $(DEP_CPP_XMLTO) "$(INTDIR)" ".\config.h"


!ELSEIF  "$(CFG)" == "xml - Win32 Debug"

DEP_CPP_XMLTO=\
	".\ascii.h"\
	".\asciitab.h"\
	".\iasciitab.h"\
	".\latin1tab.h"\
	".\nametab.h"\
	".\utf8tab.h"\
	".\xmltok.h"\
	".\xmltok_impl.c"\
	".\xmltok_impl.h"\
	".\xmltok_ns.c"\
	
NODEP_CPP_XMLTO=\
	".\config.h"\
	

"$(INTDIR)\xmltok.obj" : $(SOURCE) $(DEP_CPP_XMLTO) "$(INTDIR)" ".\config.h"


!ENDIF 

SOURCE=xmltok_impl.c
SOURCE=xmltok_ns.c
SOURCE=.\expat.h.in

!IF  "$(CFG)" == "xml - Win32 Release"

InputPath=.\expat.h.in

".\expat.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\expat.h.in .\expat.h > nul 
	echo Created expat.h from expat.h.in 
	

!ELSEIF  "$(CFG)" == "xml - Win32 Debug"

InputPath=.\expat.h.in

".\expat.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\expat.h.in .\expat.h > nul 
	echo Created expat.h from expat.h.in 
	

!ENDIF 

SOURCE=.\winconfig.h

!IF  "$(CFG)" == "xml - Win32 Release"

InputPath=.\winconfig.h

".\config.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\winconfig.h .\config.h > nul 
	echo Created config.h from winconfig.h 
	

!ELSEIF  "$(CFG)" == "xml - Win32 Debug"

InputPath=.\winconfig.h

".\config.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\winconfig.h .\config.h > nul 
	echo Created config.h from winconfig.h 
	

!ENDIF 


!ENDIF 

