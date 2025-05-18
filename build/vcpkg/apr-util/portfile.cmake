#
# Modify REF to latest commit id from https://github.com/apache/apr-util
# Update SHA512 with actual SHA512
#
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO apache/apr-util
    REF 0cf97bf278a453b976ab4305de5661560514232c
    SHA512 0
    HEAD_REF 1.7.x
)

if (VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
        FEATURES
            crypto FEATURE_CRYPTO
            xlate FEATURE_XLATE
            dbd-odbc FEATURE_DBD_ODBC
            dbd-sqlite3 FEATURE_DBD_SQLITE3
            dbd-postgresql FEATURE_DBD_PGQL
            ldap FEATURE_LDAP
    )

    vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
            -DINSTALL_PDB=OFF
            -DAPU_HAVE_CRYPTO=${FEATURE_CRYPTO}
            -DAPU_HAVE_ICONV=${FEATURE_XLATE}
            -DAPU_HAVE_ODBC=${FEATURE_DBD_ODBC}
            -DAPU_HAVE_SQLITE3=${FEATURE_DBD_SQLITE3}
            -DAPU_HAVE_PGSQL=${FEATURE_DBD_PGQL}
            -DAPR_HAS_LDAP=${FEATURE_LDAP}
            -DAPU_USE_EXPAT=ON
    )

    vcpkg_cmake_install()
    vcpkg_copy_pdbs()

    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
else()
    # In development
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
