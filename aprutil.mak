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

ALL : "libapr - Win32 Release" "$(OUTDIR)\aprutil.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ap_base64.obj"
	-@erase "$(INTDIR)\ap_buckets.obj"
	-@erase "$(INTDIR)\ap_buckets_eos.obj"
	-@erase "$(INTDIR)\ap_buckets_file.obj"
	-@erase "$(INTDIR)\ap_buckets_flush.obj"
	-@erase "$(INTDIR)\ap_buckets_heap.obj"
	-@erase "$(INTDIR)\ap_buckets_mmap.obj"
	-@erase "$(INTDIR)\ap_buckets_pipe.obj"
	-@erase "$(INTDIR)\ap_buckets_pool.obj"
	-@erase "$(INTDIR)\ap_buckets_refcount.obj"
	-@erase "$(INTDIR)\ap_buckets_simple.obj"
	-@erase "$(INTDIR)\ap_buckets_socket.obj"
	-@erase "$(INTDIR)\ap_hooks.obj"
	-@erase "$(INTDIR)\ap_sha1.obj"
	-@erase "$(INTDIR)\apr_dbm.obj"
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
 "./include/private" /I "./src/dbm/sdbm" /D "NDEBUG" /D "APR_DECLARE_EXPORT" /D\
 "WIN32" /D "_WINDOWS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\aprutil" /FD /c 
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
	"$(INTDIR)\ap_base64.obj" \
	"$(INTDIR)\ap_buckets.obj" \
	"$(INTDIR)\ap_buckets_eos.obj" \
	"$(INTDIR)\ap_buckets_file.obj" \
	"$(INTDIR)\ap_buckets_flush.obj" \
	"$(INTDIR)\ap_buckets_heap.obj" \
	"$(INTDIR)\ap_buckets_mmap.obj" \
	"$(INTDIR)\ap_buckets_pipe.obj" \
	"$(INTDIR)\ap_buckets_pool.obj" \
	"$(INTDIR)\ap_buckets_refcount.obj" \
	"$(INTDIR)\ap_buckets_simple.obj" \
	"$(INTDIR)\ap_buckets_socket.obj" \
	"$(INTDIR)\ap_hooks.obj" \
	"$(INTDIR)\ap_sha1.obj" \
	"$(INTDIR)\apr_dbm.obj" \
	"$(INTDIR)\sdbm.obj" \
	"$(INTDIR)\sdbm_hash.obj" \
	"$(INTDIR)\sdbm_lock.obj" \
	"$(INTDIR)\sdbm_pair.obj" \
	"..\apr\Release\libapr.lib"

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

ALL : "libapr - Win32 Debug" "$(OUTDIR)\aprutil.lib"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libapr - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ap_base64.obj"
	-@erase "$(INTDIR)\ap_buckets.obj"
	-@erase "$(INTDIR)\ap_buckets_eos.obj"
	-@erase "$(INTDIR)\ap_buckets_file.obj"
	-@erase "$(INTDIR)\ap_buckets_flush.obj"
	-@erase "$(INTDIR)\ap_buckets_heap.obj"
	-@erase "$(INTDIR)\ap_buckets_mmap.obj"
	-@erase "$(INTDIR)\ap_buckets_pipe.obj"
	-@erase "$(INTDIR)\ap_buckets_pool.obj"
	-@erase "$(INTDIR)\ap_buckets_refcount.obj"
	-@erase "$(INTDIR)\ap_buckets_simple.obj"
	-@erase "$(INTDIR)\ap_buckets_socket.obj"
	-@erase "$(INTDIR)\ap_hooks.obj"
	-@erase "$(INTDIR)\ap_sha1.obj"
	-@erase "$(INTDIR)\apr_dbm.obj"
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
 "./include/private" /I "./src/dbm/sdbm" /D "_DEBUG" /D "APR_DECLARE_EXPORT" /D\
 "WIN32" /D "_WINDOWS" /D "APU_USE_SDBM" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\aprutil"\
 /FD /c 
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
	"$(INTDIR)\ap_base64.obj" \
	"$(INTDIR)\ap_buckets.obj" \
	"$(INTDIR)\ap_buckets_eos.obj" \
	"$(INTDIR)\ap_buckets_file.obj" \
	"$(INTDIR)\ap_buckets_flush.obj" \
	"$(INTDIR)\ap_buckets_heap.obj" \
	"$(INTDIR)\ap_buckets_mmap.obj" \
	"$(INTDIR)\ap_buckets_pipe.obj" \
	"$(INTDIR)\ap_buckets_pool.obj" \
	"$(INTDIR)\ap_buckets_refcount.obj" \
	"$(INTDIR)\ap_buckets_simple.obj" \
	"$(INTDIR)\ap_buckets_socket.obj" \
	"$(INTDIR)\ap_hooks.obj" \
	"$(INTDIR)\ap_sha1.obj" \
	"$(INTDIR)\apr_dbm.obj" \
	"$(INTDIR)\sdbm.obj" \
	"$(INTDIR)\sdbm_hash.obj" \
	"$(INTDIR)\sdbm_lock.obj" \
	"$(INTDIR)\sdbm_pair.obj" \
	"..\apr\Debug\libapr.lib"

"$(OUTDIR)\aprutil.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "aprutil - Win32 Release" || "$(CFG)" ==\
 "aprutil - Win32 Debug"
SOURCE=.\include\private\apu_private.hw

!IF  "$(CFG)" == "aprutil - Win32 Release"

InputPath=.\include\private\apu_private.hw

".\include\private\apu_private.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\private\apu_private.hw .\include\private\apu_private.h > nul 
	echo Created apu_private.h from apu_private.hw 
	

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

InputPath=.\include\private\apu_private.hw

".\include\private\apu_private.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\include\private\apu_private.hw .\include\private\apu_private.h > nul 
	echo Created apu_private.h from apu_private.hw 
	

!ENDIF 

SOURCE=.\src\buckets\ap_buckets.c
DEP_CPP_AP_BU=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_lib.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_tables.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets.obj" : $(SOURCE) $(DEP_CPP_AP_BU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_eos.c
DEP_CPP_AP_BUC=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_eos.obj" : $(SOURCE) $(DEP_CPP_AP_BUC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_file.c
DEP_CPP_AP_BUCK=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_lib.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_file.obj" : $(SOURCE) $(DEP_CPP_AP_BUCK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_flush.c
DEP_CPP_AP_BUCKE=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_flush.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_heap.c
DEP_CPP_AP_BUCKET=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_heap.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKET) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_mmap.c
DEP_CPP_AP_BUCKETS=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_mmap.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKETS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_pipe.c
DEP_CPP_AP_BUCKETS_=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_lib.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_pipe.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKETS_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_pool.c
DEP_CPP_AP_BUCKETS_P=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_pool.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKETS_P) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_refcount.c
DEP_CPP_AP_BUCKETS_R=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_refcount.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKETS_R)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_simple.c
DEP_CPP_AP_BUCKETS_S=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_simple.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKETS_S)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\buckets\ap_buckets_socket.c
DEP_CPP_AP_BUCKETS_SO=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_lib.h"\
	"..\apr\include\apr_mmap.h"\
	"..\apr\include\apr_network_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_buckets.h"\
	".\include\ap_ring.h"\
	

"$(INTDIR)\ap_buckets_socket.obj" : $(SOURCE) $(DEP_CPP_AP_BUCKETS_SO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\crypto\ap_sha1.c
DEP_CPP_AP_SH=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_lib.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_xlate.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_base64.h"\
	".\include\ap_sha1.h"\
	

"$(INTDIR)\ap_sha1.obj" : $(SOURCE) $(DEP_CPP_AP_SH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\dbm\sdbm\sdbm.c
DEP_CPP_SDBM_=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_strings.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\apr_sdbm.h"\
	".\src\dbm\sdbm\sdbm_pair.h"\
	".\src\dbm\sdbm\sdbm_private.h"\
	".\src\dbm\sdbm\sdbm_tune.h"\
	

"$(INTDIR)\sdbm.obj" : $(SOURCE) $(DEP_CPP_SDBM_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\dbm\sdbm\sdbm_hash.c
DEP_CPP_SDBM_H=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\apr_sdbm.h"\
	

"$(INTDIR)\sdbm_hash.obj" : $(SOURCE) $(DEP_CPP_SDBM_H) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\dbm\sdbm\sdbm_lock.c
DEP_CPP_SDBM_L=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\apr_sdbm.h"\
	".\src\dbm\sdbm\sdbm_private.h"\
	

"$(INTDIR)\sdbm_lock.obj" : $(SOURCE) $(DEP_CPP_SDBM_L) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\dbm\sdbm\sdbm_pair.c
DEP_CPP_SDBM_P=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\apr_sdbm.h"\
	".\src\dbm\sdbm\sdbm_pair.h"\
	".\src\dbm\sdbm\sdbm_private.h"\
	".\src\dbm\sdbm\sdbm_tune.h"\
	

"$(INTDIR)\sdbm_pair.obj" : $(SOURCE) $(DEP_CPP_SDBM_P) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\dbm\apr_dbm.c
DEP_CPP_APR_D=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_file_io.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_time.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\apr_dbm.h"\
	".\include\apr_sdbm.h"\
	".\include\private\apu_private.h"\
	

"$(INTDIR)\apr_dbm.obj" : $(SOURCE) $(DEP_CPP_APR_D) "$(INTDIR)"\
 ".\include\private\apu_private.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\encoding\ap_base64.c
DEP_CPP_AP_BA=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_general.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_xlate.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_base64.h"\
	

"$(INTDIR)\ap_base64.obj" : $(SOURCE) $(DEP_CPP_AP_BA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\hooks\ap_hooks.c
DEP_CPP_AP_HO=\
	"..\apr\include\apr.h"\
	"..\apr\include\apr_errno.h"\
	"..\apr\include\apr_pools.h"\
	"..\apr\include\apr_tables.h"\
	"..\apr\network_io\os2\os2nerrno.h"\
	".\include\ap_hooks.h"\
	

"$(INTDIR)\ap_hooks.obj" : $(SOURCE) $(DEP_CPP_AP_HO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!IF  "$(CFG)" == "aprutil - Win32 Release"

"libapr - Win32 Release" : 
   cd "\test\httpd-2.0\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) /F ".\libapr.mak" CFG="libapr - Win32 Release" 
   cd "..\apr-util"

"libapr - Win32 ReleaseCLEAN" : 
   cd "\test\httpd-2.0\srclib\apr"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F ".\libapr.mak" CFG="libapr - Win32 Release"\
 RECURSE=1 
   cd "..\apr-util"

!ELSEIF  "$(CFG)" == "aprutil - Win32 Debug"

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


!ENDIF 

