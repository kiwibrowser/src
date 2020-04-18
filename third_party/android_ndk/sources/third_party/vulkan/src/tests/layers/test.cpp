/*
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 */

#include <cassert>
#include <iostream>
#include <unordered_map>

#include "vk_dispatch_table_helper.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"

namespace test
{

struct layer_data {
    VkInstance instance;
    VkLayerInstanceDispatchTable *instance_dispatch_table;

    layer_data() : instance(VK_NULL_HANDLE), instance_dispatch_table(nullptr) {};
};

static std::unordered_map<void *, layer_data *> layer_data_map;

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
		VkInstance* pInstance)
{
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    assert(chain_info != nullptr);

    assert(chain_info->u.pLayerInfo != nullptr);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    assert(fpGetInstanceProcAddr != nullptr);

    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance) fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == nullptr)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    instance_data->instance = *pInstance;
    instance_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, instance_data->instance_dispatch_table, fpGetInstanceProcAddr);

    // Marker for testing.
    std::cout << "VK_LAYER_LUNARG_test: CreateInstance" << '\n';

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *instance_data = get_my_data_ptr(key, layer_data_map);
    instance_data->instance_dispatch_table->DestroyInstance(instance, pAllocator);

    delete instance_data->instance_dispatch_table;
    layer_data_map.erase(key);

    // Marker for testing.
    std::cout << "VK_LAYER_LUNARG_test: DestroyInstance" << '\n';
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    // Return the functions that are intercepted by this layer.
    static const struct
    {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] =
    {
        { "vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr) },
        { "vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance) },
        { "vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance) }
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++)
    {
        if (!strcmp(core_instance_commands[i].name, funcName))
        {
            return core_instance_commands[i].proc;
        }
    }

    // Only call down the chain for Vulkan commands that this layer does not intercept.
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = instance_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == nullptr)
    {
        return nullptr;
    }

    return pTable->GetInstanceProcAddr(instance, funcName);
}

}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    return test::GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char *funcName)
{
    return nullptr;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties)
{
    return VK_ERROR_LAYER_NOT_PRESENT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties)
{
    return VK_ERROR_LAYER_NOT_PRESENT;
}
