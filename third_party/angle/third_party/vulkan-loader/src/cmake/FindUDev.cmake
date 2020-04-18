# - FindUDev
#
# Copyright (C) 2015 Valve Corporation

find_package(PkgConfig)

pkg_check_modules(PC_LIBUDEV QUIET libudev)

find_path(UDEV_INCLUDE_DIR NAMES libudev.h
    HINTS
    ${PC_LIBUDEV_INCLUDEDIR}
    ${PC_LIBUDEV_INCLUDE_DIRS}
    )

find_library(UDEV_LIBRARY NAMES udev
    HINTS
    ${PC_LIBUDEV_LIBDIR}
    ${PC_LIBUDEV_LIBRARY_DIRS}
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDev DEFAULT_MSG
    UDEV_INCLUDE_DIR UDEV_LIBRARY)

mark_as_advanced(UDEV_INCLUDE_DIR UDEV_LIBRARY)

set(UDEV_INCLUDE_DIRS ${UDEV_INCLUDE_DIR})
set(UDEV_LIBRARIES ${UDEV_LIBRARY})
