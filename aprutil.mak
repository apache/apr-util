# Microsoft Developer Studio Generated NMAKE File, Based on aprutil.dsp
!IF "$(CFG)" == ""
CFG=aprutil - Win32 Debug
!MESSAGE No configuration specified. Defaulting to aprutil - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "aprutil - Win32 Release" && "$(CFG)" !=\
 "aprutil - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "aprutil - Win32 Release"

OUTDIR=.\LibR
INTDIR=.\LibR
# Begin Custom Macros
OutDir=.\LibR
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\aprutil.lib"

!ELSE 

ALL : "xml - Win32 Release" "gen_uri_delims - Win32 Release"\
 "apr - Win32 Release" "$(OUTDIR)\aprutil.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 ReleaseCLEAN" "gen_uri_delims - Win32 ReleaseCLEAN"\
 "xml - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_base64.obj"
	-@erase "$(INTDIR)\apr_brigade.obj"
	-@erase "$(INTDIR)\apr_buckets.obj"
	-@erase "$(INTDIR)\apr_buckets_eos.obj"
	-@erase "$(INTDIR)\apr_buckets_file.obj"
	-@erase "$(INTDIR)\apr_buckets_flush.obj"
	-@erase "$(INTDIR)\apr_buckets_heap.obj"
	-@erase "$(INTDIR)\apr_buckets_mmap.obj"
	-@erase "$(INTDIR)\apr_buckets_pipe.obj"
	-@erase "$(INTDIR)\apr_buckets_pool.obj"
	-@erase "$(INTDIR)\apr_buckets_refcount.obj"
	-@erase "$(INTDIR)\apr_buckets_simple.obj"
	-@erase "$(INTDIR)\apr_buckets_socket.obj"
	-@erase "$(INTDIR)\apr_dbm.obj"
	-@erase "$(INTDIR)\apr_hooks.obj"
	-@erase "$(INTDIR)\apr_sha1.obj"
	-@erase "$(INTDIR)\apr_uri.obj"
	-@erase "$(INTDIR)\apr_xml.obj"
	-@erase "$(INTDIR)\aprutil.idb"
	-@erase "$(INTDIR)\sdbm.obj"
	-@erase "$(INTDIR)\sdbm_hash.obj"
	-@erase "$(INTDIR)\sdbm_lock.obj"
	-@erase "$(INTDIR)\sdbm_pair.obj"
	-@erase "$(OUTDIR)\aprutil.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

RSC=rc.exe
CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /I "./include" /I "../apr/include" /I\
 "./include/private" /I "./dbm/sdbm" /I "./xml/expat/lib" /D "NDEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /D "APU_USE_SDBM"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\aprutil" /FD /c 
CPP_OBJS=.\LibR/
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

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aprutil.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\aprutil.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_base64.obj" \
	"$(INTDIR)\apr_brigade.obj" \
	"$(INTDIR)\apr_buckets.obj" \
	"$(INTDIR)\apr_buckets_eos.obj" \
	"$(INTDIR)\apr_buckets_file.obj" \
	"$(INTDIR)\apr_buckets_flush.obj" \
	"$(INTDIR)\apr_buckets_heap.obj" \
	"$(INTDIR)\apr_buckets_mmap.obj" \
	"$(INTDIR)\apr_buckets_pipe.obj" \
	"$(INTDIR)\apr_buckets_pool.obj" \
	"$(INTDIR)\apr_buckets_refcount.obj" \
	"$(INTDIR)\apr_buckets_simple.obj" \
	"$(INTDIR)\apr_buckets_socket.obj" \
	"$(INTDIR)\apr_dbm.obj" \
	"$(INTDIR)\apr_hooks.obj" \
	"$(INTDIR)\apr_sha1.obj" \
	"$(INTDIR)\apr_uri.obj" \
	"$(INTDIR)\apr_xml.obj" \
	"$(INTDIR)\sdbm.obj" \
	"$(INTDIR)\sdbm_hash.obj" \
	"$(INTDIR)\sdbm_lock.obj" \
	"$(INTDIR)\sdbm_pair.obj" \
	"..\apr\LibR\apr.lib" \
	".\xml\expat\lib\LibR\xml.lib"

"$(OUTDIR)\aprutil.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

OUTDIR=.\LibD
INTDIR=.\LibD
# Begin Custom Macros
OutDir=.\LibD
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\aprutil.lib"

!ELSE 

ALL : "xml - Win32 Debug" "gen_uri_delims - Win32 Debug" "apr - Win32 Debug"\
 "$(OUTDIR)\aprutil.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"apr - Win32 DebugCLEAN" "gen_uri_delims - Win32 DebugCLEAN"\
 "xml - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\apr_base64.obj"
	-@erase "$(INTDIR)\apr_brigade.obj"
	-@erase "$(INTDIR)\apr_buckets.obj"
	-@erase "$(INTDIR)\apr_buckets_eos.obj"
	-@erase "$(INTDIR)\apr_buckets_file.obj"
	-@erase "$(INTDIR)\apr_buckets_flush.obj"
	-@erase "$(INTDIR)\apr_buckets_heap.obj"
	-@erase "$(INTDIR)\apr_buckets_mmap.obj"
	-@erase "$(INTDIR)\apr_buckets_pipe.obj"
	-@erase "$(INTDIR)\apr_buckets_pool.obj"
	-@erase "$(INTDIR)\apr_buckets_refcount.obj"
	-@erase "$(INTDIR)\apr_buckets_simple.obj"
	-@erase "$(INTDIR)\apr_buckets_socket.obj"
	-@erase "$(INTDIR)\apr_dbm.obj"
	-@erase "$(INTDIR)\apr_hooks.obj"
	-@erase "$(INTDIR)\apr_sha1.obj"
	-@erase "$(INTDIR)\apr_uri.obj"
	-@erase "$(INTDIR)\apr_xml.obj"
	-@erase "$(INTDIR)\aprutil.idb"
	-@erase "$(INTDIR)\aprutil.pdb"
	-@erase "$(INTDIR)\sdbm.obj"
	-@erase "$(INTDIR)\sdbm_hash.obj"
	-@erase "$(INTDIR)\sdbm_lock.obj"
	-@erase "$(INTDIR)\sdbm_pair.obj"
	-@erase "$(OUTDIR)\aprutil.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

RSC=rc.exe
CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /I "./include" /I "../apr/include" /I\
 "./include/private" /I "./dbm/sdbm" /I "./xml/expat/lib" /D "_DEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /D "APU_USE_SDBM"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\aprutil" /FD /c 
CPP_OBJS=.\LibD/
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

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aprutil.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\aprutil.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apr_base64.obj" \
	"$(INTDIR)\apr_brigade.obj" \
	"$(INTDIR)\apr_buckets.obj" \
	"$(INTDIR)\apr_buckets_eos.obj" \
	"$(INTDIR)\apr_buckets_file.obj" \
	"$(INTDIR)\apr_buckets_flush.obj" \
	"$(INTDIR)\apr_buckets_heap.obj" \
	"$(INTDIR)\apr_buckets_mmap.obj" \
	"$(INTDIR)\apr_buckets_pipe.obj" \
	"$(INTDIR)\apr_buckets_pool.obj" \
	"$(INTDIR)\apr_buckets_refcount.obj" \
	"$(INTDIR)\apr_buckets_simple.obj" \
	"$(INTDIR)\apr_buckets_socket.obj" \
	"$(INTDIR)\apr_dbm.obj" \
	"$(INTDIR)\apr_hooks.obj" \
	"$(INTDIR)\apr_sha1.obj" \
	"$(INTDIR)\apr_uri.obj" \
	"$(INTDIR)\apr_xml.obj" \
	"$(INTDIR)\sdbm.obj" \
	"$(INTDIR)\sdbm_hash.obj" \
	"$(INTDIR)\sdbm_lock.obj" \
	"$(INTDIR)\sdbm_pair.obj" \
	"..\apr\LibD\apr.lib" \
	".\xml\expat\lib\LibD\xml.lib"

"$(OUTDIR)\aprutil.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "aprutil - Win32 Release" || "$(CFG)" ==\
 "aprutil - Win32 Debug"
SOURCE=.\buckets\apr_brigade.c
DEP_CPP_APR_B=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_tables.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_brigade.obj" : $(SOURCE) $(DEP_CPP_APR_B) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets.c
DEP_CPP_APR_BU=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets.obj" : $(SOURCE) $(DEP_CPP_APR_BU) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_eos.c
DEP_CPP_APR_BUC=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_eos.obj" : $(SOURCE) $(DEP_CPP_APR_BUC) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_file.c
DEP_CPP_APR_BUCK=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_file.obj" : $(SOURCE) $(DEP_CPP_APR_BUCK) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_flush.c
DEP_CPP_APR_BUCKE=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_flush.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKE) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_heap.c
DEP_CPP_APR_BUCKET=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_heap.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKET) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_mmap.c
DEP_CPP_APR_BUCKETS=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_mmap.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKETS) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_pipe.c
DEP_CPP_APR_BUCKETS_=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_pipe.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKETS_)\
 "$(INTDIR)" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_pool.c
DEP_CPP_APR_BUCKETS_P=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_pool.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKETS_P)\
 "$(INTDIR)" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_refcount.c
DEP_CPP_APR_BUCKETS_R=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_refcount.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKETS_R)\
 "$(INTDIR)" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_simple.c
DEP_CPP_APR_BUCKETS_S=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_simple.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKETS_S)\
 "$(INTDIR)" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\buckets\apr_buckets_socket.c
DEP_CPP_APR_BUCKETS_SO=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_buckets.h"\
	".\include\apr_ring.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_buckets_socket.obj" : $(SOURCE) $(DEP_CPP_APR_BUCKETS_SO)\
 "$(INTDIR)" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\crypto\apr_sha1.c
DEP_CPP_APR_S=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_lib.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_xlate.h"\
	".\include\apr_base64.h"\
	".\include\apr_sha1.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_sha1.obj" : $(SOURCE) $(DEP_CPP_APR_S) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\dbm\sdbm\sdbm.c
DEP_CPP_SDBM_=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\dbm\sdbm\sdbm_pair.h"\
	".\dbm\sdbm\sdbm_private.h"\
	".\dbm\sdbm\sdbm_tune.h"\
	".\include\apr_sdbm.h"\
	".\include\apu.h"\
	

"$(INTDIR)\sdbm.obj" : $(SOURCE) $(DEP_CPP_SDBM_) "$(INTDIR)" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\dbm\sdbm\sdbm_hash.c
DEP_CPP_SDBM_H=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\dbm\sdbm\sdbm_private.h"\
	".\include\apr_sdbm.h"\
	".\include\apu.h"\
	

"$(INTDIR)\sdbm_hash.obj" : $(SOURCE) $(DEP_CPP_SDBM_H) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\dbm\sdbm\sdbm_lock.c
DEP_CPP_SDBM_L=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\dbm\sdbm\sdbm_private.h"\
	".\dbm\sdbm\sdbm_tune.h"\
	".\include\apr_sdbm.h"\
	".\include\apu.h"\
	

"$(INTDIR)\sdbm_lock.obj" : $(SOURCE) $(DEP_CPP_SDBM_L) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\dbm\sdbm\sdbm_pair.c
DEP_CPP_SDBM_P=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\dbm\sdbm\sdbm_pair.h"\
	".\dbm\sdbm\sdbm_private.h"\
	".\dbm\sdbm\sdbm_tune.h"\
	".\include\apr_sdbm.h"\
	".\include\apu.h"\
	

"$(INTDIR)\sdbm_pair.obj" : $(SOURCE) $(DEP_CPP_SDBM_P) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\dbm\apr_dbm.c
DEP_CPP_APR_D=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_dbm.h"\
	".\include\apr_sdbm.h"\
	".\include\apu.h"\
	".\include\private\apu_select_dbm.h"\
	

"$(INTDIR)\apr_dbm.obj" : $(SOURCE) $(DEP_CPP_APR_D) "$(INTDIR)"\
 ".\include\private\apu_select_dbm.h" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\encoding\apr_base64.c
DEP_CPP_APR_BA=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_xlate.h"\
	".\include\apr_base64.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_base64.obj" : $(SOURCE) $(DEP_CPP_APR_BA) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\hooks\apr_hooks.c
DEP_CPP_APR_H=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_hash.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_tables.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_hooks.h"\
	".\include\apr_optional.h"\
	".\include\apr_optional_hooks.h"\
	".\include\apu.h"\
	

"$(INTDIR)\apr_hooks.obj" : $(SOURCE) $(DEP_CPP_APR_H) "$(INTDIR)"\
 ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\uri\apr_uri.c
DEP_CPP_APR_U=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_info.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\include\apr_user.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_uri.h"\
	".\include\apu.h"\
	".\uri\uri_delims.h"\
	

"$(INTDIR)\apr_uri.obj" : $(SOURCE) $(DEP_CPP_APR_U) "$(INTDIR)"\
 ".\include\apu.h" ".\uri\uri_delims.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\xml\apr_xml.c
DEP_CPP_APR_X=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_tables.h"\
	"..\apr\include\apr_want.h"\
	".\include\apr_xml.h"\
	".\include\apu.h"\
	".\include\private\apu_config.h"\
	".\xml\expat\lib\expat.h"\
	
NODEP_CPP_APR_X=\
	".\xml\xmlparse.h"\
	

"$(INTDIR)\apr_xml.obj" : $(SOURCE) $(DEP_CPP_APR_X) "$(INTDIR)"\
 ".\include\private\apu_config.h" ".\include\apu.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\include\apu.hw

!IF  "$(CFG)" == "aprutil - Win32 Release"

InputPath=.\include\apu.hw

".\include\apu.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apu.hw .\include\apu.h > nul 
	echo Created apu.h from apu.hw 
	

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

InputPath=.\include\apu.hw

".\include\apu.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\apu.hw .\include\apu.h > nul 
	echo Created apu.h from apu.hw 
	

!ENDIF 

SOURCE=.\include\private\apu_config.hw

!IF  "$(CFG)" == "aprutil - Win32 Release"

InputPath=.\include\private\apu_config.hw

".\include\private\apu_config.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\private\apu_config.hw .\include\private\apu_config.h > nul 
	echo Created apu_config.h from apu_config.hw 
	

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

InputPath=.\include\private\apu_config.hw

".\include\private\apu_config.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\private\apu_config.hw .\include\private\apu_config.h > nul 
	echo Created apu_config.h from apu_config.hw 
	

!ENDIF 

SOURCE=.\include\private\apu_select_dbm.hw

!IF  "$(CFG)" == "aprutil - Win32 Release"

InputPath=.\include\private\apu_select_dbm.hw

".\include\private\apu_select_dbm.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\private\apu_select_dbm.hw .\include\private\apu_select_dbm.h >\
 nul 
	echo Created apu_select_dbm.h from apu_select_dbm.hw 
	

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

InputPath=.\include\private\apu_select_dbm.hw

".\include\private\apu_select_dbm.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\private\apu_select_dbm.hw .\include\private\apu_select_dbm.h >\
 nul 
	echo Created apu_select_dbm.h from apu_select_dbm.hw 
	

!ENDIF 

SOURCE=.\uri\gen_uri_delims.exe

!IF  "$(CFG)" == "aprutil - Win32 Release"

InputPath=.\uri\gen_uri_delims.exe

".\uri\uri_delims.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	.\uri\gen_uri_delims.exe >.\uri\uri_delims.h 
	echo Generated uri_delims.h from gen_uri_delims.exe 
	

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

InputPath=.\uri\gen_uri_delims.exe

".\uri\uri_delims.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	.\uri\gen_uri_delims.exe >.\uri\uri_delims.h 
	echo Generated uri_delims.h from gen_uri_delims.exe 
	

!ENDIF 

!IF  "$(CFG)" == "aprutil - Win32 Release"

"apr - Win32 Release" : 
   cd "..\..\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Release" 
   cd "..\apr-util"

"apr - Win32 ReleaseCLEAN" : 
   cd "..\..\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\apr.mak" CFG="apr - Win32 Release"\
 RECURSE=1 
   cd "..\apr-util"

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

"apr - Win32 Debug" : 
   cd "..\..\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) /F ".\apr.mak" CFG="apr - Win32 Debug" 
   cd "..\apr-util"

"apr - Win32 DebugCLEAN" : 
   cd "..\..\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\apr.mak" CFG="apr - Win32 Debug" RECURSE=1\
 
   cd "..\apr-util"

!ENDIF 

!IF  "$(CFG)" == "aprutil - Win32 Release"

"gen_uri_delims - Win32 Release" : 
   cd ".\uri"
   $(MAKE) /$(MAKEFLAGS) /F ".\gen_uri_delims.mak"\
 CFG="gen_uri_delims - Win32 Release" 
   cd ".."

"gen_uri_delims - Win32 ReleaseCLEAN" : 
   cd ".\uri"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\gen_uri_delims.mak"\
 CFG="gen_uri_delims - Win32 Release" RECURSE=1 
   cd ".."

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

"gen_uri_delims - Win32 Debug" : 
   cd ".\uri"
   $(MAKE) /$(MAKEFLAGS) /F ".\gen_uri_delims.mak"\
 CFG="gen_uri_delims - Win32 Debug" 
   cd ".."

"gen_uri_delims - Win32 DebugCLEAN" : 
   cd ".\uri"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\gen_uri_delims.mak"\
 CFG="gen_uri_delims - Win32 Debug" RECURSE=1 
   cd ".."

!ENDIF 

!IF  "$(CFG)" == "aprutil - Win32 Release"

"xml - Win32 Release" : 
   cd ".\xml\expat\lib"
   $(MAKE) /$(MAKEFLAGS) /F ".\xml.mak" CFG="xml - Win32 Release" 
   cd "..\..\.."

"xml - Win32 ReleaseCLEAN" : 
   cd ".\xml\expat\lib"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\xml.mak" CFG="xml - Win32 Release"\
 RECURSE=1 
   cd "..\..\.."

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

"xml - Win32 Debug" : 
   cd ".\xml\expat\lib"
   $(MAKE) /$(MAKEFLAGS) /F ".\xml.mak" CFG="xml - Win32 Debug" 
   cd "..\..\.."

"xml - Win32 DebugCLEAN" : 
   cd ".\xml\expat\lib"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\xml.mak" CFG="xml - Win32 Debug" RECURSE=1\
 
   cd "..\..\.."

!ENDIF 


!ENDIF 

