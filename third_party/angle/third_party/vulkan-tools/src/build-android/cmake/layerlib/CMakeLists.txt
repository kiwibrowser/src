cmake_minimum_required(VERSION 3.4.1)

# Validation layers could be built with code from
#    github repo   OR
#    ndk's snapshot
# The file structure for glslang spir-V is different, adding BUILD_IN_NDK variable
set(BUILD_IN_NDK OFF)
if (CMAKE_CURRENT_SOURCE_DIR MATCHES "sources/third_party/vulkan/src")
    set(BUILD_IN_NDK ON)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror \
        -Wno-unused-function -Wno-unused-const-variable \
        -Wno-c++98-compat-pedantic -DVK_PROTOTYPES \
        -DVK_USE_PLATFORM_ANDROID_KHR")

if (BUILD_IN_NDK)
    # NDK directory structure:
    #  $ANDROID_NDK/sources/third_party/
    #        vulkan/src
    #        shaderc/third_party (for glslang & spirv-tools )
    # this build uses combined lib libshaderc.a for libSPIRV-Tools.a purpose
    get_filename_component(SRC_DIR
        "${ANDROID_NDK}/sources/third_party/vulkan/src" ABSOLUTE)
    get_filename_component(EXTERNAL_DIR
        "${SRC_DIR}/../../shaderc/third_party" ABSOLUTE)
    get_filename_component(SPIRV_LIB
        "${SRC_DIR}/../../shaderc/libs/${ANDROID_STL}/${ANDROID_ABI}/libshaderc.a"
        ABSOLUTE)
else ()
    # github directory structure:
    #   ${SRC_DIR}
    #   ${SRC_DIR}/build-android/external (for glslang, spirv-tools & shaderc )
    get_filename_component(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.."  ABSOLUTE)
    set(EXTERNAL_DIR "${SRC_DIR}/build-android/third_party/shaderc/third_party")
    set(SPIRV_LIB
        "${SRC_DIR}/build-android/third_party/shaderc/android_test/obj/local/${ANDROID_ABI}/libSPIRV-Tools.a")
endif()
set(COMMON_DIR "${SRC_DIR}/build-android/generated")

set(CMAKE_CXX_clone "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR \
        -fvisibility=hidden")
include_directories(${SRC_DIR}/include
                    ${COMMON_DIR}/include
                    ${SRC_DIR}/layers
                    ${SRC_DIR}/loader)
add_library(layer_utils STATIC
        ${SRC_DIR}/layers/vk_layer_config.cpp
        ${SRC_DIR}/layers/vk_layer_extension_utils.cpp
        ${SRC_DIR}/layers/vk_layer_utils.cpp
        ${SRC_DIR}/layers/vk_format_utils.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")

# assume shaderc already built externally
add_library(SPIRV-Tools-prebuilt STATIC IMPORTED)
set_target_properties(SPIRV-Tools-prebuilt PROPERTIES IMPORTED_LOCATION
    ${SPIRV_LIB})

# build core_validation layers which including shader validation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR \
                     -fvisibility=hidden")
add_library(VkLayer_core_validation SHARED
        ${SRC_DIR}/layers/core_validation.cpp
        ${SRC_DIR}/layers/descriptor_sets.cpp
        ${SRC_DIR}/layers/buffer_validation.cpp
        ${SRC_DIR}/layers/shader_validation.cpp
        ${SRC_DIR}/layers/vk_layer_table.cpp
	${SRC_DIR}/layers/xxhash.c)
target_include_directories(VkLayer_core_validation PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader
        ${EXTERNAL_DIR}/glslang
        ${EXTERNAL_DIR}/spirv-tools/include)
target_link_libraries(VkLayer_core_validation PRIVATE
        log layer_utils SPIRV-Tools-prebuilt)

add_library(VkLayer_parameter_validation SHARED
        ${COMMON_DIR}/include/parameter_validation.cpp
        ${SRC_DIR}/layers/parameter_validation_utils.cpp
        ${SRC_DIR}/layers/vk_layer_table.cpp)
target_include_directories(VkLayer_parameter_validation PRIVATE
        ${SRC_DIR}/include
        ${COMMON_DIR}/include
        ${SRC_DIR}/layers
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_parameter_validation PRIVATE log layer_utils)

add_library(VkLayer_object_tracker SHARED
        ${COMMON_DIR}/include/object_tracker.cpp
        ${SRC_DIR}/layers/object_tracker_utils.cpp
        ${SRC_DIR}/layers/vk_layer_table.cpp)
target_include_directories(VkLayer_object_tracker PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_object_tracker PRIVATE log layer_utils)

add_library(VkLayer_threading SHARED
        ${SRC_DIR}/layers/threading.cpp
        ${SRC_DIR}/layers/vk_layer_table.cpp)
target_include_directories(VkLayer_threading PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_threading PRIVATE log layer_utils)

add_library(VkLayer_unique_objects SHARED
        ${SRC_DIR}/layers/unique_objects.cpp
        ${SRC_DIR}/layers/vk_layer_table.cpp)
target_include_directories(VkLayer_unique_objects PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${SRC_DIR}/loader)
target_link_libraries(VkLayer_unique_objects PRIVATE log layer_utils)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")
