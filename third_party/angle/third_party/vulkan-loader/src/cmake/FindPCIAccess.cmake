# - FindPCIAccess
#
# Copyright 2015 Valve Corporation

find_package(PkgConfig)

pkg_check_modules(PC_PCIACCESS QUIET pciaccess)

find_path(PCIACCESS_INCLUDE_DIR NAMES pciaccess.h
    HINTS
    ${PC_PCIACCESS_INCLUDEDIR}
    ${PC_PCIACCESS_INCLUDE_DIRS}
    )

find_library(PCIACCESS_LIBRARY NAMES pciaccess
    HINTS
    ${PC_PCIACCESS_LIBDIR}
    ${PC_PCIACCESS_LIBRARY_DIRS}
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCIAccess DEFAULT_MSG
    PCIACCESS_INCLUDE_DIR PCIACCESS_LIBRARY)

mark_as_advanced(PCIACCESS_INCLUDE_DIR PCIACCESS_LIBRARY)

set(PCIACCESS_INCLUDE_DIRS ${PCIACCESS_INCLUDE_DIR})
set(PCIACCESS_LIBRARIES ${PCIACCESS_LIBRARY})
