/*
 * Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and/or associated documentation files (the "Materials"), to
 * deal in the Materials without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Materials, and to permit persons to whom the Materials are
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included in
 * all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
 * USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 * Author: Jeremy Hayes <jeremy@lunarG.com>
 * Author: Mark Young <marky@lunarG.com>
 */

// Following items are needed for C++ to work with PRIxLEAST64
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdint.h>  // For UINT32_MAX

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "test_common.h"
#include <vulkan/vulkan.h>

namespace VK {

struct InstanceCreateInfo {
    InstanceCreateInfo()
        : info  // MSVC can't handle list initialization, thus explicit construction herein.
          (VkInstanceCreateInfo{
              VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  // sType
              nullptr,                                 // pNext
              0,                                       // flags
              nullptr,                                 // pApplicationInfo
              0,                                       // enabledLayerCount
              nullptr,                                 // ppEnabledLayerNames
              0,                                       // enabledExtensionCount
              nullptr                                  // ppEnabledExtensionNames
          }) {}

    InstanceCreateInfo &sType(VkStructureType const &sType) {
        info.sType = sType;

        return *this;
    }

    InstanceCreateInfo &pNext(void const *const pNext) {
        info.pNext = pNext;

        return *this;
    }

    InstanceCreateInfo &flags(VkInstanceCreateFlags const &flags) {
        info.flags = flags;

        return *this;
    }

    InstanceCreateInfo &pApplicationInfo(VkApplicationInfo const *const pApplicationInfo) {
        info.pApplicationInfo = pApplicationInfo;

        return *this;
    }

    InstanceCreateInfo &enabledLayerCount(uint32_t const &enabledLayerCount) {
        info.enabledLayerCount = enabledLayerCount;

        return *this;
    }

    InstanceCreateInfo &ppEnabledLayerNames(char const *const *const ppEnabledLayerNames) {
        info.ppEnabledLayerNames = ppEnabledLayerNames;

        return *this;
    }

    InstanceCreateInfo &enabledExtensionCount(uint32_t const &enabledExtensionCount) {
        info.enabledExtensionCount = enabledExtensionCount;

        return *this;
    }

    InstanceCreateInfo &ppEnabledExtensionNames(char const *const *const ppEnabledExtensionNames) {
        info.ppEnabledExtensionNames = ppEnabledExtensionNames;

        return *this;
    }

    operator VkInstanceCreateInfo const *() const { return &info; }

    operator VkInstanceCreateInfo *() { return &info; }

    VkInstanceCreateInfo info;
};

struct DeviceQueueCreateInfo {
    DeviceQueueCreateInfo()
        : info  // MSVC can't handle list initialization, thus explicit construction herein.
          (VkDeviceQueueCreateInfo{
              VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // sType
              nullptr,                                     // pNext
              0,                                           // flags
              0,                                           // queueFamilyIndex
              0,                                           // queueCount
              nullptr                                      // pQueuePriorities
          }) {}

    DeviceQueueCreateInfo &sType(VkStructureType const &sType) {
        info.sType = sType;

        return *this;
    }

    DeviceQueueCreateInfo &pNext(void const *const pNext) {
        info.pNext = pNext;

        return *this;
    }

    DeviceQueueCreateInfo &flags(VkDeviceQueueCreateFlags const &flags) {
        info.flags = flags;

        return *this;
    }

    DeviceQueueCreateInfo &queueFamilyIndex(uint32_t const &queueFamilyIndex) {
        info.queueFamilyIndex = queueFamilyIndex;

        return *this;
    }

    DeviceQueueCreateInfo &queueCount(uint32_t const &queueCount) {
        info.queueCount = queueCount;

        return *this;
    }

    DeviceQueueCreateInfo &pQueuePriorities(float const *const pQueuePriorities) {
        info.pQueuePriorities = pQueuePriorities;

        return *this;
    }

    operator VkDeviceQueueCreateInfo() { return info; }

    VkDeviceQueueCreateInfo info;
};

struct DeviceCreateInfo {
    DeviceCreateInfo()
        : info  // MSVC can't handle list initialization, thus explicit construction herein.
          (VkDeviceCreateInfo{
              VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,  // sType
              nullptr,                               // pNext
              0,                                     // flags
              0,                                     // queueCreateInfoCount
              nullptr,                               // pQueueCreateInfos
              0,                                     // enabledLayerCount
              nullptr,                               // ppEnabledLayerNames
              0,                                     // enabledExtensionCount
              nullptr,                               // ppEnabledExtensionNames
              nullptr                                // pEnabledFeatures
          }) {}

    DeviceCreateInfo &sType(VkStructureType const &sType) {
        info.sType = sType;

        return *this;
    }

    DeviceCreateInfo &pNext(void const *const pNext) {
        info.pNext = pNext;

        return *this;
    }

    DeviceCreateInfo &flags(VkDeviceQueueCreateFlags const &flags) {
        info.flags = flags;

        return *this;
    }

    DeviceCreateInfo &queueCreateInfoCount(uint32_t const &queueCreateInfoCount) {
        info.queueCreateInfoCount = queueCreateInfoCount;

        return *this;
    }

    DeviceCreateInfo &pQueueCreateInfos(VkDeviceQueueCreateInfo const *const pQueueCreateInfos) {
        info.pQueueCreateInfos = pQueueCreateInfos;

        return *this;
    }

    DeviceCreateInfo &enabledLayerCount(uint32_t const &enabledLayerCount) {
        info.enabledLayerCount = enabledLayerCount;

        return *this;
    }

    DeviceCreateInfo &ppEnabledLayerNames(char const *const *const ppEnabledLayerNames) {
        info.ppEnabledLayerNames = ppEnabledLayerNames;

        return *this;
    }

    DeviceCreateInfo &enabledExtensionCount(uint32_t const &enabledExtensionCount) {
        info.enabledExtensionCount = enabledExtensionCount;

        return *this;
    }

    DeviceCreateInfo &ppEnabledExtensionNames(char const *const *const ppEnabledExtensionNames) {
        info.ppEnabledExtensionNames = ppEnabledExtensionNames;

        return *this;
    }

    DeviceCreateInfo &pEnabledFeatures(VkPhysicalDeviceFeatures const *const pEnabledFeatures) {
        info.pEnabledFeatures = pEnabledFeatures;

        return *this;
    }

    operator VkDeviceCreateInfo const *() const { return &info; }

    operator VkDeviceCreateInfo *() { return &info; }

    VkDeviceCreateInfo info;
};
}  // namespace VK

struct CommandLine : public ::testing::Test {
    static void Initialize(int argc, char **argv) { arguments.assign(argv, argv + argc); };

    static void SetUpTestCase(){};
    static void TearDownTestCase(){};

    static std::vector<std::string> arguments;
};
std::vector<std::string> CommandLine::arguments;

struct EnumerateInstanceLayerProperties : public CommandLine {};
struct EnumerateInstanceExtensionProperties : public CommandLine {};
struct ImplicitLayer : public CommandLine {};

// Allocation tracking utilities
struct AllocTrack {
    bool active;
    bool was_allocated;
    void *aligned_start_addr;
    char *actual_start_addr;
    size_t requested_size_bytes;
    size_t actual_size_bytes;
    VkSystemAllocationScope alloc_scope;
    uint64_t user_data;

    AllocTrack()
        : active(false),
          was_allocated(false),
          aligned_start_addr(nullptr),
          actual_start_addr(nullptr),
          requested_size_bytes(0),
          actual_size_bytes(0),
          alloc_scope(VK_SYSTEM_ALLOCATION_SCOPE_COMMAND),
          user_data(0) {}
};

// Global vector to track allocations.  This will be resized before each test and emptied after.
// However, we have to globally define it so the allocation callback functions work properly.
std::vector<AllocTrack> g_allocated_vector;
bool g_intentional_fail_enabled = false;
uint32_t g_intenional_fail_index = 0;
uint32_t g_intenional_fail_count = 0;

void FreeAllocTracker() { g_allocated_vector.clear(); }

void InitAllocTracker(size_t size, uint32_t intentional_fail_index = UINT32_MAX) {
    if (g_allocated_vector.size() > 0) {
        FreeAllocTracker();
    }
    g_allocated_vector.resize(size);
    if (intentional_fail_index != UINT32_MAX) {
        g_intentional_fail_enabled = true;
        g_intenional_fail_index = intentional_fail_index;
        g_intenional_fail_count = 0;
    } else {
        g_intentional_fail_enabled = false;
        g_intenional_fail_index = 0;
        g_intenional_fail_count = 0;
    }
}

bool IsAllocTrackerEmpty() {
    bool success = true;
    bool was_allocated = false;
    char print_command[1024];
    sprintf(print_command, "\t%%04d\t%%p (%%p) : 0x%%%s (0x%%%s) : scope %%d : user_data 0x%%%s\n", PRIxLEAST64, PRIxLEAST64,
            PRIxLEAST64);
    for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
        if (g_allocated_vector[iii].active) {
            if (success) {
                printf("ERROR: Allocations still remain!\n");
            }
            printf(print_command, iii, g_allocated_vector[iii].aligned_start_addr, g_allocated_vector[iii].actual_start_addr,
                   g_allocated_vector[iii].requested_size_bytes, g_allocated_vector[iii].actual_size_bytes,
                   g_allocated_vector[iii].alloc_scope, g_allocated_vector[iii].user_data);
            success = false;
        } else if (!was_allocated && g_allocated_vector[iii].was_allocated) {
            was_allocated = true;
        }
    }
    if (!g_intentional_fail_enabled && !was_allocated) {
        printf("No allocations ever generated!");
        success = false;
    }
    return success;
}

VKAPI_ATTR void *VKAPI_CALL AllocCallbackFunc(void *pUserData, size_t size, size_t alignment,
                                              VkSystemAllocationScope allocationScope) {
    if (g_intentional_fail_enabled) {
        if (++g_intenional_fail_count >= g_intenional_fail_index) {
            return nullptr;
        }
    }
    for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
        if (!g_allocated_vector[iii].active) {
            g_allocated_vector[iii].requested_size_bytes = size;
            g_allocated_vector[iii].actual_size_bytes = size + (alignment - 1);
            g_allocated_vector[iii].aligned_start_addr = NULL;
            g_allocated_vector[iii].actual_start_addr = new char[g_allocated_vector[iii].actual_size_bytes];
            if (g_allocated_vector[iii].actual_start_addr != NULL) {
                uint64_t addr = (uint64_t)g_allocated_vector[iii].actual_start_addr;
                addr += (alignment - 1);
                addr &= ~(alignment - 1);
                g_allocated_vector[iii].aligned_start_addr = (void *)addr;
                g_allocated_vector[iii].alloc_scope = allocationScope;
                g_allocated_vector[iii].user_data = (uint64_t)pUserData;
                g_allocated_vector[iii].active = true;
                g_allocated_vector[iii].was_allocated = true;
            }
            return g_allocated_vector[iii].aligned_start_addr;
        }
    }
    return nullptr;
}

VKAPI_ATTR void VKAPI_CALL FreeCallbackFunc(void *pUserData, void *pMemory) {
    for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
        if (g_allocated_vector[iii].active && g_allocated_vector[iii].aligned_start_addr == pMemory) {
            delete[] g_allocated_vector[iii].actual_start_addr;
            g_allocated_vector[iii].active = false;
            break;
        }
    }
}

VKAPI_ATTR void *VKAPI_CALL ReallocCallbackFunc(void *pUserData, void *pOriginal, size_t size, size_t alignment,
                                                VkSystemAllocationScope allocationScope) {
    if (pOriginal != NULL) {
        for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
            if (g_allocated_vector[iii].active && g_allocated_vector[iii].aligned_start_addr == pOriginal) {
                if (size == 0) {
                    FreeCallbackFunc(pUserData, pOriginal);
                    return nullptr;
                } else if (size < g_allocated_vector[iii].requested_size_bytes) {
                    return pOriginal;
                } else {
                    void *pNew = AllocCallbackFunc(pUserData, size, alignment, allocationScope);
                    if (pNew != NULL) {
                        size_t copy_size = size;
                        if (g_allocated_vector[iii].requested_size_bytes < size) {
                            copy_size = g_allocated_vector[iii].requested_size_bytes;
                        }
                        memcpy(pNew, pOriginal, copy_size);
                        FreeCallbackFunc(pUserData, pOriginal);
                    }
                    return pNew;
                }
            }
        }
        return nullptr;
    } else {
        return AllocCallbackFunc(pUserData, size, alignment, allocationScope);
    }
}

void test_create_device(VkPhysicalDevice physical) {
    uint32_t familyCount = 0;
    VkResult result;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, nullptr);
    ASSERT_GT(familyCount, 0u);

    std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, family.get());
    ASSERT_GT(familyCount, 0u);

    for (uint32_t q = 0; q < familyCount; ++q) {
        if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            continue;
        }

        float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
        VkDeviceQueueCreateInfo const queueInfo[1]{
            VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

        auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

        VkDevice device;
        result = vkCreateDevice(physical, deviceInfo, nullptr, &device);
        ASSERT_EQ(result, VK_SUCCESS);

        vkDestroyDevice(device, nullptr);
    }
}

// Test groups:
// LX = lunar exchange
// LVLGH = loader and validation github
// LVLGL = lodaer and validation gitlab

TEST(LX435, InstanceCreateInfoConst) {
    VkInstanceCreateInfo const info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, nullptr, 0, nullptr, 0, nullptr};

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&info, VK_NULL_HANDLE, &instance);
    EXPECT_EQ(result, VK_SUCCESS);

    vkDestroyInstance(instance, nullptr);
}

TEST(LX475, DestroyInstanceNullHandle) { vkDestroyInstance(VK_NULL_HANDLE, nullptr); }

TEST(LX475, DestroyDeviceNullHandle) { vkDestroyDevice(VK_NULL_HANDLE, nullptr); }

TEST(CreateInstance, ExtensionNotPresent) {
    char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().enabledExtensionCount(1).ppEnabledExtensionNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);

    // It's not necessary to destroy the instance because it will not be created successfully.
}

TEST(CreateInstance, LayerNotPresent) {
    char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().enabledLayerCount(1).ppEnabledLayerNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_LAYER_NOT_PRESENT);

    // It's not necessary to destroy the instance because it will not be created successfully.
}

// Used by run_loader_tests.sh to test for layer insertion.
TEST(CreateInstance, LayerPresent) {
    char const *const names1[] = {"VK_LAYER_LUNARG_test"};  // Temporary required due to MSVC bug.
    char const *const names2[] = {"VK_LAYER_LUNARG_meta"};  // Temporary required due to MSVC bug.
    char const *const names3[] = {"VK_LAYER_LUNARG_meta_rev"};  // Temporary required due to MSVC bug.
    auto const info1 = VK::InstanceCreateInfo().enabledLayerCount(1).ppEnabledLayerNames(names1);
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info1, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);
    vkDestroyInstance(instance, nullptr);

    for (auto names : {names2, names3}) {
        auto const info2 = VK::InstanceCreateInfo().enabledLayerCount(1).ppEnabledLayerNames(names);
        instance = VK_NULL_HANDLE;
        result = vkCreateInstance(info2, VK_NULL_HANDLE, &instance);
        ASSERT_EQ(result, VK_SUCCESS);

        uint32_t deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devs(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devs.data());
        test_create_device(devs[0]);

        vkDestroyInstance(instance, nullptr);
    }
}

// Used by run_loader_tests.sh to test that calling vkEnumeratePhysicalDevices without first querying
// the count, works.
TEST(EnumeratePhysicalDevices, OneCall) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 500;
    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    vkDestroyInstance(instance, nullptr);
}

// Used by run_loader_tests.sh to test for the expected usage of the vkEnumeratePhysicalDevices call.
TEST(EnumeratePhysicalDevices, TwoCall) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    vkDestroyInstance(instance, nullptr);
}

// Used by run_loader_tests.sh to test that calling vkEnumeratePhysicalDevices without first querying
// the count, matches the count from the standard call.
TEST(EnumeratePhysicalDevices, MatchOneAndTwoCallNumbers) {
    VkInstance instance_one = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance_one);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount_one = 500;
    std::unique_ptr<VkPhysicalDevice[]> physical_one(new VkPhysicalDevice[physicalCount_one]);
    result = vkEnumeratePhysicalDevices(instance_one, &physicalCount_one, physical_one.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount_one, 0u);

    VkInstance instance_two = VK_NULL_HANDLE;
    result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance_two);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount_two = 0;
    result = vkEnumeratePhysicalDevices(instance_two, &physicalCount_two, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount_two, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical_two(new VkPhysicalDevice[physicalCount_two]);
    result = vkEnumeratePhysicalDevices(instance_two, &physicalCount_two, physical_two.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount_two, 0u);

    ASSERT_EQ(physicalCount_one, physicalCount_two);

    vkDestroyInstance(instance_one, nullptr);
    vkDestroyInstance(instance_two, nullptr);
}

// Used by run_loader_tests.sh to test for the expected usage of the vkEnumeratePhysicalDevices
// call if not enough numbers are provided for the final list.
TEST(EnumeratePhysicalDevices, TwoCallIncomplete) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);

    // Remove one from the physical device count so we can get the VK_INCOMPLETE message
    physicalCount -= 1;

    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_INCOMPLETE);

    vkDestroyInstance(instance, nullptr);
}

// Test to make sure that layers enabled in the instance show up in the list of device layers.
TEST(EnumerateDeviceLayers, LayersMatch) {
    char const *const names1[] = {"VK_LAYER_LUNARG_meta"};
    char const *const names2[2] = {"VK_LAYER_LUNARG_test", "VK_LAYER_LUNARG_wrap_objects"};
    auto const info1 = VK::InstanceCreateInfo().enabledLayerCount(1).ppEnabledLayerNames(names1);
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info1, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);
    uint32_t count = 24;
    VkLayerProperties layer_props[24];
    vkEnumerateDeviceLayerProperties(physical[0], &count, layer_props);
    ASSERT_GE(count, 1u);
    bool found = false;
    for (uint32_t iii = 0; iii < count; iii++) {
        if (!strcmp(layer_props[iii].layerName, names1[0])) {
            found = true;
            break;
        }
    }
    if (!found) {
        ASSERT_EQ(count, 0);
    }

    vkDestroyInstance(instance, nullptr);

    auto const info2 = VK::InstanceCreateInfo().enabledLayerCount(2).ppEnabledLayerNames(names2);
    instance = VK_NULL_HANDLE;
    result = vkCreateInstance(info2, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical2(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical2.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    count = 24;
    vkEnumerateDeviceLayerProperties(physical2[0], &count, layer_props);
    ASSERT_GE(count, 2u);
    for (uint32_t jjj = 0; jjj < 2; jjj++) {
        found = false;
        for (uint32_t iii = 0; iii < count; iii++) {
            if (!strcmp(layer_props[iii].layerName, names2[jjj])) {
                found = true;
                break;
            }
        }
        if (!found) {
            ASSERT_EQ(count, 0);
        }
    }

    vkDestroyInstance(instance, nullptr);
}

TEST(CreateDevice, ExtensionNotPresent) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
            auto const deviceInfo = VK::DeviceCreateInfo()
                                        .queueCreateInfoCount(1)
                                        .pQueueCreateInfos(queueInfo)
                                        .enabledExtensionCount(1)
                                        .ppEnabledExtensionNames(names);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);

            // It's not necessary to destroy the device because it will not be created successfully.
        }
    }

    vkDestroyInstance(instance, nullptr);
}

// LX535 / MI-76: Device layers are deprecated.
// For backwards compatibility, they are allowed, but must be ignored.
// Ensure that no errors occur if a bogus device layer list is passed to vkCreateDevice.
TEST(CreateDevice, LayersNotPresent) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
            auto const deviceInfo = VK::DeviceCreateInfo()
                                        .queueCreateInfoCount(1)
                                        .pQueueCreateInfos(queueInfo)
                                        .enabledLayerCount(1)
                                        .ppEnabledLayerNames(names);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            vkDestroyDevice(device, nullptr);
        }
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(EnumerateInstanceLayerProperties, PropertyCountLessThanAvailable) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    // We need atleast two for the test to be relevant.
    if (count < 2u) {
        return;
    }

    std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
    count = 1;
    result = vkEnumerateInstanceLayerProperties(&count, properties.get());
    ASSERT_EQ(result, VK_INCOMPLETE);
}

TEST(EnumerateDeviceLayerProperties, PropertyCountLessThanAvailable) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t count = 0u;
        result = vkEnumerateDeviceLayerProperties(physical[p], &count, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);

        // We need atleast two for the test to be relevant.
        if (count < 2u) {
            continue;
        }

        std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
        count = 1;
        result = vkEnumerateDeviceLayerProperties(physical[p], &count, properties.get());
        ASSERT_EQ(result, VK_INCOMPLETE);
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(EnumerateInstanceLayerProperties, Count) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "count") != arguments.end()) {
        std::cout << "count=" << count << '\n';
    }
}

TEST_F(EnumerateInstanceLayerProperties, OnePass) {
    // Count required for this test.
    if (std::find(arguments.begin(), arguments.end(), "count") == arguments.end()) {
        return;
    }

    uint32_t count = std::stoul(arguments[2]);

    std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
    VkResult result = vkEnumerateInstanceLayerProperties(&count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].layerName << ' ' << properties[p].specVersion << ' '
                      << properties[p].implementationVersion << ' ' << properties[p].description << '\n';
        }
    }
}

TEST_F(EnumerateInstanceLayerProperties, TwoPass) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
    result = vkEnumerateInstanceLayerProperties(&count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].layerName << ' ' << properties[p].specVersion << ' '
                      << properties[p].implementationVersion << ' ' << properties[p].description << '\n';
        }
    }
}

TEST_F(EnumerateInstanceExtensionProperties, PropertyCountLessThanAvailable) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    // We need atleast two for the test to be relevant.
    if (count < 2u) {
        return;
    }

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    count = 1;
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_INCOMPLETE);
}

TEST(EnumerateDeviceExtensionProperties, PropertyCountLessThanAvailable) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t count = 0u;
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);

        // We need atleast two for the test to be relevant.
        if (count < 2u) {
            continue;
        }

        std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
        count = 1;
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, properties.get());
        ASSERT_EQ(result, VK_INCOMPLETE);
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(EnumerateInstanceExtensionProperties, Count) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "count") != arguments.end()) {
        std::cout << "count=" << count << '\n';
    }
}

TEST_F(EnumerateInstanceExtensionProperties, OnePass) {
    // Count required for this test.
    if (std::find(arguments.begin(), arguments.end(), "count") == arguments.end()) {
        return;
    }

    uint32_t count = std::stoul(arguments[2]);

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].extensionName << ' ' << properties[p].specVersion
                      << '\n';
        }
    }
}

TEST_F(EnumerateInstanceExtensionProperties, TwoPass) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].extensionName << ' ' << properties[p].specVersion
                      << '\n';
        }
    }
}

TEST_F(EnumerateInstanceExtensionProperties, InstanceExtensionEnumerated) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    ASSERT_NE(std::find_if(
                  &properties[0], &properties[count],
                  [](VkExtensionProperties const &properties) { return strcmp(properties.extensionName, "VK_KHR_surface") == 0; }),
              &properties[count]);
}

TEST(EnumerateDeviceExtensionProperties, DeviceExtensionEnumerated) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t count = 0u;
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);

        std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, properties.get());
        ASSERT_EQ(result, VK_SUCCESS);

        ASSERT_NE(std::find_if(&properties[0], &properties[count],
                               [](VkExtensionProperties const &properties) {
                                   return strcmp(properties.extensionName, "VK_KHR_swapchain") == 0;
                               }),
                  &properties[count]);
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(ImplicitLayer, Present) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    vkDestroyInstance(instance, nullptr);
}

TEST(WrapObjects, Insert) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        test_create_device(physical[p]);
    }

    vkDestroyInstance(instance, nullptr);
}

// Test making sure the allocation functions are called to allocate and cleanup everything during
// a CreateInstance/DestroyInstance call pair.
TEST(Allocation, Instance) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000001;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    alloc_callbacks.pUserData = (void *)0x00000002;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything during
// a CreateInstance/DestroyInstance call pair with a call to GetInstanceProcAddr.
TEST(Allocation, GetInstanceProcAddr) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000010;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    void *pfnCreateDevice = (void *)vkGetInstanceProcAddr(instance, "vkCreateDevice");
    void *pfnDestroyDevice = (void *)vkGetInstanceProcAddr(instance, "vkDestroyDevice");
    ASSERT_TRUE(pfnCreateDevice != NULL && pfnDestroyDevice != NULL);

    alloc_callbacks.pUserData = (void *)0x00000011;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything during
// a vkEnumeratePhysicalDevices call pair.
TEST(Allocation, EnumeratePhysicalDevices) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000021;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    alloc_callbacks.pUserData = (void *)0x00000022;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    alloc_callbacks.pUserData = (void *)0x00000023;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    alloc_callbacks.pUserData = (void *)0x00000024;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything from
// vkCreateInstance, to vkCreateDevicce, and then through their destructors.  With special
// allocators used on both the instance and device.
TEST(Allocation, InstanceAndDevice) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000031;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            VkDevice device;
            alloc_callbacks.pUserData = (void *)0x00000032;
            result = vkCreateDevice(physical[p], deviceInfo, &alloc_callbacks, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            alloc_callbacks.pUserData = (void *)0x00000033;
            vkDestroyDevice(device, &alloc_callbacks);
        }
    }

    alloc_callbacks.pUserData = (void *)0x00000034;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything from
// vkCreateInstance, to vkCreateDevicce, and then through their destructors.  With special
// allocators used on only the instance and not the device.
TEST(Allocation, InstanceButNotDevice) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000041;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, NULL, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            vkDestroyDevice(device, NULL);
        }
    }

    alloc_callbacks.pUserData = (void *)0x00000042;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything from
// vkCreateInstance, to vkCreateDevicce, and then through their destructors.  With special
// allocators used on only the device and not the instance.
TEST(Allocation, DeviceButNotInstance) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, NULL, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            VkDevice device;
            alloc_callbacks.pUserData = (void *)0x00000051;
            result = vkCreateDevice(physical[p], deviceInfo, &alloc_callbacks, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            alloc_callbacks.pUserData = (void *)0x00000052;
            vkDestroyDevice(device, &alloc_callbacks);
        }
    }

    vkDestroyInstance(instance, NULL);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test failure during vkCreateInstance to make sure we don't leak memory if
// one of the out-of-memory conditions trigger.
TEST(Allocation, CreateInstanceIntentionalAllocFail) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    VkResult result;
    uint32_t fail_index = 1;
    do {
        InitAllocTracker(9999, fail_index);

        result = vkCreateInstance(info, &alloc_callbacks, &instance);
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            if (!IsAllocTrackerEmpty()) {
                std::cout << "Failed on index " << fail_index << '\n';
                ASSERT_EQ(true, IsAllocTrackerEmpty());
            }
        }
        fail_index++;
        // Make sure we don't overrun the memory
        ASSERT_LT(fail_index, 9999u);

        FreeAllocTracker();
    } while (result == VK_ERROR_OUT_OF_HOST_MEMORY);

    vkDestroyInstance(instance, &alloc_callbacks);
}

// Test failure during vkCreateDevice to make sure we don't leak memory if
// one of the out-of-memory conditions trigger.
TEST(Allocation, CreateDeviceIntentionalAllocFail) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    VkResult result = vkCreateInstance(info, NULL, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            uint32_t fail_index = 1;
            do {
                InitAllocTracker(9999, fail_index);

                result = vkCreateDevice(physical[p], deviceInfo, &alloc_callbacks, &device);
                if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
                    if (!IsAllocTrackerEmpty()) {
                        std::cout << "Failed on index " << fail_index << '\n';
                        ASSERT_EQ(true, IsAllocTrackerEmpty());
                    }
                }
                fail_index++;
                // Make sure we don't overrun the memory
                ASSERT_LT(fail_index, 9999u);

                FreeAllocTracker();
            } while (result == VK_ERROR_OUT_OF_HOST_MEMORY);
            vkDestroyDevice(device, &alloc_callbacks);
            break;
        }
    }

    vkDestroyInstance(instance, NULL);
}

// Test failure during vkCreateInstance and vkCreateDevice to make sure we don't
// leak memory if one of the out-of-memory conditions trigger.
TEST(Allocation, CreateInstanceDeviceIntentionalAllocFail) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    VkResult result = VK_ERROR_OUT_OF_HOST_MEMORY;
    uint32_t fail_index = 0;
    uint32_t physicalCount = 0;
    while (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
        InitAllocTracker(9999, ++fail_index);
        ASSERT_LT(fail_index, 9999u);

        result = vkCreateInstance(info, &alloc_callbacks, &instance);
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            if (!IsAllocTrackerEmpty()) {
                std::cout << "Failed on index " << fail_index << '\n';
                ASSERT_EQ(true, IsAllocTrackerEmpty());
            }
            FreeAllocTracker();
            continue;
        }
        ASSERT_EQ(result, VK_SUCCESS);

        physicalCount = 0;
        result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            vkDestroyInstance(instance, &alloc_callbacks);
            if (!IsAllocTrackerEmpty()) {
                std::cout << "Failed on index " << fail_index << '\n';
                ASSERT_EQ(true, IsAllocTrackerEmpty());
            }
            FreeAllocTracker();
            continue;
        }
        ASSERT_EQ(result, VK_SUCCESS);

        std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
        result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            vkDestroyInstance(instance, &alloc_callbacks);
            if (!IsAllocTrackerEmpty()) {
                std::cout << "Failed on index " << fail_index << '\n';
                ASSERT_EQ(true, IsAllocTrackerEmpty());
            }
            FreeAllocTracker();
            continue;
        }
        ASSERT_EQ(result, VK_SUCCESS);

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[0], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[0], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        uint32_t queue_index = 0;
        for (uint32_t q = 0; q < familyCount; ++q) {
            if (family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queue_index = q;
                break;
            }
        }

        float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
        VkDeviceQueueCreateInfo const queueInfo[1]{
            VK::DeviceQueueCreateInfo().queueFamilyIndex(queue_index).queueCount(1).pQueuePriorities(priorities)};

        auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

        result = vkCreateDevice(physical[0], deviceInfo, &alloc_callbacks, &device);
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            vkDestroyInstance(instance, &alloc_callbacks);
            if (!IsAllocTrackerEmpty()) {
                std::cout << "Failed on index " << fail_index << '\n';
                ASSERT_EQ(true, IsAllocTrackerEmpty());
            }
            FreeAllocTracker();
            continue;
        }
        vkDestroyDevice(device, &alloc_callbacks);
        vkDestroyInstance(instance, &alloc_callbacks);
        FreeAllocTracker();
    }
}

// Used by run_loader_tests.sh to test that calling vkEnumeratePhysicalDeviceGroupsKHR without first querying
// the count, works.  And, that it also returns only physical devices made available by the standard
// enumerate call
TEST(EnumeratePhysicalDeviceGroupsKHR, OneCall) {
    VkInstance instance = VK_NULL_HANDLE;
    char const *const names[] = {VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME};
    auto const info = VK::InstanceCreateInfo().enabledExtensionCount(1).ppEnabledExtensionNames(names);
    uint32_t group;
    uint32_t dev;
    std::vector<std::pair<VkPhysicalDevice, bool>> phys_dev_normal_found;
    std::vector<std::pair<VkPhysicalDevice, bool>> phys_dev_group_found;

    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        // Extension isn't present, just skip this test
        ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);
        std::cout << "Skipping EnumeratePhysicalDeviceGroupsKHR : OneCall due to Instance lacking support"
                  << " for " << VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME << " extension\n";
        return;
    }

    uint32_t phys_dev_count = 500;
    std::unique_ptr<VkPhysicalDevice[]> phys_devs(new VkPhysicalDevice[phys_dev_count]);
    result = vkEnumeratePhysicalDevices(instance, &phys_dev_count, phys_devs.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(phys_dev_count, 0u);

    // Initialize the normal physical device boolean pair array
    for (dev = 0; dev < phys_dev_count; dev++) {
        phys_dev_normal_found.push_back(std::make_pair(phys_devs[dev], false));
    }

    // Get a pointer to the new vkEnumeratePhysicalDeviceGroupsKHR call
    PFN_vkEnumeratePhysicalDeviceGroupsKHR p_vkEnumeratePhysicalDeviceGroupsKHR =
        (PFN_vkEnumeratePhysicalDeviceGroupsKHR)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroupsKHR");

    // Setup the group information in preparation for the call
    uint32_t group_count = 30;
    std::unique_ptr<VkPhysicalDeviceGroupPropertiesKHR[]> phys_dev_groups(new VkPhysicalDeviceGroupPropertiesKHR[group_count]);
    for (group = 0; group < group_count; group++) {
        phys_dev_groups[group].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES_KHR;
        phys_dev_groups[group].pNext = nullptr;
        phys_dev_groups[group].physicalDeviceCount = 0;
        memset(phys_dev_groups[group].physicalDevices, 0, sizeof(VkPhysicalDevice) * VK_MAX_DEVICE_GROUP_SIZE_KHR);
        phys_dev_groups[group].subsetAllocation = VK_FALSE;
    }

    result = p_vkEnumeratePhysicalDeviceGroupsKHR(instance, &group_count, phys_dev_groups.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(group_count, 0u);

    // Initialize the group physical device boolean pair array
    for (group = 0; group < group_count; group++) {
        for (dev = 0; dev < phys_dev_groups[group].physicalDeviceCount; dev++) {
            phys_dev_group_found.push_back(std::make_pair(phys_dev_groups[group].physicalDevices[dev], false));
        }
    }

    // Now, make sure we can find each normal and group item in the other list
    for (dev = 0; dev < phys_dev_count; dev++) {
        for (group = 0; group < phys_dev_group_found.size(); group++) {
            if (phys_dev_normal_found[dev].first == phys_dev_group_found[group].first) {
                phys_dev_normal_found[dev].second = true;
                phys_dev_group_found[group].second = true;
                break;
            }
        }
    }

    for (dev = 0; dev < phys_dev_count; dev++) {
        ASSERT_EQ(phys_dev_normal_found[dev].second, true);
    }
    for (dev = 0; dev < phys_dev_group_found.size(); dev++) {
        ASSERT_EQ(phys_dev_group_found[dev].second, true);
    }

    vkDestroyInstance(instance, nullptr);
}

// Used by run_loader_tests.sh to test for the expected usage of the
// vkEnumeratePhysicalDeviceGroupsKHR call in a two call fasion (once with NULL data
// to get count, and then again with data).
TEST(EnumeratePhysicalDeviceGroupsKHR, TwoCall) {
    VkInstance instance = VK_NULL_HANDLE;
    char const *const names[] = {VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME};
    auto const info = VK::InstanceCreateInfo().enabledExtensionCount(1).ppEnabledExtensionNames(names);
    uint32_t group;
    uint32_t group_count;
    uint32_t dev;
    std::vector<std::pair<VkPhysicalDevice, bool>> phys_dev_normal_found;
    std::vector<std::pair<VkPhysicalDevice, bool>> phys_dev_group_found;

    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        // Extension isn't present, just skip this test
        ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);
        std::cout << "Skipping EnumeratePhysicalDeviceGroupsKHR : TwoCall due to Instance lacking support"
                  << " for " << VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME << " extension\n";
        return;
    }

    // Get a pointer to the new vkEnumeratePhysicalDeviceGroupsKHR call
    PFN_vkEnumeratePhysicalDeviceGroupsKHR p_vkEnumeratePhysicalDeviceGroupsKHR =
        (PFN_vkEnumeratePhysicalDeviceGroupsKHR)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroupsKHR");

    // Setup the group information in preparation for the call
    uint32_t array_group_count = 30;
    std::unique_ptr<VkPhysicalDeviceGroupPropertiesKHR[]> phys_dev_groups(
        new VkPhysicalDeviceGroupPropertiesKHR[array_group_count]);
    for (group = 0; group < array_group_count; group++) {
        phys_dev_groups[group].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES_KHR;
        phys_dev_groups[group].pNext = nullptr;
        phys_dev_groups[group].physicalDeviceCount = 0;
        memset(phys_dev_groups[group].physicalDevices, 0, sizeof(VkPhysicalDevice) * VK_MAX_DEVICE_GROUP_SIZE_KHR);
        phys_dev_groups[group].subsetAllocation = VK_FALSE;
    }

    result = p_vkEnumeratePhysicalDeviceGroupsKHR(instance, &group_count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(group_count, 0u);
    ASSERT_LT(group_count, array_group_count);

    result = p_vkEnumeratePhysicalDeviceGroupsKHR(instance, &group_count, phys_dev_groups.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(group_count, 0u);
    ASSERT_LT(group_count, array_group_count);

    // Initialize the group physical device boolean pair array
    for (group = 0; group < group_count; group++) {
        for (dev = 0; dev < phys_dev_groups[group].physicalDeviceCount; dev++) {
            phys_dev_group_found.push_back(std::make_pair(phys_dev_groups[group].physicalDevices[dev], false));
        }
    }

    uint32_t phys_dev_count = 500;
    std::unique_ptr<VkPhysicalDevice[]> phys_devs(new VkPhysicalDevice[phys_dev_count]);
    result = vkEnumeratePhysicalDevices(instance, &phys_dev_count, phys_devs.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(phys_dev_count, 0u);

    // Initialize the normal physical device boolean pair array
    for (dev = 0; dev < phys_dev_count; dev++) {
        phys_dev_normal_found.push_back(std::make_pair(phys_devs[dev], false));
    }

    // Now, make sure we can find each normal and group item in the other list
    for (dev = 0; dev < phys_dev_count; dev++) {
        for (group = 0; group < phys_dev_group_found.size(); group++) {
            if (phys_dev_normal_found[dev].first == phys_dev_group_found[group].first) {
                phys_dev_normal_found[dev].second = true;
                phys_dev_group_found[group].second = true;
                break;
            }
        }
    }

    for (dev = 0; dev < phys_dev_count; dev++) {
        ASSERT_EQ(phys_dev_normal_found[dev].second, true);
    }
    for (dev = 0; dev < phys_dev_group_found.size(); dev++) {
        ASSERT_EQ(phys_dev_group_found[dev].second, true);
    }

    vkDestroyInstance(instance, nullptr);
}

// Used by run_loader_tests.sh to test for the expected usage of the EnumeratePhysicalDeviceGroupsKHR
// call if not enough numbers are provided for the final list.
TEST(EnumeratePhysicalDeviceGroupsKHR, TwoCallIncomplete) {
    VkInstance instance = VK_NULL_HANDLE;
    char const *const names[] = {VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME};
    auto const info = VK::InstanceCreateInfo().enabledExtensionCount(1).ppEnabledExtensionNames(names);
    uint32_t group;
    uint32_t group_count;

    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        // Extension isn't present, just skip this test
        ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);
        std::cout << "Skipping EnumeratePhysicalDeviceGroupsKHR : TwoCallIncomplete due to Instance lacking support"
                  << " for " << VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME << " extension\n";
        return;
    }

    // Get a pointer to the new vkEnumeratePhysicalDeviceGroupsKHR call
    PFN_vkEnumeratePhysicalDeviceGroupsKHR p_vkEnumeratePhysicalDeviceGroupsKHR =
        (PFN_vkEnumeratePhysicalDeviceGroupsKHR)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroupsKHR");

    // Setup the group information in preparation for the call
    uint32_t array_group_count = 30;
    std::unique_ptr<VkPhysicalDeviceGroupPropertiesKHR[]> phys_dev_groups(
        new VkPhysicalDeviceGroupPropertiesKHR[array_group_count]);
    for (group = 0; group < array_group_count; group++) {
        phys_dev_groups[group].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES_KHR;
        phys_dev_groups[group].pNext = nullptr;
        phys_dev_groups[group].physicalDeviceCount = 0;
        memset(phys_dev_groups[group].physicalDevices, 0, sizeof(VkPhysicalDevice) * VK_MAX_DEVICE_GROUP_SIZE_KHR);
        phys_dev_groups[group].subsetAllocation = VK_FALSE;
    }

    result = p_vkEnumeratePhysicalDeviceGroupsKHR(instance, &group_count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(group_count, 0u);
    ASSERT_LT(group_count, array_group_count);

    group_count -= 1;

    result = p_vkEnumeratePhysicalDeviceGroupsKHR(instance, &group_count, phys_dev_groups.get());
    ASSERT_EQ(result, VK_INCOMPLETE);

    vkDestroyInstance(instance, nullptr);
}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 0) {
        CommandLine::Initialize(argc, argv);
    }

    result = RUN_ALL_TESTS();

    return result;
}
