# - FindValgrind
#
# Copyright (C) 2015 Valve Corporation

find_package(PkgConfig)

pkg_check_modules(PC_VALGRIND QUIET valgrind)

find_path(VALGRIND_INCLUDE_DIR NAMES valgrind.h memcheck.h
    HINTS
    ${PC_VALGRIND_INCLUDEDIR}
    ${PC_VALGRIND_INCLUDE_DIRS}
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Valgrind DEFAULT_MSG
    VALGRIND_INCLUDE_DIR)

mark_as_advanced(VALGRIND_INCLUDE_DIR)

set(VALGRIND_INCLUDE_DIRS ${VALGRIND_INCLUDE_DIR})
set(VALGRIND_LIBRARIES "")
