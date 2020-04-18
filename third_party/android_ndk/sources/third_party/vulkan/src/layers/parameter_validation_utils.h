/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
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
 * Author: Dustin Graves <dustin@lunarg.com>
 */

#ifndef PARAMETER_VALIDATION_UTILS_H
#define PARAMETER_VALIDATION_UTILS_H

#include <algorithm>
#include <cstdlib>
#include <string>

#include "vulkan/vulkan.h"
#include "vk_enum_string_helper.h"
#include "vk_layer_logging.h"
#include "vk_validation_error_messages.h"

#include "parameter_name.h"

#include "parameter_name.h"

namespace parameter_validation {

enum ErrorCode {
    NONE,                 // Used for INFO & other non-error messages
    INVALID_USAGE,        // The value of a parameter is not consistent
                          // with the valid usage criteria defined in
                          // the Vulkan specification.
    INVALID_STRUCT_STYPE, // The sType field of a Vulkan structure does
                          // not contain the value expected for a structure
                          // of that type.
    INVALID_STRUCT_PNEXT, // The pNext field of a Vulkan structure references
                          // a value that is not compatible with a structure of
                          // that type or is not NULL when a structure of that
                          // type has no compatible pNext values.
    REQUIRED_PARAMETER,   // A required parameter was specified as 0 or NULL.
    RESERVED_PARAMETER,   // A parameter reserved for future use was not
                          // specified as 0 or NULL.
    UNRECOGNIZED_VALUE,   // A Vulkan enumeration, VkFlags, or VkBool32 parameter
                          // contains a value that is not recognized as valid for
                          // that type.
    DEVICE_LIMIT,         // A specified parameter exceeds the limits returned
                          // by the physical device
    DEVICE_FEATURE,       // Use of a requested feature is not supported by
                          // the device
    FAILURE_RETURN_CODE,  // A Vulkan return code indicating a failure condition
                          // was encountered.
};

struct GenericHeader {
    VkStructureType sType;
    const void *pNext;
};

// Layer name string to be logged with validation messages.
const char LayerName[] = "ParameterValidation";

// Enables for display-related instance extensions
struct instance_extension_enables {
    bool wsi_enabled;
    bool xlib_enabled;
    bool xcb_enabled;
    bool wayland_enabled;
    bool mir_enabled;
    bool android_enabled;
    bool win32_enabled;
};

// String returned by string_VkStructureType for an unrecognized type.
const std::string UnsupportedStructureTypeString = "Unhandled VkStructureType";

// String returned by string_VkResult for an unrecognized type.
const std::string UnsupportedResultString = "Unhandled VkResult";

// The base value used when computing the offset for an enumeration token value that is added by an extension.
// When validating enumeration tokens, any value >= to this value is considered to be provided by an extension.
// See Appendix C.10 "Assigning Extension Token Values" from the Vulkan specification
const uint32_t ExtEnumBaseValue = 1000000000;

template <typename T> bool is_extension_added_token(T value) {
    return (std::abs(static_cast<int32_t>(value)) >= ExtEnumBaseValue);
}

// VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE token is a special case that was converted from a core token to an
// extension added token.  Its original value was intentionally preserved after the conversion, so it does not use
// the base value that other extension added tokens use, and it does not fall within the enum's begin/end range.
template <> bool is_extension_added_token(VkSamplerAddressMode value) {
    bool result = (std::abs(static_cast<int32_t>(value)) >= ExtEnumBaseValue);
    return (result || (value == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE));
}

/**
* Validate a minimum value.
*
* Verify that the specified value is greater than the specified lower bound.
*
* @param report_data debug_report_data object for routing validation messages.
* @param api_name Name of API call being validated.
* @param parameter_name Name of parameter being validated.
* @param value Value to validate.
* @param lower_bound Lower bound value to use for validation.
* @return Boolean value indicating that the call should be skipped.
*/
template <typename T>
bool ValidateGreaterThan(debug_report_data *report_data, const char *api_name, const ParameterName &parameter_name, T value,
                         T lower_bound) {
    bool skip_call = false;

    if (value <= lower_bound) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, LayerName,
                             "%s: parameter %s must be greater than %d", api_name, parameter_name.get_name().c_str(), lower_bound);
    }

    return skip_call;
}

/**
 * Validate a required pointer.
 *
 * Verify that a required pointer is not NULL.
 *
 * @param report_data debug_report_data object for routing validation messages.
 * @param apiName Name of API call being validated.
 * @param parameterName Name of parameter being validated.
 * @param value Pointer to validate.
 * @return Boolean value indicating that the call should be skipped.
 */
static bool validate_required_pointer(debug_report_data *report_data, const char *apiName, const ParameterName &parameterName,
                                      const void *value) {
    bool skip_call = false;

    if (value == NULL) {

        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                             REQUIRED_PARAMETER, LayerName, "%s: required parameter %s specified as NULL", apiName,
                             parameterName.get_name().c_str());
    }

    return skip_call;
}

/**
 * Validate array count and pointer to array.
 *
 * Verify that required count and array parameters are not 0 or NULL.  If the
 * count parameter is not optional, verify that it is not 0.  If the array
 * parameter is NULL, and it is not optional, verify that count is 0.
 *
 * @param report_data debug_report_data object for routing validation messages.
 * @param apiName Name of API call being validated.
 * @param countName Name of count parameter.
 * @param arrayName Name of array parameter.
 * @param count Number of elements in the array.
 * @param array Array to validate.
 * @param countRequired The 'count' parameter may not be 0 when true.
 * @param arrayRequired The 'array' parameter may not be NULL when true.
 * @return Boolean value indicating that the call should be skipped.
 */
template <typename T>
bool validate_array(debug_report_data *report_data, const char *apiName, const ParameterName &countName,
                    const ParameterName &arrayName, T count, const void *array, bool countRequired, bool arrayRequired) {
    bool skip_call = false;

    // Count parameters not tagged as optional cannot be 0
    if (countRequired && (count == 0)) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            REQUIRED_PARAMETER, LayerName, "%s: parameter %s must be greater than 0", apiName,
                            countName.get_name().c_str());
    }

    // Array parameters not tagged as optional cannot be NULL, unless the count is 0
    if ((array == NULL) && arrayRequired && (count != 0)) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            REQUIRED_PARAMETER, LayerName, "%s: required parameter %s specified as NULL", apiName,
                            arrayName.get_name().c_str());
    }

    return skip_call;
}

/**
* Validate pointer to array count and pointer to array.
*
* Verify that required count and array parameters are not NULL.  If count
* is not NULL and its value is not optional, verify that it is not 0.  If the
* array parameter is NULL, and it is not optional, verify that count is 0.
* The array parameter will typically be optional for this case (where count is
* a pointer), allowing the caller to retrieve the available count.
*
* @param report_data debug_report_data object for routing validation messages.
* @param apiName Name of API call being validated.
* @param countName Name of count parameter.
* @param arrayName Name of array parameter.
* @param count Pointer to the number of elements in the array.
* @param array Array to validate.
* @param countPtrRequired The 'count' parameter may not be NULL when true.
* @param countValueRequired The '*count' value may not be 0 when true.
* @param arrayRequired The 'array' parameter may not be NULL when true.
* @return Boolean value indicating that the call should be skipped.
*/
template <typename T>
bool validate_array(debug_report_data *report_data, const char *apiName, const ParameterName &countName,
                    const ParameterName &arrayName, const T *count, const void *array, bool countPtrRequired,
                    bool countValueRequired, bool arrayRequired) {
    bool skip_call = false;

    if (count == NULL) {
        if (countPtrRequired) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                 REQUIRED_PARAMETER, LayerName, "%s: required parameter %s specified as NULL", apiName,
                                 countName.get_name().c_str());
        }
    }
    else {
        skip_call |= validate_array(report_data, apiName, countName, arrayName, (*count), array, countValueRequired, arrayRequired);
    }

    return skip_call;
}

/**
 * Validate a pointer to a Vulkan structure.
 *
 * Verify that a required pointer to a structure is not NULL.  If the pointer is
 * not NULL, verify that each structure's sType field is set to the correct
 * VkStructureType value.
 *
 * @param report_data debug_report_data object for routing validation messages.
 * @param apiName Name of API call being validated.
 * @param parameterName Name of struct parameter being validated.
 * @param sTypeName Name of expected VkStructureType value.
 * @param value Pointer to the struct to validate.
 * @param sType VkStructureType for structure validation.
 * @param required The parameter may not be NULL when true.
 * @return Boolean value indicating that the call should be skipped.
 */
template <typename T>
bool validate_struct_type(debug_report_data *report_data, const char *apiName, const ParameterName &parameterName,
                          const char *sTypeName, const T *value, VkStructureType sType, bool required) {
    bool skip_call = false;

    if (value == NULL) {
        if (required) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                 REQUIRED_PARAMETER, LayerName, "%s: required parameter %s specified as NULL", apiName,
                                 parameterName.get_name().c_str());
        }
    } else if (value->sType != sType) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                             INVALID_STRUCT_STYPE, LayerName, "%s: parameter %s->sType must be %s", apiName,
                             parameterName.get_name().c_str(), sTypeName);
    }

    return skip_call;
}

/**
 * Validate an array of Vulkan structures.
 *
 * Verify that required count and array parameters are not NULL.  If count
 * is not NULL and its value is not optional, verify that it is not 0.
 * If the array contains 1 or more structures, verify that each structure's
 * sType field is set to the correct VkStructureType value.
 *
 * @param report_data debug_report_data object for routing validation messages.
 * @param apiName Name of API call being validated.
 * @param countName Name of count parameter.
 * @param arrayName Name of array parameter.
 * @param sTypeName Name of expected VkStructureType value.
 * @param count Pointer to the number of elements in the array.
 * @param array Array to validate.
 * @param sType VkStructureType for structure validation.
 * @param countPtrRequired The 'count' parameter may not be NULL when true.
 * @param countValueRequired The '*count' value may not be 0 when true.
 * @param arrayRequired The 'array' parameter may not be NULL when true.
 * @return Boolean value indicating that the call should be skipped.
 */
template <typename T>
bool validate_struct_type_array(debug_report_data *report_data, const char *apiName, const ParameterName &countName,
                                const ParameterName &arrayName, const char *sTypeName, const uint32_t *count, const T *array,
                                VkStructureType sType, bool countPtrRequired, bool countValueRequired, bool arrayRequired) {
    bool skip_call = false;

    if (count == NULL) {
        if (countPtrRequired) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                 REQUIRED_PARAMETER, LayerName, "%s: required parameter %s specified as NULL", apiName,
                                 countName.get_name().c_str());
        }
    } else {
        skip_call |= validate_struct_type_array(report_data, apiName, countName, arrayName, sTypeName, (*count), array, sType,
                                                countValueRequired, arrayRequired);
    }

    return skip_call;
}

/**
 * Validate an array of Vulkan structures
 *
 * Verify that required count and array parameters are not 0 or NULL.  If
 * the array contains 1 or more structures, verify that each structure's
 * sType field is set to the correct VkStructureType value.
 *
 * @param report_data debug_report_data object for routing validation messages.
 * @param apiName Name of API call being validated.
 * @param countName Name of count parameter.
 * @param arrayName Name of array parameter.
 * @param sTypeName Name of expected VkStructureType value.
 * @param count Number of elements in the array.
 * @param array Array to validate.
 * @param sType VkStructureType for structure validation.
 * @param countRequired The 'count' parameter may not be 0 when true.
 * @param arrayRequired The 'array' parameter may not be NULL when true.
 * @return Boolean value indicating that the call should be skipped.
 */
template <typename T>
bool validate_struct_type_array(debug_report_data *report_data, const char *apiName, const ParameterName &countName,
                                const ParameterName &arrayName, const char *sTypeName, uint32_t count, const T *array,
                                VkStructureType sType, bool countRequired, bool arrayRequired) {
    bool skip_call = false;

    if ((count == 0) || (array == NULL)) {
        skip_call |= validate_array(report_data, apiName, countName, arrayName, count, array, countRequired, arrayRequired);
    } else {
        // Verify that all structs in the array have the correct type
        for (uint32_t i = 0; i < count; ++i) {
            if (array[i].sType != sType) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, INVALID_STRUCT_STYPE, LayerName, "%s: parameter %s[%d].sType must be %s", apiName,
                                     arrayName.get_name().c_str(), i, sTypeName);
            }
        }
    }

    return skip_call;
}

/**
* Validate a Vulkan handle.
*
* Verify that the specified handle is not VK_NULL_HANDLE.
*
* @param report_data debug_report_data object for routing validation messages.
* @param api_name Name of API call being validated.
* @param parameter_name Name of struct parameter being validated.
* @param value Handle to validate.
* @return Boolean value indicating that the call should be skipped.
*/
template <typename T>
bool validate_required_handle(debug_report_data *report_data, const char *api_name, const ParameterName &parameter_name, T value) {
    bool skip_call = false;

    if (value == VK_NULL_HANDLE) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                             REQUIRED_PARAMETER, LayerName, "%s: required parameter %s specified as VK_NULL_HANDLE", api_name,
                             parameter_name.get_name().c_str());
    }

    return skip_call;
}

/**
* Validate an array of Vulkan handles.
*
* Verify that required count and array parameters are not NULL.  If count
* is not NULL and its value is not optional, verify that it is not 0.
* If the array contains 1 or more handles, verify that no handle is set to
* VK_NULL_HANDLE.
*
* @note This function is only intended to validate arrays of handles when none
*       of the handles are allowed to be VK_NULL_HANDLE.  For arrays of handles
*       that are allowed to contain VK_NULL_HANDLE, use validate_array() instead.
*
* @param report_data debug_report_data object for routing validation messages.
* @param api_name Name of API call being validated.
* @param count_name Name of count parameter.
* @param array_name Name of array parameter.
* @param count Number of elements in the array.
* @param array Array to validate.
* @param count_required The 'count' parameter may not be 0 when true.
* @param array_required The 'array' parameter may not be NULL when true.
* @return Boolean value indicating that the call should be skipped.
*/
template <typename T>
bool validate_handle_array(debug_report_data *report_data, const char *api_name, const ParameterName &count_name,
                           const ParameterName &array_name, uint32_t count, const T *array, bool count_required,
                           bool array_required) {
    bool skip_call = false;

    if ((count == 0) || (array == NULL)) {
        skip_call |= validate_array(report_data, api_name, count_name, array_name, count, array, count_required, array_required);
    } else {
        // Verify that no handles in the array are VK_NULL_HANDLE
        for (uint32_t i = 0; i < count; ++i) {
            if (array[i] == VK_NULL_HANDLE) {
                skip_call |=
                    log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            REQUIRED_PARAMETER, LayerName, "%s: required parameter %s[%d] specified as VK_NULL_HANDLE", api_name,
                            array_name.get_name().c_str(), i);
            }
        }
    }

    return skip_call;
}

/**
 * Validate string array count and content.
 *
 * Verify that required count and array parameters are not 0 or NULL.  If the
 * count parameter is not optional, verify that it is not 0.  If the array
 * parameter is NULL, and it is not optional, verify that count is 0.  If the
 * array parameter is not NULL, verify that none of the strings are NULL.
 *
 * @param report_data debug_report_data object for routing validation messages.
 * @param apiName Name of API call being validated.
 * @param countName Name of count parameter.
 * @param arrayName Name of array parameter.
 * @param count Number of strings in the array.
 * @param array Array of strings to validate.
 * @param countRequired The 'count' parameter may not be 0 when true.
 * @param arrayRequired The 'array' parameter may not be NULL when true.
 * @return Boolean value indicating that the call should be skipped.
 */
static bool validate_string_array(debug_report_data *report_data, const char *apiName, const ParameterName &countName,
                                  const ParameterName &arrayName, uint32_t count, const char *const *array, bool countRequired,
                                  bool arrayRequired) {
    bool skip_call = false;

    if ((count == 0) || (array == NULL)) {
        skip_call |= validate_array(report_data, apiName, countName, arrayName, count, array, countRequired, arrayRequired);
    } else {
        // Verify that strings in the array are not NULL
        for (uint32_t i = 0; i < count; ++i) {
            if (array[i] == NULL) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, REQUIRED_PARAMETER, LayerName, "%s: required parameter %s[%d] specified as NULL",
                                     apiName, arrayName.get_name().c_str(), i);
            }
        }
    }

    return skip_call;
}

/**
 * Validate a structure's pNext member.
 *
 * Verify that the specified pNext value points to the head of a list of
 * allowed extension structures.  If no extension structures are allowed,
 * verify that pNext is null.
 *
 * @param report_data debug_report_data object for routing validation messages.
 * @param api_name Name of API call being validated.
 * @param parameter_name Name of parameter being validated.
 * @param allowed_struct_names Names of allowed structs.
 * @param next Pointer to validate.
 * @param allowed_type_count Total number of allowed structure types.
 * @param allowed_types Array of strcuture types allowed for pNext.
 * @param header_version Version of header defining the pNext validation rules.
 * @return Boolean value indicating that the call should be skipped.
 */
static bool validate_struct_pnext(debug_report_data *report_data, const char *api_name, const ParameterName &parameter_name,
                                  const char *allowed_struct_names, const void *next, size_t allowed_type_count,
                                  const VkStructureType *allowed_types, uint32_t header_version) {
    bool skip_call = false;
    const char disclaimer[] = "This warning is based on the Valid Usage documentation for version %d of the Vulkan header.  It "
                              "is possible that you are using a struct from a private extension or an extension that was added "
                              "to a later version of the Vulkan header, in which case your use of %s is perfectly valid but "
                              "is not guaranteed to work correctly with validation enabled";

    if (next != NULL) {
        if (allowed_type_count == 0) {
            std::string message = "%s: value of %s must be NULL.  ";
            message += disclaimer;
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                 INVALID_STRUCT_PNEXT, LayerName, message.c_str(), api_name, parameter_name.get_name().c_str(),
                                 header_version, parameter_name.get_name().c_str());
        } else {
            const VkStructureType *start = allowed_types;
            const VkStructureType *end = allowed_types + allowed_type_count;
            const GenericHeader *current = reinterpret_cast<const GenericHeader *>(next);

            while (current != NULL) {
                if (std::find(start, end, current->sType) == end) {
                    std::string type_name = string_VkStructureType(current->sType);

                    if (type_name == UnsupportedStructureTypeString) {
                        std::string message = "%s: %s chain includes a structure with unexpected VkStructureType (%d); Allowed "
                                              "structures are [%s].  ";
                        message += disclaimer;
                        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                                             0, __LINE__, INVALID_STRUCT_PNEXT, LayerName, message.c_str(), api_name,
                                             parameter_name.get_name().c_str(), current->sType, allowed_struct_names,
                                             header_version, parameter_name.get_name().c_str());
                    } else {
                        std::string message =
                            "%s: %s chain includes a structure with unexpected VkStructureType %s; Allowed structures are [%s].  ";
                        message += disclaimer;
                        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                                             0, __LINE__, INVALID_STRUCT_PNEXT, LayerName, message.c_str(), api_name,
                                             parameter_name.get_name().c_str(), type_name.c_str(), allowed_struct_names,
                                             header_version, parameter_name.get_name().c_str());
                    }
                }

                current = reinterpret_cast<const GenericHeader *>(current->pNext);
            }
        }
    }

    return skip_call;
}

/**
* Validate a VkBool32 value.
*
* Generate a warning if a VkBool32 value is neither VK_TRUE nor VK_FALSE.
*
* @param report_data debug_report_data object for routing validation messages.
* @param apiName Name of API call being validated.
* @param parameterName Name of parameter being validated.
* @param value Boolean value to validate.
* @return Boolean value indicating that the call should be skipped.
*/
static bool validate_bool32(debug_report_data *report_data, const char *apiName, const ParameterName &parameterName,
                            VkBool32 value) {
    bool skip_call = false;

    if ((value != VK_TRUE) && (value != VK_FALSE)) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                             UNRECOGNIZED_VALUE, LayerName, "%s: value of %s (%d) is neither VK_TRUE nor VK_FALSE", apiName,
                             parameterName.get_name().c_str(), value);
    }

    return skip_call;
}

/**
* Validate a Vulkan enumeration value.
*
* Generate a warning if an enumeration token value does not fall within the core enumeration
* begin and end token values, and was not added to the enumeration by an extension.  Extension
* provided enumerations use the equation specified in Appendix C.10 of the Vulkan specification,
* with 1,000,000,000 as the base token value.
*
* @note This function does not expect to process enumerations defining bitmask flag bits.
*
* @param report_data debug_report_data object for routing validation messages.
* @param apiName Name of API call being validated.
* @param parameterName Name of parameter being validated.
* @param enumName Name of the enumeration being validated.
* @param begin The begin range value for the enumeration.
* @param end The end range value for the enumeration.
* @param value Enumeration value to validate.
* @return Boolean value indicating that the call should be skipped.
*/
template <typename T>
bool validate_ranged_enum(debug_report_data *report_data, const char *apiName, const ParameterName &parameterName,
                          const char *enumName, T begin, T end, T value) {
    bool skip_call = false;

    if (((value < begin) || (value > end)) && !is_extension_added_token(value)) {
        skip_call |=
            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName, "%s: value of %s (%d) does not fall within the begin..end range of the core %s "
                                                   "enumeration tokens and is not an extension added token",
                    apiName, parameterName.get_name().c_str(), value, enumName);
    }

    return skip_call;
}

/**
* Validate an array of Vulkan enumeration value.
*
* Process all enumeration token values in the specified array and generate a warning if a value
* does not fall within the core enumeration begin and end token values, and was not added to
* the enumeration by an extension.  Extension provided enumerations use the equation specified
* in Appendix C.10 of the Vulkan specification, with 1,000,000,000 as the base token value.
*
* @note This function does not expect to process enumerations defining bitmask flag bits.
*
* @param report_data debug_report_data object for routing validation messages.
* @param apiName Name of API call being validated.
* @param countName Name of count parameter.
* @param arrayName Name of array parameter.
* @param enumName Name of the enumeration being validated.
* @param begin The begin range value for the enumeration.
* @param end The end range value for the enumeration.
* @param count Number of enumeration values in the array.
* @param array Array of enumeration values to validate.
* @param countRequired The 'count' parameter may not be 0 when true.
* @param arrayRequired The 'array' parameter may not be NULL when true.
* @return Boolean value indicating that the call should be skipped.
*/
template <typename T>
static bool validate_ranged_enum_array(debug_report_data *report_data, const char *apiName, const ParameterName &countName,
                                       const ParameterName &arrayName, const char *enumName, T begin, T end, uint32_t count,
                                       const T *array, bool countRequired, bool arrayRequired) {
    bool skip_call = false;

    if ((count == 0) || (array == NULL)) {
        skip_call |= validate_array(report_data, apiName, countName, arrayName, count, array, countRequired, arrayRequired);
    } else {
        for (uint32_t i = 0; i < count; ++i) {
            if (((array[i] < begin) || (array[i] > end)) && !is_extension_added_token(array[i])) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, UNRECOGNIZED_VALUE, LayerName,
                                     "%s: value of %s[%d] (%d) does not fall within the begin..end range of the core %s "
                                     "enumeration tokens and is not an extension added token",
                                     apiName, arrayName.get_name().c_str(), i, array[i], enumName);
            }
        }
    }

    return skip_call;
}

/**
* Verify that a reserved VkFlags value is zero.
*
* Verify that the specified value is zero, to check VkFlags values that are reserved for
* future use.
*
* @param report_data debug_report_data object for routing validation messages.
* @param api_name Name of API call being validated.
* @param parameter_name Name of parameter being validated.
* @param value Value to validate.
* @return Boolean value indicating that the call should be skipped.
*/
static bool validate_reserved_flags(debug_report_data *report_data, const char *api_name, const ParameterName &parameter_name,
                                    VkFlags value) {
    bool skip_call = false;

    if (value != 0) {
        skip_call |=
            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    RESERVED_PARAMETER, LayerName, "%s: parameter %s must be 0", api_name, parameter_name.get_name().c_str());
    }

    return skip_call;
}

/**
* Validate a Vulkan bitmask value.
*
* Generate a warning if a value with a VkFlags derived type does not contain valid flag bits
* for that type.
*
* @param report_data debug_report_data object for routing validation messages.
* @param api_name Name of API call being validated.
* @param parameter_name Name of parameter being validated.
* @param flag_bits_name Name of the VkFlags type being validated.
* @param all_flags A bit mask combining all valid flag bits for the VkFlags type being validated.
* @param value VkFlags value to validate.
* @param flags_required The 'value' parameter may not be 0 when true.
* @return Boolean value indicating that the call should be skipped.
*/
static bool validate_flags(debug_report_data *report_data, const char *api_name, const ParameterName &parameter_name,
                           const char *flag_bits_name, VkFlags all_flags, VkFlags value, bool flags_required) {
    bool skip_call = false;

    if (value == 0) {
        if (flags_required) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                 REQUIRED_PARAMETER, LayerName, "%s: value of %s must not be 0", api_name,
                                 parameter_name.get_name().c_str());
        }
    } else if ((value & (~all_flags)) != 0) {
        skip_call |=
            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName, "%s: value of %s contains flag bits that are not recognized members of %s",
                    api_name, parameter_name.get_name().c_str(), flag_bits_name);
    }

    return skip_call;
}

/**
* Validate an array of Vulkan bitmask values.
*
* Generate a warning if a value with a VkFlags derived type does not contain valid flag bits
* for that type.
*
* @param report_data debug_report_data object for routing validation messages.
* @param api_name Name of API call being validated.
* @param count_name Name of parameter being validated.
* @param array_name Name of parameter being validated.
* @param flag_bits_name Name of the VkFlags type being validated.
* @param all_flags A bitmask combining all valid flag bits for the VkFlags type being validated.
* @param count Number of VkFlags values in the array.
* @param array Array of VkFlags value to validate.
* @param count_required The 'count' parameter may not be 0 when true.
* @param array_required The 'array' parameter may not be NULL when true.
* @return Boolean value indicating that the call should be skipped.
*/
static bool validate_flags_array(debug_report_data *report_data, const char *api_name, const ParameterName &count_name,
                                 const ParameterName &array_name, const char *flag_bits_name, VkFlags all_flags, uint32_t count,
                                 const VkFlags *array, bool count_required, bool array_required) {
    bool skip_call = false;

    if ((count == 0) || (array == NULL)) {
        skip_call |= validate_array(report_data, api_name, count_name, array_name, count, array, count_required, array_required);
    } else {
        // Verify that all VkFlags values in the array
        for (uint32_t i = 0; i < count; ++i) {
            if (array[i] == 0) {
                // Current XML registry logic for validity generation uses the array parameter's optional tag to determine if
                // elements in the array are allowed be 0
                if (array_required) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, REQUIRED_PARAMETER, LayerName, "%s: value of %s[%d] must not be 0", api_name,
                                         array_name.get_name().c_str(), i);
                }
            } else if ((array[i] & (~all_flags)) != 0) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, UNRECOGNIZED_VALUE, LayerName,
                                     "%s: value of %s[%d] contains flag bits that are not recognized members of %s", api_name,
                                     array_name.get_name().c_str(), i, flag_bits_name);
            }
        }
    }

    return skip_call;
}

/**
* Get VkResult code description.
*
* Returns a string describing the specified VkResult code.  The description is based on the language in the Vulkan API
* specification.
*
* @param value VkResult code to process.
* @return String describing the specified VkResult code.
*/
static std::string get_result_description(VkResult result) {
    // clang-format off
    switch (result) {
        case VK_SUCCESS:                        return "a command completed successfully";
        case VK_NOT_READY:                      return "a fence or query has not yet completed";
        case VK_TIMEOUT:                        return "a wait operation has not completed in the specified time";
        case VK_EVENT_SET:                      return "an event is signaled";
        case VK_EVENT_RESET:                    return "an event is unsignalled";
        case VK_INCOMPLETE:                     return "a return array was too small for the result";
        case VK_ERROR_OUT_OF_HOST_MEMORY:       return "a host memory allocation has failed";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:     return "a device memory allocation has failed";
        case VK_ERROR_INITIALIZATION_FAILED:    return "initialization of an object has failed";
        case VK_ERROR_DEVICE_LOST:              return "the logical device has been lost";
        case VK_ERROR_MEMORY_MAP_FAILED:        return "mapping of a memory object has failed";
        case VK_ERROR_LAYER_NOT_PRESENT:        return "the specified layer does not exist";
        case VK_ERROR_EXTENSION_NOT_PRESENT:    return "the specified extension does not exist";
        case VK_ERROR_FEATURE_NOT_PRESENT:      return "the requested feature is not available on this device";
        case VK_ERROR_INCOMPATIBLE_DRIVER:      return "a Vulkan driver could not be found";
        case VK_ERROR_TOO_MANY_OBJECTS:         return "too many objects of the type have already been created";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:     return "the requested format is not supported on this device";
        case VK_ERROR_SURFACE_LOST_KHR:         return "a surface is no longer available";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "the requested window is already connected to another "
                                                       "VkSurfaceKHR object, or some other non-Vulkan surface object";
        case VK_SUBOPTIMAL_KHR:                 return "an image became available, and the swapchain no longer "
                                                       "matches the surface properties exactly, but can still be used to "
                                                       "present to the surface successfully.";
        case VK_ERROR_OUT_OF_DATE_KHR:          return "a surface has changed in such a way that it is no "
                                                       "longer compatible with the swapchain";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "the display used by a swapchain does not use the same "
                                                       "presentable image layout, or is incompatible in a way that prevents "
                                                       "sharing an image";
        case VK_ERROR_VALIDATION_FAILED_EXT:    return "API validation has detected an invalid use of the API";
        case VK_ERROR_INVALID_SHADER_NV:        return "one or more shaders failed to compile or link";
        default:                                return "an error has occurred";
    };
    // clang-format on
}

/**
* Validate return code.
*
* Print a message describing the reason for failure when an error code is returned.
*
* @param report_data debug_report_data object for routing validation messages.
* @param apiName Name of API call being validated.
* @param value VkResult value to validate.
*/
static void validate_result(debug_report_data *report_data, const char *apiName, VkResult result) {
    if (result < 0 && result != VK_ERROR_VALIDATION_FAILED_EXT) {
        std::string resultName = string_VkResult(result);

        if (resultName == UnsupportedResultString) {
            // Unrecognized result code
            log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    FAILURE_RETURN_CODE, LayerName, "%s: returned a result code indicating that an error has occurred", apiName);
        } else {
            std::string resultDesc = get_result_description(result);
            log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    FAILURE_RETURN_CODE, LayerName, "%s: returned %s, indicating that %s", apiName, resultName.c_str(),
                    resultDesc.c_str());
        }
    }
}

} // namespace parameter_validation

#endif // PARAMETER_VALIDATION_UTILS_H
