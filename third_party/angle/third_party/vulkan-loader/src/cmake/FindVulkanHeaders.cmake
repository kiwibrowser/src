#.rst:
# FindVulkanHeaders
# -----------------
#
# Try to find Vulkan Headers and Registry.
#
# This module is intended to be used by projects that build Vulkan
# "system" components such as the loader and layers.
# Vulkan applications should instead use the FindVulkan (or similar)
# find module that locates the headers and the loader library.
#
# When using this find module to locate the headers and registry
# in a Vulkan-Headers repository, the Vulkan-Headers repository
# should be built with 'install' target and the following environment
# or CMake variable set to the location of the install directory.
#
#    VULKAN_HEADERS_INSTALL_DIR
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines no IMPORTED targets
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   VulkanHeaders_FOUND          - True if VulkanHeaders was found
#   VulkanHeaders_INCLUDE_DIRS   - include directories for VulkanHeaders
#
#   VulkanRegistry_FOUND         - True if VulkanRegistry was found
#   VulkanRegistry_DIRS          - directories for VulkanRegistry
#
#   VulkanHeaders_VERSION_MAJOR  - The Major API version of the latest version
#                                  contained in the Vulkan header
#   VulkanHeaders_VERSION_MINOR  - The Minor API version of the latest version
#                                  contained in the Vulkan header
#   VulkanHeaders_VERSION_PATCH  - The Patch API version of the latest version
#                                  contained in the Vulkan header
#
# The module will also define two cache variables::
#
#   VulkanHeaders_INCLUDE_DIR    - the VulkanHeaders include directory
#   VulkanRegistry_DIR           - the VulkanRegistry directory
#

# Use HINTS instead of PATH to search these locations before
# searching system environment variables like $PATH that may
# contain SDK directories.
find_path(VulkanHeaders_INCLUDE_DIR
    NAMES vulkan/vulkan.h
    HINTS
        ${VULKAN_HEADERS_INSTALL_DIR}/include
        "$ENV{VULKAN_HEADERS_INSTALL_DIR}/include"
        "$ENV{VULKAN_SDK}/include")

if(VulkanHeaders_INCLUDE_DIR)
   get_filename_component(VULKAN_REGISTRY_PATH_HINT ${VulkanHeaders_INCLUDE_DIR} DIRECTORY)
   find_path(VulkanRegistry_DIR
       NAMES vk.xml
       HINTS "${VULKAN_REGISTRY_PATH_HINT}/share/vulkan/registry")
endif()

set(VulkanHeaders_INCLUDE_DIRS ${VulkanHeaders_INCLUDE_DIR})
set(VulkanRegistry_DIRS ${VulkanRegistry_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VulkanHeaders
    DEFAULT_MSG
    VulkanHeaders_INCLUDE_DIR)
find_package_handle_standard_args(VulkanRegistry
    DEFAULT_MSG
    VulkanRegistry_DIR)

mark_as_advanced(VulkanHeaders_INCLUDE_DIR VulkanRegistry_DIR)

# Determine the major/minor/patch version from the vulkan header
set(VulkanHeaders_VERSION_MAJOR "0")
set(VulkanHeaders_VERSION_MINOR "0")
set(VulkanHeaders_VERSION_PATCH "0")

# First, determine which header we need to grab the version information from.
# Starting with Vulkan 1.1, we should use vulkan_core.h, but prior to that,
# the information was in vulkan.h.
if (EXISTS "${VulkanHeaders_INCLUDE_DIR}/vulkan/vulkan_core.h")
    set(VulkanHeaders_main_header ${VulkanHeaders_INCLUDE_DIR}/vulkan/vulkan_core.h)
else()
    set(VulkanHeaders_main_header ${VulkanHeaders_INCLUDE_DIR}/vulkan/vulkan.h)
endif()

# Find all lines in the header file that contain any version we may be interested in
#  NOTE: They start with #define and then have other keywords
file(STRINGS
        ${VulkanHeaders_main_header}
        VulkanHeaders_lines
        REGEX "^#define (VK_API_VERSION.*VK_MAKE_VERSION|VK_HEADER_VERSION)")

foreach(VulkanHeaders_line ${VulkanHeaders_lines})

    # First, handle the case where we have a major/minor version
    #   Format is:
    #        #define VK_API_VERSION_X_Y VK_MAKE_VERSION(X, Y, 0)
    #   We grab the major version (X) and minor version (Y) out of the parentheses
    string(REGEX MATCH "VK_MAKE_VERSION\\(.*\\)" VulkanHeaders_out ${VulkanHeaders_line})
    string(REGEX MATCHALL "[0-9]+" VulkanHeaders_MAJOR_MINOR "${VulkanHeaders_out}")
    if (VulkanHeaders_MAJOR_MINOR)
        list (GET VulkanHeaders_MAJOR_MINOR 0 VulkanHeaders_cur_major)
        list (GET VulkanHeaders_MAJOR_MINOR 1 VulkanHeaders_cur_minor)
        if (${VulkanHeaders_cur_major} GREATER ${VulkanHeaders_VERSION_MAJOR})
            set(VulkanHeaders_VERSION_MAJOR ${VulkanHeaders_cur_major})
            set(VulkanHeaders_VERSION_MINOR ${VulkanHeaders_cur_minor})
        endif()
        if (${VulkanHeaders_cur_major} EQUAL ${VulkanHeaders_VERSION_MAJOR} AND
            ${VulkanHeaders_cur_minor} GREATER ${VulkanHeaders_VERSION_MINOR})
            set(VulkanHeaders_VERSION_MINOR ${VulkanHeaders_cur_minor})
        endif()
    endif()

    # Second, handle the case where we have the patch version
    #   Format is:
    #      #define VK_HEADER_VERSION Z
    #   Where Z is the patch version which we just grab off the end
    string(REGEX MATCH "define.*VK_HEADER_VERSION.*[0-9]+" VulkanHeaders_out ${VulkanHeaders_line})
    list(LENGTH VulkanHeaders_out VulkanHeaders_len)
    if (VulkanHeaders_len)
        string(REGEX MATCH "[0-9]+" VulkanHeaders_VERSION_PATCH "${VulkanHeaders_out}")
    endif()

endforeach()
MESSAGE(STATUS
        "Detected Vulkan Version ${VulkanHeaders_VERSION_MAJOR}."
        "${VulkanHeaders_VERSION_MINOR}."
        "${VulkanHeaders_VERSION_PATCH}")
