# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Read README.cmake before using this.

# CMAKE_MINIMUM_REQUIRED should be the first directive in the file:
# https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(APR-Util C)

INCLUDE(CMakeDependentOption)

# Enable support for MSVC runtime library selection by abstraction
# if supported by CMake.
IF(POLICY CMP0091)
  CMAKE_POLICY(SET CMP0091 NEW)
ENDIF()

FIND_PACKAGE(APR REQUIRED)

OPTION(APU_USE_EXPAT      "Use Expat"                    ON)
OPTION(APU_USE_LIBXML2    "Use LibXml2"                  OFF)

OPTION(BUILD_SHARED_LIBS    "Build using shared libraries"              ON)
CMAKE_DEPENDENT_OPTION(APU_DSO_BUILD "Use DSO build of modular components" ON "BUILD_SHARED_LIBS" OFF)
OPTION(APU_HAVE_CRYPTO      "Crypto support"                            OFF)
option(APU_HAVE_ICONV       "Xlate support"                             OFF)
OPTION(APU_HAVE_ODBC        "Build ODBC DBD driver"                     ON)
OPTION(APU_HAVE_SQLITE3     "Build SQLite3 DBD driver"                  OFF)
OPTION(APU_HAVE_PGSQL       "Build PostgreSQL DBD driver"               OFF)
OPTION(APR_HAS_LDAP         "LDAP support"                              OFF)
OPTION(INSTALL_PDB          "Install .pdb files (if generated)"         ON)
OPTION(APU_BUILD_TEST       "Build the test suite"                      OFF)

SET(APU_INSTALL_BIN_DIR      "bin"                       CACHE STRING "Install subdirectory for binary files")
SET(APU_INSTALL_LIB_DIR      "lib"                       CACHE STRING "Install subdirectory for library files")
SET(APU_INSTALL_INCLUDE_DIR  "include"                   CACHE STRING "Install subdirectory for include files")

IF(NOT APU_USE_EXPAT AND NOT APU_USE_LIBXML2)
  MESSAGE(FATAL_ERROR "Either Expat or LibXml2 must be selected")
ENDIF()
IF(APU_USE_EXPAT AND APU_USE_LIBXML2)
  MESSAGE(FATAL_ERROR "Only one of Expat and LibXml2 can be selected")
ENDIF()

IF(APU_DSO_BUILD AND NOT BUILD_SHARED_LIBS)
  MESSAGE(FATAL_ERROR "APU_DSO_BUILD requires APU_BUILD_SHARED.")
ENDIF()

IF(APU_USE_EXPAT)
  FIND_PACKAGE(expat CONFIG REQUIRED)
ENDIF()

IF(APU_USE_LIBXML2)
  FIND_PACKAGE(LibXml2 REQUIRED)
ENDIF()

IF(APU_HAVE_CRYPTO)
  FIND_PACKAGE(OpenSSL REQUIRED)
  SET(APU_HAVE_OPENSSL ON)
  SET(APU_HAVE_CRYPTO_PRNG ON)
ENDIF()

IF(APU_HAVE_ICONV)
  FIND_PACKAGE(Iconv REQUIRED)
ENDIF()

IF(APU_HAVE_SQLITE3)
  FIND_PACKAGE(SQLite3 REQUIRED)
endif()

IF(APU_HAVE_PGSQL)
  FIND_PACKAGE(PostgreSQL REQUIRED)
ENDIF()

# create 1-or-0 representation of feature tests for apu.h

SET(apu_have_apr_iconv_10 0) # not yet implemented

SET(LDAP_LIBRARIES)
IF(APR_HAS_LDAP)
  SET(LDAP_LIBRARIES wldap32)
ENDIF()

CONFIGURE_FILE(include/apu.hwc
              ${PROJECT_BINARY_DIR}/apu.h)
CONFIGURE_FILE(include/apr_ldap.hwc
              ${PROJECT_BINARY_DIR}/apr_ldap.h)
CONFIGURE_FILE(include/private/apu_config.hwc
               ${PROJECT_BINARY_DIR}/apu_config.h)
CONFIGURE_FILE(include/private/apu_select_dbm.hw
               ${PROJECT_BINARY_DIR}/apu_select_dbm.h
               COPYONLY)
CONFIGURE_FILE(include/apu_want.hw
               ${PROJECT_BINARY_DIR}/apu_want.h
               COPYONLY)

IF(APU_USE_EXPAT)
  SET(XMLLIB_LIBRARIES   expat::expat)
ELSE()
  SET(XMLLIB_LIBRARIES   LibXml2::LibXml2)
ENDIF()

IF(APU_HAVE_ICONV)
  SET(XLATE_LIBRARIES    Iconv::Iconv)
  ADD_COMPILE_DEFINITIONS(HAVE_ICONV_H)
ELSE()
  SET(XLATE_LIBRARIES   "")
ENDIF()

# Generated .h files are stored in PROJECT_BINARY_DIR, not the
# source tree.
#
# BROKEN: not searching PROJECT_BINARY_DIR first, so you have to
#         manually delete apu.h in PROJECT_SOURCE_DIR/include if
#         you've generated apu.h before using a different build

SET(APR_INCLUDE_DIRECTORIES
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/private
)

INCLUDE_DIRECTORIES(${APR_INCLUDE_DIRECTORIES})

SET(APR_PUBLIC_HEADERS_STATIC
  include/apr_anylock.h
  include/apr_base64.h
  include/apr_buckets.h
  include/apr_buffer.h
  include/apr_crypto.h
  include/apr_date.h
  include/apr_dbd.h
  include/apr_dbm.h
  include/apr_hooks.h
  include/apr_jose.h
  include/apr_json.h
  include/apr_ldap_init.h
  include/apr_ldap_option.h
  include/apr_ldap_rebind.h
  include/apr_ldap_url.h
  include/apr_md4.h
  include/apr_md5.h
  include/apr_memcache.h
  include/apr_optional.h
  include/apr_optional_hooks.h
  include/apr_queue.h
  include/apr_redis.h
  include/apr_reslist.h
  include/apr_rmm.h
  include/apr_sdbm.h
  include/apr_sha1.h
  include/apr_siphash.h
  include/apr_strmatch.h
  include/apr_thread_pool.h
  include/apr_uri.h
  include/apr_uuid.h
  include/apr_xlate.h
  include/apr_xml.h
  include/apu_errno.h
  include/apu_version.h
)

# apu_config.h and apu_select_dbm.h are private
SET(APR_PUBLIC_HEADERS_GENERATED
  ${PROJECT_BINARY_DIR}/apu.h
  ${PROJECT_BINARY_DIR}/apr_ldap.h
  ${PROJECT_BINARY_DIR}/apu_want.h
)

SET(APR_SOURCES
  buckets/apr_brigade.c
  buckets/apr_buckets.c
  buckets/apr_buckets_alloc.c
  buckets/apr_buckets_eos.c
  buckets/apr_buckets_file.c
  buckets/apr_buckets_flush.c
  buckets/apr_buckets_heap.c
  buckets/apr_buckets_mmap.c
  buckets/apr_buckets_pipe.c
  buckets/apr_buckets_pool.c
  buckets/apr_buckets_refcount.c
  buckets/apr_buckets_simple.c
  buckets/apr_buckets_socket.c
  buffer/apr_buffer.c
  crypto/apr_crypto.c
  crypto/apr_crypto_prng.c
  crypto/apr_md4.c
  crypto/apr_md5.c
  crypto/apr_passwd.c
  crypto/apr_sha1.c
  crypto/apr_siphash.c
  crypto/crypt_blowfish.c
  crypto/getuuid.c
  crypto/uuid.c
  dbd/apr_dbd.c
  dbm/apr_dbm.c
  dbm/apr_dbm_sdbm.c
  dbm/sdbm/sdbm.c
  dbm/sdbm/sdbm_hash.c
  dbm/sdbm/sdbm_lock.c
  dbm/sdbm/sdbm_pair.c
  encoding/apr_base64.c
  hooks/apr_hooks.c
  jose/apr_jose.c
  jose/apr_jose_decode.c
  jose/apr_jose_encode.c
  json/apr_json.c
  json/apr_json_decode.c
  json/apr_json_encode.c
  memcache/apr_memcache.c
  misc/apr_date.c
  misc/apr_error.c
  misc/apr_queue.c
  misc/apr_reslist.c
  misc/apr_rmm.c
  misc/apr_thread_pool.c
  misc/apu_dso.c
  misc/apu_version.c
  redis/apr_redis.c
  strmatch/apr_strmatch.c
  uri/apr_uri.c
  xlate/xlate.c
  xml/apr_xml.c
  xml/apr_xml_expat.c
  xml/apr_xml_libxml2.c
)

IF(APR_HAS_LDAP)
  SET(APR_SOURCES ${APR_SOURCES} ldap/apr_ldap_stub.c ldap/apr_ldap_url.c)
ENDIF()

# Sources and libraries for APU builtin drivers. Used when APU_DSO_BUILD=OFF.
SET(APU_EXTRA_SOURCES)
SET(APU_EXTRA_LIBRARIES)
SET(APU_EXTRA_INCLUDE_DIRECTORIES)

SET(APR_TEST_SUITES
  testbuckets
  testbuffer
  testcrypto
  testdate
  testdbd
  testdbm
  testjson
  testjose
  testldap
  testmd4
  testmd5
  testmemcache
  testpass
  testqueue
  testredis
  testreslist
  testrmm
  testsiphash
  teststrmatch
  testuri
  testuuid
  testxlate
  testxml
)

SET(install_targets)
SET(install_modules)
SET(install_bin_pdb)
SET(dbd_drivers)

IF (MSVC)
  # Ignore Microsoft's interpretation of secure development
  # and the POSIX string handling API
  ADD_COMPILE_DEFINITIONS(_CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_WARNINGS)
ENDIF()

MACRO(ADD_APU_MODULE name dllname sources libraries)
  IF(APU_DSO_BUILD)
    ADD_LIBRARY(${name} MODULE ${sources})
    LIST(APPEND install_modules ${name})
    LIST(APPEND install_bin_pdb $<TARGET_PDB_FILE:${name}>)

    TARGET_SOURCES(${name} PRIVATE libaprutil.rc)
    TARGET_COMPILE_DEFINITIONS(${name}
                               PRIVATE DLL_NAME=${dllname} APU_DSO_MODULE_BUILD)

    TARGET_LINK_LIBRARIES(${name}
                          PRIVATE libaprutil-1 apr::libapr-1)
    TARGET_LINK_LIBRARIES(${name}
                          PRIVATE ${libraries})
  ELSE()
    LIST(APPEND APU_EXTRA_SOURCES ${sources})
    LIST(APPEND APU_EXTRA_LIBRARIES ${libraries})
  ENDIF()
ENDMACRO()

IF(APU_HAVE_CRYPTO)
  IF(NOT OPENSSL_FOUND)
    MESSAGE(FATAL_ERROR "Only OpenSSL-based crypto is currently implemented in the cmake build")
  ENDIF()

  ADD_APU_MODULE(
    apr_crypto_openssl-1 "apr_crypto_openssl"
    "crypto/apr_crypto_openssl.c"
    "OpenSSL::Crypto")
ENDIF()

IF(APR_HAS_LDAP)
  ADD_APU_MODULE(
    apr_ldap-1 "apr_ldap"
    "ldap/apr_ldap_init.c;ldap/apr_ldap_option.c;ldap/apr_ldap_rebind.c;ldap/apr_ldap.c"
    ${LDAP_LIBRARIES}
  )
ENDIF()

IF(APU_HAVE_ODBC)
  LIST(APPEND dbd_drivers odbc)

  ADD_APU_MODULE(apr_dbd_odbc-1 "apr_dbd_odbc"
                 "dbd/apr_dbd_odbc.c"
                 "odbc32;odbccp32")
ENDIF()

IF(APU_HAVE_SQLITE3)
  LIST(APPEND dbd_drivers sqlite3)

  ADD_APU_MODULE(apr_dbd_sqlite3-1 "apr_dbd_sqlite3-1"
                 "dbd/apr_dbd_sqlite3.c"
                 "SQLite::SQLite3")
ENDIF()

IF(APU_HAVE_PGSQL)
  LIST(APPEND dbd_drivers pgsql)

  ADD_APU_MODULE(apr_dbd_pgsql-2 "apr_dbd_pgsql"
                 "dbd/apr_dbd_pgsql.c"
                 "PostgreSQL::PostgreSQL")

  ADD_COMPILE_DEFINITIONS(HAVE_LIBPQ_FE_H)
ENDIF()

ADD_LIBRARY(libaprutil-1 ${APR_SOURCES} ${APU_EXTRA_SOURCES} ${APR_PUBLIC_HEADERS_GENERATED})
LIST(APPEND install_targets libaprutil-1)
TARGET_LINK_LIBRARIES(libaprutil-1
                      PRIVATE ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APU_EXTRA_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(libaprutil-1
                           PRIVATE ${APU_EXTRA_INCLUDE_DIRECTORIES})

IF(BUILD_SHARED_LIBS)
  LIST(APPEND install_bin_pdb $<TARGET_PDB_FILE:libaprutil-1>)
  TARGET_SOURCES(libaprutil-1 PRIVATE libaprutil.rc)
  TARGET_LINK_LIBRARIES(libaprutil-1
                        PRIVATE apr::libapr-1)
  TARGET_COMPILE_DEFINITIONS(libaprutil-1
                             PRIVATE "APU_DECLARE_EXPORT"
                             INTERFACE "APU_DECLARE_IMPORT")

  # Copy libapr-1.dll to BuildDir for test run.
  ADD_CUSTOM_COMMAND(
      TARGET libaprutil-1 POST_BUILD
      COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_RUNTIME_DLLS:libaprutil-1>
            $<TARGET_FILE_DIR:libaprutil-1>
      COMMAND_EXPAND_LISTS
  )
ELSE()
  SET_TARGET_PROPERTIES(libaprutil-1 PROPERTIES
                        OUTPUT_NAME aprutil-1)
  TARGET_LINK_LIBRARIES(libaprutil-1
                        PRIVATE apr::apr-1)
  TARGET_COMPILE_DEFINITIONS(libaprutil-1
                             PRIVATE "APU_DECLARE_STATIC"
                             INTERFACE "APU_DECLARE_STATIC")
ENDIF()

IF(APU_BUILD_TEST)
  ENABLE_TESTING()
  # Create a "check" target that displays test program output to the console.
  ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

  # copy data files to build directory so that we can run programs from there
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory 
                  ${PROJECT_BINARY_DIR}/data)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                  ${PROJECT_SOURCE_DIR}/test/data/billion-laughs.xml
                  ${PROJECT_BINARY_DIR}/data/billion-laughs.xml)

  IF(BUILD_SHARED_LIBS)
    SET(whichapr    "apr::libapr-1;libaprutil-1")
  ELSE()
    SET(whichapr    "apr::apr-1;libaprutil-1")
  ENDIF()

  SET (APR_TEST_SOURCES
    test/abts.c
    test/testutil.c
    )

  FOREACH(testsuite ${APR_TEST_SUITES})
    LIST(APPEND APR_TEST_SOURCES test/${testsuite}.c)
  ENDFOREACH()

  ADD_EXECUTABLE(testall ${APR_TEST_SOURCES})
  TARGET_LINK_LIBRARIES(testall ${whichapr} ${LDAP_LIBRARIES})
  TARGET_COMPILE_DEFINITIONS(testall PRIVATE "BINPATH=$<TARGET_FILE_DIR:testall>")

  FOREACH(test ${APR_TEST_SUITES})
    ADD_TEST(NAME ${test} COMMAND testall -v ${test})
  ENDFOREACH()

  ADD_EXECUTABLE(dbd test/dbd.c)
  TARGET_LINK_LIBRARIES(dbd ${whichapr})

  # dbd is run multiple times with different parameters.
  FOREACH(somedbd ${dbd_drivers})
    ADD_TEST(NAME dbd-${somedbd} COMMAND dbd ${somedbd})
  ENDFOREACH()

  ADD_EXECUTABLE(memcachedmock test/memcachedmock.c)
  TARGET_LINK_LIBRARIES(memcachedmock ${whichapr})

  ADD_DEPENDENCIES(testall memcachedmock)
ENDIF (APU_BUILD_TEST)

# Installation

INSTALL(TARGETS ${install_targets}
        RUNTIME DESTINATION ${APU_INSTALL_BIN_DIR}
        LIBRARY DESTINATION ${APU_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${APU_INSTALL_LIB_DIR}
       )

# For DSO modules, install only binaries
INSTALL(TARGETS ${install_modules}
        DESTINATION ${APU_INSTALL_BIN_DIR})

IF(INSTALL_PDB)
  INSTALL(FILES ${install_bin_pdb}
          DESTINATION ${APU_INSTALL_BIN_DIR}
          CONFIGURATIONS RelWithDebInfo Debug)
ENDIF()

INSTALL(FILES ${APR_PUBLIC_HEADERS_STATIC} ${APR_PUBLIC_HEADERS_GENERATED}
        DESTINATION ${APU_INSTALL_INCLUDE_DIR})

STRING(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
MESSAGE(STATUS "")
MESSAGE(STATUS "")
MESSAGE(STATUS "APR-Util configuration summary:")
MESSAGE(STATUS "")
MESSAGE(STATUS "  Build type ...................... : ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "  Build shared libs ............... : ${BUILD_SHARED_LIBS}")
MESSAGE(STATUS "  Install .pdb (if available)...... : ${INSTALL_PDB}")
MESSAGE(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "    Directory for binary files .... : PREFIX/${APU_INSTALL_BIN_DIR}")
MESSAGE(STATUS "    Directory for library files ... : PREFIX/${APU_INSTALL_LIB_DIR}")
MESSAGE(STATUS "    Directory for include files ... : PREFIX/${APU_INSTALL_INCLUDE_DIR}")
MESSAGE(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "  DBD ODBC driver ................. : ${APU_HAVE_ODBC}")
MESSAGE(STATUS "  DBD SQLite3 driver .............. : ${APU_HAVE_SQLITE3}")
MESSAGE(STATUS "  DBD PostgreSQL .................. : ${APU_HAVE_PGSQL}")
MESSAGE(STATUS "  APU_HAVE_CRYPTO ................. : ${APU_HAVE_CRYPTO}")
MESSAGE(STATUS "  APU_HAVE_ICONV .................. : ${APU_HAVE_ICONV}")
MESSAGE(STATUS "  APR_HAS_LDAP .................... : ${APR_HAS_LDAP}")
MESSAGE(STATUS "  Use Expat ....................... : ${APU_USE_EXPAT}")
MESSAGE(STATUS "  Use LibXml2 ..................... : ${APU_USE_LIBXML2}")
MESSAGE(STATUS "  Build test suite ................ : ${APU_BUILD_TEST}")

