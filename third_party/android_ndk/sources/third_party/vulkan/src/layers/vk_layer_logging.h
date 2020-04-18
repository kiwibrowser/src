/* Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 *
 */

#ifndef LAYER_LOGGING_H
#define LAYER_LOGGING_H

#include "vk_loader_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_data.h"
#include "vk_layer_table.h"
#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include <cinttypes>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <unordered_map>
#include <vector>

typedef struct _debug_report_data {
    VkLayerDbgFunctionNode *debug_callback_list;
    VkLayerDbgFunctionNode *default_debug_callback_list;
    VkFlags active_flags;
    bool g_DEBUG_REPORT;
} debug_report_data;

template debug_report_data *get_my_data_ptr<debug_report_data>(void *data_key,
                                                               std::unordered_map<void *, debug_report_data *> &data_map);

// Forward Declarations
static inline bool debug_report_log_msg(const debug_report_data *debug_data, VkFlags msgFlags,
                                        VkDebugReportObjectTypeEXT objectType, uint64_t srcObject, size_t location, int32_t msgCode,
                                        const char *pLayerPrefix, const char *pMsg);

// Add a debug message callback node structure to the specified callback linked list
static inline void AddDebugMessageCallback(debug_report_data *debug_data, VkLayerDbgFunctionNode **list_head,
                                           VkLayerDbgFunctionNode *new_node) {

    new_node->pNext = *list_head;
    *list_head = new_node;
}

// Remove specified debug message callback node structure from the specified callback linked list
static inline void RemoveDebugMessageCallback(debug_report_data *debug_data, VkLayerDbgFunctionNode **list_head,
                                              VkDebugReportCallbackEXT callback) {
    VkLayerDbgFunctionNode *cur_callback = *list_head;
    VkLayerDbgFunctionNode *prev_callback = cur_callback;
    bool matched = false;

    debug_data->active_flags = 0;
    while (cur_callback) {
        if (cur_callback->msgCallback == callback) {
            matched = true;
            prev_callback->pNext = cur_callback->pNext;
            if (*list_head == cur_callback) {
                *list_head = cur_callback->pNext;
            }
            debug_report_log_msg(debug_data, VK_DEBUG_REPORT_DEBUG_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT,
                                 reinterpret_cast<uint64_t &>(cur_callback->msgCallback), 0, VK_DEBUG_REPORT_ERROR_CALLBACK_REF_EXT,
                                 "DebugReport", "Destroyed callback");
        } else {
            matched = false;
            debug_data->active_flags |= cur_callback->msgFlags;
        }
        prev_callback = cur_callback;
        cur_callback = cur_callback->pNext;
        if (matched) {
            free(prev_callback);
        }
    }
}

// Removes all debug callback function nodes from the specified callback linked lists and frees their resources
static inline void RemoveAllMessageCallbacks(debug_report_data *debug_data, VkLayerDbgFunctionNode **list_head) {
    VkLayerDbgFunctionNode *current_callback = *list_head;
    VkLayerDbgFunctionNode *prev_callback = current_callback;

    while (current_callback) {
        prev_callback = current_callback->pNext;
        debug_report_log_msg(debug_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT,
                             (uint64_t)current_callback->msgCallback, 0, VK_DEBUG_REPORT_ERROR_CALLBACK_REF_EXT, "DebugReport",
                             "Debug Report callbacks not removed before DestroyInstance");
        free(current_callback);
        current_callback = prev_callback;
    }
    *list_head = NULL;
}

// Utility function to handle reporting
static inline bool debug_report_log_msg(const debug_report_data *debug_data, VkFlags msgFlags,
                                        VkDebugReportObjectTypeEXT objectType, uint64_t srcObject, size_t location, int32_t msgCode,
                                        const char *pLayerPrefix, const char *pMsg) {
    bool bail = false;
    VkLayerDbgFunctionNode *pTrav = NULL;

    if (debug_data->debug_callback_list != NULL) {
        pTrav = debug_data->debug_callback_list;
    } else {
        pTrav = debug_data->default_debug_callback_list;
    }

    while (pTrav) {
        if (pTrav->msgFlags & msgFlags) {
            if (pTrav->pfnMsgCallback(msgFlags, objectType, srcObject, location, msgCode, pLayerPrefix, pMsg, pTrav->pUserData)) {
                bail = true;
            }
        }
        pTrav = pTrav->pNext;
    }

    return bail;
}

static inline debug_report_data *
debug_report_create_instance(VkLayerInstanceDispatchTable *table, VkInstance inst, uint32_t extension_count,
                             const char *const *ppEnabledExtensions) // layer or extension name to be enabled
{
    debug_report_data *debug_data = (debug_report_data *)malloc(sizeof(debug_report_data));
    if (!debug_data)
        return NULL;

    memset(debug_data, 0, sizeof(debug_report_data));
    for (uint32_t i = 0; i < extension_count; i++) {
        // TODO: Check other property fields
        if (strcmp(ppEnabledExtensions[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            debug_data->g_DEBUG_REPORT = true;
        }
    }
    return debug_data;
}

static inline void layer_debug_report_destroy_instance(debug_report_data *debug_data) {
    if (debug_data) {
        RemoveAllMessageCallbacks(debug_data, &debug_data->default_debug_callback_list);
        RemoveAllMessageCallbacks(debug_data, &debug_data->debug_callback_list);
        free(debug_data);
    }
}

static inline debug_report_data *layer_debug_report_create_device(debug_report_data *instance_debug_data, VkDevice device) {
    // DEBUG_REPORT shares data between Instance and Device,
    // so just return instance's data pointer
    return instance_debug_data;
}

static inline void layer_debug_report_destroy_device(VkDevice device) {
    // Nothing to do since we're using instance data record
}

static inline void layer_destroy_msg_callback(debug_report_data *debug_data, VkDebugReportCallbackEXT callback,
                                              const VkAllocationCallbacks *pAllocator) {
    RemoveDebugMessageCallback(debug_data, &debug_data->debug_callback_list, callback);
    RemoveDebugMessageCallback(debug_data, &debug_data->default_debug_callback_list, callback);
}

static inline VkResult layer_create_msg_callback(debug_report_data *debug_data, bool default_callback,
                                                 const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback) {
    VkLayerDbgFunctionNode *pNewDbgFuncNode = (VkLayerDbgFunctionNode *)malloc(sizeof(VkLayerDbgFunctionNode));
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    // Handle of 0 is logging_callback so use allocated Node address as unique handle
    if (!(*pCallback))
        *pCallback = (VkDebugReportCallbackEXT)pNewDbgFuncNode;
    pNewDbgFuncNode->msgCallback = *pCallback;
    pNewDbgFuncNode->pfnMsgCallback = pCreateInfo->pfnCallback;
    pNewDbgFuncNode->msgFlags = pCreateInfo->flags;
    pNewDbgFuncNode->pUserData = pCreateInfo->pUserData;

    if (default_callback) {
        AddDebugMessageCallback(debug_data, &debug_data->default_debug_callback_list, pNewDbgFuncNode);
    } else {
        AddDebugMessageCallback(debug_data, &debug_data->debug_callback_list, pNewDbgFuncNode);
    }
    debug_data->active_flags |= pCreateInfo->flags;

    debug_report_log_msg(debug_data, VK_DEBUG_REPORT_DEBUG_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT,
                         (uint64_t)*pCallback, 0, VK_DEBUG_REPORT_ERROR_CALLBACK_REF_EXT, "DebugReport", "Added callback");
    return VK_SUCCESS;
}

static inline PFN_vkVoidFunction debug_report_get_instance_proc_addr(debug_report_data *debug_data, const char *funcName) {
    if (!debug_data || !debug_data->g_DEBUG_REPORT) {
        return NULL;
    }

    if (!strcmp(funcName, "vkCreateDebugReportCallbackEXT")) {
        return (PFN_vkVoidFunction)vkCreateDebugReportCallbackEXT;
    }
    if (!strcmp(funcName, "vkDestroyDebugReportCallbackEXT")) {
        return (PFN_vkVoidFunction)vkDestroyDebugReportCallbackEXT;
    }
    if (!strcmp(funcName, "vkDebugReportMessageEXT")) {
        return (PFN_vkVoidFunction)vkDebugReportMessageEXT;
    }
    return NULL;
}

// This utility (called at vkCreateInstance() time), looks at a pNext chain.
// It counts any VkDebugReportCallbackCreateInfoEXT structs that it finds.  It
// then allocates an array that can hold that many structs, as well as that
// many VkDebugReportCallbackEXT handles.  It then copies each
// VkDebugReportCallbackCreateInfoEXT, and initializes each handle.
static VkResult layer_copy_tmp_callbacks(const void *pChain, uint32_t *num_callbacks, VkDebugReportCallbackCreateInfoEXT **infos,
                                         VkDebugReportCallbackEXT **callbacks) {
    uint32_t n = *num_callbacks = 0;

    const void *pNext = pChain;
    while (pNext) {
        // 1st, count the number VkDebugReportCallbackCreateInfoEXT:
        if (((VkDebugReportCallbackCreateInfoEXT *)pNext)->sType == VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT) {
            n++;
        }
        pNext = (void *)((VkDebugReportCallbackCreateInfoEXT *)pNext)->pNext;
    }
    if (n == 0) {
        return VK_SUCCESS;
    }

    // 2nd, allocate memory for each VkDebugReportCallbackCreateInfoEXT:
    VkDebugReportCallbackCreateInfoEXT *pInfos = *infos =
        ((VkDebugReportCallbackCreateInfoEXT *)malloc(n * sizeof(VkDebugReportCallbackCreateInfoEXT)));
    if (!pInfos) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    // 3rd, allocate memory for a unique handle for each callback:
    VkDebugReportCallbackEXT *pCallbacks = *callbacks = ((VkDebugReportCallbackEXT *)malloc(n * sizeof(VkDebugReportCallbackEXT)));
    if (!pCallbacks) {
        free(pInfos);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    // 4th, copy each VkDebugReportCallbackCreateInfoEXT for use by
    // vkDestroyInstance, and assign a unique handle to each callback (just
    // use the address of the copied VkDebugReportCallbackCreateInfoEXT):
    pNext = pChain;
    while (pNext) {
        if (((VkInstanceCreateInfo *)pNext)->sType == VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT) {
            memcpy(pInfos, pNext, sizeof(VkDebugReportCallbackCreateInfoEXT));
            *pCallbacks++ = (VkDebugReportCallbackEXT)pInfos++;
        }
        pNext = (void *)((VkInstanceCreateInfo *)pNext)->pNext;
    }

    *num_callbacks = n;
    return VK_SUCCESS;
}

// This utility frees the arrays allocated by layer_copy_tmp_callbacks()
static void layer_free_tmp_callbacks(VkDebugReportCallbackCreateInfoEXT *infos, VkDebugReportCallbackEXT *callbacks) {
    free(infos);
    free(callbacks);
}

// This utility enables all of the VkDebugReportCallbackCreateInfoEXT structs
// that were copied by layer_copy_tmp_callbacks()
static VkResult layer_enable_tmp_callbacks(debug_report_data *debug_data, uint32_t num_callbacks,
                                           VkDebugReportCallbackCreateInfoEXT *infos, VkDebugReportCallbackEXT *callbacks) {
    VkResult rtn = VK_SUCCESS;
    for (uint32_t i = 0; i < num_callbacks; i++) {
        rtn = layer_create_msg_callback(debug_data, false, &infos[i], NULL, &callbacks[i]);
        if (rtn != VK_SUCCESS) {
            for (uint32_t j = 0; j < i; j++) {
                layer_destroy_msg_callback(debug_data, callbacks[j], NULL);
            }
            return rtn;
        }
    }
    return rtn;
}

// This utility disables all of the VkDebugReportCallbackCreateInfoEXT structs
// that were copied by layer_copy_tmp_callbacks()
static void layer_disable_tmp_callbacks(debug_report_data *debug_data, uint32_t num_callbacks,
                                        VkDebugReportCallbackEXT *callbacks) {
    for (uint32_t i = 0; i < num_callbacks; i++) {
        layer_destroy_msg_callback(debug_data, callbacks[i], NULL);
    }
}

// Checks if the message will get logged.
// Allows layer to defer collecting & formating data if the
// message will be discarded.
static inline bool will_log_msg(debug_report_data *debug_data, VkFlags msgFlags) {
    if (!debug_data || !(debug_data->active_flags & msgFlags)) {
        // Message is not wanted
        return false;
    }

    return true;
}

#ifdef WIN32
static inline int vasprintf(char **strp, char const *fmt, va_list ap) {
    *strp = nullptr;
    int size = _vscprintf(fmt, ap);
    if (size >= 0) {
        *strp = (char *)malloc(size+1);
        if (!*strp) {
            return -1;
        }
        _vsnprintf(*strp, size+1, fmt, ap);
    }
    return size;
}
#endif

// Output log message via DEBUG_REPORT
// Takes format and variable arg list so that output string
// is only computed if a message needs to be logged
#ifndef WIN32
static inline bool log_msg(const debug_report_data *debug_data, VkFlags msgFlags, VkDebugReportObjectTypeEXT objectType,
                           uint64_t srcObject, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *format, ...)
    __attribute__((format(printf, 8, 9)));
#endif
static inline bool log_msg(const debug_report_data *debug_data, VkFlags msgFlags, VkDebugReportObjectTypeEXT objectType,
                           uint64_t srcObject, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *format,
                           ...) {
    if (!debug_data || !(debug_data->active_flags & msgFlags)) {
        // Message is not wanted
        return false;
    }

    va_list argptr;
    va_start(argptr, format);
    char *str;
    if (-1 == vasprintf(&str, format, argptr)) {
        // On failure, glibc vasprintf leaves str undefined
        str = nullptr;
    }
    va_end(argptr);
    bool result = debug_report_log_msg(debug_data, msgFlags, objectType, srcObject, location, msgCode, pLayerPrefix,
                                       str ? str : "Allocation failure");
    free(str);
    return result;
}

static inline VKAPI_ATTR VkBool32 VKAPI_CALL log_callback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                          size_t location, int32_t msgCode, const char *pLayerPrefix,
                                                          const char *pMsg, void *pUserData) {
    char msg_flags[30];

    print_msg_flags(msgFlags, msg_flags);

    fprintf((FILE *)pUserData, "%s(%s): object: 0x%" PRIx64 " type: %d location: %lu msgCode: %d: %s\n", pLayerPrefix, msg_flags,
            srcObject, objType, (unsigned long)location, msgCode, pMsg);
    fflush((FILE *)pUserData);

    return false;
}

static inline VKAPI_ATTR VkBool32 VKAPI_CALL win32_debug_output_msg(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
                                                                    uint64_t srcObject, size_t location, int32_t msgCode,
                                                                    const char *pLayerPrefix, const char *pMsg, void *pUserData) {
#ifdef WIN32
    char msg_flags[30];
    char buf[2048];

    print_msg_flags(msgFlags, msg_flags);
    _snprintf(buf, sizeof(buf) - 1,
              "%s (%s): object: 0x%" PRIxPTR " type: %d location: " PRINTF_SIZE_T_SPECIFIER " msgCode: %d: %s\n", pLayerPrefix,
              msg_flags, (size_t)srcObject, objType, location, msgCode, pMsg);

    OutputDebugString(buf);
#endif

    return false;
}

#endif // LAYER_LOGGING_H
