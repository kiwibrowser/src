#pragma once

#ifdef _WIN32

#pragma warning( disable : 4065 )

#endif

#include <vulkan/vulkan.h>


static inline const char* string_VkAccessFlagBits(VkAccessFlagBits input_value)
{
    switch ((VkAccessFlagBits)input_value)
    {
        case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:
            return "VK_ACCESS_COLOR_ATTACHMENT_READ_BIT";
        case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
            return "VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT";
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
            return "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT";
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
            return "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT";
        case VK_ACCESS_HOST_READ_BIT:
            return "VK_ACCESS_HOST_READ_BIT";
        case VK_ACCESS_HOST_WRITE_BIT:
            return "VK_ACCESS_HOST_WRITE_BIT";
        case VK_ACCESS_INDEX_READ_BIT:
            return "VK_ACCESS_INDEX_READ_BIT";
        case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
            return "VK_ACCESS_INDIRECT_COMMAND_READ_BIT";
        case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:
            return "VK_ACCESS_INPUT_ATTACHMENT_READ_BIT";
        case VK_ACCESS_MEMORY_READ_BIT:
            return "VK_ACCESS_MEMORY_READ_BIT";
        case VK_ACCESS_MEMORY_WRITE_BIT:
            return "VK_ACCESS_MEMORY_WRITE_BIT";
        case VK_ACCESS_SHADER_READ_BIT:
            return "VK_ACCESS_SHADER_READ_BIT";
        case VK_ACCESS_SHADER_WRITE_BIT:
            return "VK_ACCESS_SHADER_WRITE_BIT";
        case VK_ACCESS_TRANSFER_READ_BIT:
            return "VK_ACCESS_TRANSFER_READ_BIT";
        case VK_ACCESS_TRANSFER_WRITE_BIT:
            return "VK_ACCESS_TRANSFER_WRITE_BIT";
        case VK_ACCESS_UNIFORM_READ_BIT:
            return "VK_ACCESS_UNIFORM_READ_BIT";
        case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:
            return "VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT";
        default:
            return "Unhandled VkAccessFlagBits";
    }
}


static inline const char* string_VkAttachmentDescriptionFlagBits(VkAttachmentDescriptionFlagBits input_value)
{
    switch ((VkAttachmentDescriptionFlagBits)input_value)
    {
        case VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT:
            return "VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT";
        default:
            return "Unhandled VkAttachmentDescriptionFlagBits";
    }
}


static inline const char* string_VkAttachmentLoadOp(VkAttachmentLoadOp input_value)
{
    switch ((VkAttachmentLoadOp)input_value)
    {
        case VK_ATTACHMENT_LOAD_OP_CLEAR:
            return "VK_ATTACHMENT_LOAD_OP_CLEAR";
        case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
            return "VK_ATTACHMENT_LOAD_OP_DONT_CARE";
        case VK_ATTACHMENT_LOAD_OP_LOAD:
            return "VK_ATTACHMENT_LOAD_OP_LOAD";
        default:
            return "Unhandled VkAttachmentLoadOp";
    }
}


static inline const char* string_VkAttachmentStoreOp(VkAttachmentStoreOp input_value)
{
    switch ((VkAttachmentStoreOp)input_value)
    {
        case VK_ATTACHMENT_STORE_OP_DONT_CARE:
            return "VK_ATTACHMENT_STORE_OP_DONT_CARE";
        case VK_ATTACHMENT_STORE_OP_STORE:
            return "VK_ATTACHMENT_STORE_OP_STORE";
        default:
            return "Unhandled VkAttachmentStoreOp";
    }
}


static inline const char* string_VkBlendFactor(VkBlendFactor input_value)
{
    switch ((VkBlendFactor)input_value)
    {
        case VK_BLEND_FACTOR_CONSTANT_ALPHA:
            return "VK_BLEND_FACTOR_CONSTANT_ALPHA";
        case VK_BLEND_FACTOR_CONSTANT_COLOR:
            return "VK_BLEND_FACTOR_CONSTANT_COLOR";
        case VK_BLEND_FACTOR_DST_ALPHA:
            return "VK_BLEND_FACTOR_DST_ALPHA";
        case VK_BLEND_FACTOR_DST_COLOR:
            return "VK_BLEND_FACTOR_DST_COLOR";
        case VK_BLEND_FACTOR_ONE:
            return "VK_BLEND_FACTOR_ONE";
        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
            return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA";
        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
            return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR";
        case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
            return "VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA";
        case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
            return "VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
            return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
            return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
            return "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA";
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
            return "VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR";
        case VK_BLEND_FACTOR_SRC1_ALPHA:
            return "VK_BLEND_FACTOR_SRC1_ALPHA";
        case VK_BLEND_FACTOR_SRC1_COLOR:
            return "VK_BLEND_FACTOR_SRC1_COLOR";
        case VK_BLEND_FACTOR_SRC_ALPHA:
            return "VK_BLEND_FACTOR_SRC_ALPHA";
        case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
            return "VK_BLEND_FACTOR_SRC_ALPHA_SATURATE";
        case VK_BLEND_FACTOR_SRC_COLOR:
            return "VK_BLEND_FACTOR_SRC_COLOR";
        case VK_BLEND_FACTOR_ZERO:
            return "VK_BLEND_FACTOR_ZERO";
        default:
            return "Unhandled VkBlendFactor";
    }
}


static inline const char* string_VkBlendOp(VkBlendOp input_value)
{
    switch ((VkBlendOp)input_value)
    {
        case VK_BLEND_OP_ADD:
            return "VK_BLEND_OP_ADD";
        case VK_BLEND_OP_MAX:
            return "VK_BLEND_OP_MAX";
        case VK_BLEND_OP_MIN:
            return "VK_BLEND_OP_MIN";
        case VK_BLEND_OP_REVERSE_SUBTRACT:
            return "VK_BLEND_OP_REVERSE_SUBTRACT";
        case VK_BLEND_OP_SUBTRACT:
            return "VK_BLEND_OP_SUBTRACT";
        default:
            return "Unhandled VkBlendOp";
    }
}


static inline const char* string_VkBorderColor(VkBorderColor input_value)
{
    switch ((VkBorderColor)input_value)
    {
        case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
            return "VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK";
        case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
            return "VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE";
        case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
            return "VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK";
        case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
            return "VK_BORDER_COLOR_INT_OPAQUE_BLACK";
        case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
            return "VK_BORDER_COLOR_INT_OPAQUE_WHITE";
        case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
            return "VK_BORDER_COLOR_INT_TRANSPARENT_BLACK";
        default:
            return "Unhandled VkBorderColor";
    }
}


static inline const char* string_VkBufferCreateFlagBits(VkBufferCreateFlagBits input_value)
{
    switch ((VkBufferCreateFlagBits)input_value)
    {
        case VK_BUFFER_CREATE_SPARSE_ALIASED_BIT:
            return "VK_BUFFER_CREATE_SPARSE_ALIASED_BIT";
        case VK_BUFFER_CREATE_SPARSE_BINDING_BIT:
            return "VK_BUFFER_CREATE_SPARSE_BINDING_BIT";
        case VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT:
            return "VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT";
        default:
            return "Unhandled VkBufferCreateFlagBits";
    }
}


static inline const char* string_VkBufferUsageFlagBits(VkBufferUsageFlagBits input_value)
{
    switch ((VkBufferUsageFlagBits)input_value)
    {
        case VK_BUFFER_USAGE_INDEX_BUFFER_BIT:
            return "VK_BUFFER_USAGE_INDEX_BUFFER_BIT";
        case VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT:
            return "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT";
        case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT:
            return "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT";
        case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT:
            return "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT";
        case VK_BUFFER_USAGE_TRANSFER_DST_BIT:
            return "VK_BUFFER_USAGE_TRANSFER_DST_BIT";
        case VK_BUFFER_USAGE_TRANSFER_SRC_BIT:
            return "VK_BUFFER_USAGE_TRANSFER_SRC_BIT";
        case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:
            return "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT";
        case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT:
            return "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT";
        case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:
            return "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT";
        default:
            return "Unhandled VkBufferUsageFlagBits";
    }
}


static inline const char* string_VkColorComponentFlagBits(VkColorComponentFlagBits input_value)
{
    switch ((VkColorComponentFlagBits)input_value)
    {
        case VK_COLOR_COMPONENT_A_BIT:
            return "VK_COLOR_COMPONENT_A_BIT";
        case VK_COLOR_COMPONENT_B_BIT:
            return "VK_COLOR_COMPONENT_B_BIT";
        case VK_COLOR_COMPONENT_G_BIT:
            return "VK_COLOR_COMPONENT_G_BIT";
        case VK_COLOR_COMPONENT_R_BIT:
            return "VK_COLOR_COMPONENT_R_BIT";
        default:
            return "Unhandled VkColorComponentFlagBits";
    }
}


static inline const char* string_VkColorSpaceKHR(VkColorSpaceKHR input_value)
{
    switch ((VkColorSpaceKHR)input_value)
    {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
            return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
        default:
            return "Unhandled VkColorSpaceKHR";
    }
}


static inline const char* string_VkCommandBufferLevel(VkCommandBufferLevel input_value)
{
    switch ((VkCommandBufferLevel)input_value)
    {
        case VK_COMMAND_BUFFER_LEVEL_PRIMARY:
            return "VK_COMMAND_BUFFER_LEVEL_PRIMARY";
        case VK_COMMAND_BUFFER_LEVEL_SECONDARY:
            return "VK_COMMAND_BUFFER_LEVEL_SECONDARY";
        default:
            return "Unhandled VkCommandBufferLevel";
    }
}


static inline const char* string_VkCommandBufferResetFlagBits(VkCommandBufferResetFlagBits input_value)
{
    switch ((VkCommandBufferResetFlagBits)input_value)
    {
        case VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT:
            return "VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT";
        default:
            return "Unhandled VkCommandBufferResetFlagBits";
    }
}


static inline const char* string_VkCommandBufferUsageFlagBits(VkCommandBufferUsageFlagBits input_value)
{
    switch ((VkCommandBufferUsageFlagBits)input_value)
    {
        case VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT:
            return "VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT";
        case VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT:
            return "VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT";
        case VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT:
            return "VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT";
        default:
            return "Unhandled VkCommandBufferUsageFlagBits";
    }
}


static inline const char* string_VkCommandPoolCreateFlagBits(VkCommandPoolCreateFlagBits input_value)
{
    switch ((VkCommandPoolCreateFlagBits)input_value)
    {
        case VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT:
            return "VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT";
        case VK_COMMAND_POOL_CREATE_TRANSIENT_BIT:
            return "VK_COMMAND_POOL_CREATE_TRANSIENT_BIT";
        default:
            return "Unhandled VkCommandPoolCreateFlagBits";
    }
}


static inline const char* string_VkCommandPoolResetFlagBits(VkCommandPoolResetFlagBits input_value)
{
    switch ((VkCommandPoolResetFlagBits)input_value)
    {
        case VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT:
            return "VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT";
        default:
            return "Unhandled VkCommandPoolResetFlagBits";
    }
}


static inline const char* string_VkCompareOp(VkCompareOp input_value)
{
    switch ((VkCompareOp)input_value)
    {
        case VK_COMPARE_OP_ALWAYS:
            return "VK_COMPARE_OP_ALWAYS";
        case VK_COMPARE_OP_EQUAL:
            return "VK_COMPARE_OP_EQUAL";
        case VK_COMPARE_OP_GREATER:
            return "VK_COMPARE_OP_GREATER";
        case VK_COMPARE_OP_GREATER_OR_EQUAL:
            return "VK_COMPARE_OP_GREATER_OR_EQUAL";
        case VK_COMPARE_OP_LESS:
            return "VK_COMPARE_OP_LESS";
        case VK_COMPARE_OP_LESS_OR_EQUAL:
            return "VK_COMPARE_OP_LESS_OR_EQUAL";
        case VK_COMPARE_OP_NEVER:
            return "VK_COMPARE_OP_NEVER";
        case VK_COMPARE_OP_NOT_EQUAL:
            return "VK_COMPARE_OP_NOT_EQUAL";
        default:
            return "Unhandled VkCompareOp";
    }
}


static inline const char* string_VkComponentSwizzle(VkComponentSwizzle input_value)
{
    switch ((VkComponentSwizzle)input_value)
    {
        case VK_COMPONENT_SWIZZLE_A:
            return "VK_COMPONENT_SWIZZLE_A";
        case VK_COMPONENT_SWIZZLE_B:
            return "VK_COMPONENT_SWIZZLE_B";
        case VK_COMPONENT_SWIZZLE_G:
            return "VK_COMPONENT_SWIZZLE_G";
        case VK_COMPONENT_SWIZZLE_IDENTITY:
            return "VK_COMPONENT_SWIZZLE_IDENTITY";
        case VK_COMPONENT_SWIZZLE_ONE:
            return "VK_COMPONENT_SWIZZLE_ONE";
        case VK_COMPONENT_SWIZZLE_R:
            return "VK_COMPONENT_SWIZZLE_R";
        case VK_COMPONENT_SWIZZLE_ZERO:
            return "VK_COMPONENT_SWIZZLE_ZERO";
        default:
            return "Unhandled VkComponentSwizzle";
    }
}


static inline const char* string_VkCompositeAlphaFlagBitsKHR(VkCompositeAlphaFlagBitsKHR input_value)
{
    switch ((VkCompositeAlphaFlagBitsKHR)input_value)
    {
        case VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR:
            return "VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR";
        case VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR:
            return "VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR";
        case VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR:
            return "VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR";
        case VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR:
            return "VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR";
        default:
            return "Unhandled VkCompositeAlphaFlagBitsKHR";
    }
}


static inline const char* string_VkCullModeFlagBits(VkCullModeFlagBits input_value)
{
    switch ((VkCullModeFlagBits)input_value)
    {
        case VK_CULL_MODE_BACK_BIT:
            return "VK_CULL_MODE_BACK_BIT";
        case VK_CULL_MODE_FRONT_AND_BACK:
            return "VK_CULL_MODE_FRONT_AND_BACK";
        case VK_CULL_MODE_FRONT_BIT:
            return "VK_CULL_MODE_FRONT_BIT";
        case VK_CULL_MODE_NONE:
            return "VK_CULL_MODE_NONE";
        default:
            return "Unhandled VkCullModeFlagBits";
    }
}


static inline const char* string_VkDebugReportErrorEXT(VkDebugReportErrorEXT input_value)
{
    switch ((VkDebugReportErrorEXT)input_value)
    {
        case VK_DEBUG_REPORT_ERROR_CALLBACK_REF_EXT:
            return "VK_DEBUG_REPORT_ERROR_CALLBACK_REF_EXT";
        case VK_DEBUG_REPORT_ERROR_NONE_EXT:
            return "VK_DEBUG_REPORT_ERROR_NONE_EXT";
        default:
            return "Unhandled VkDebugReportErrorEXT";
    }
}


static inline const char* string_VkDebugReportFlagBitsEXT(VkDebugReportFlagBitsEXT input_value)
{
    switch ((VkDebugReportFlagBitsEXT)input_value)
    {
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            return "VK_DEBUG_REPORT_DEBUG_BIT_EXT";
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            return "VK_DEBUG_REPORT_ERROR_BIT_EXT";
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            return "VK_DEBUG_REPORT_INFORMATION_BIT_EXT";
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            return "VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT";
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            return "VK_DEBUG_REPORT_WARNING_BIT_EXT";
        default:
            return "Unhandled VkDebugReportFlagBitsEXT";
    }
}


static inline const char* string_VkDebugReportObjectTypeEXT(VkDebugReportObjectTypeEXT input_value)
{
    switch ((VkDebugReportObjectTypeEXT)input_value)
    {
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT";
        default:
            return "Unhandled VkDebugReportObjectTypeEXT";
    }
}


static inline const char* string_VkDependencyFlagBits(VkDependencyFlagBits input_value)
{
    switch ((VkDependencyFlagBits)input_value)
    {
        case VK_DEPENDENCY_BY_REGION_BIT:
            return "VK_DEPENDENCY_BY_REGION_BIT";
        default:
            return "Unhandled VkDependencyFlagBits";
    }
}


static inline const char* string_VkDescriptorPoolCreateFlagBits(VkDescriptorPoolCreateFlagBits input_value)
{
    switch ((VkDescriptorPoolCreateFlagBits)input_value)
    {
        case VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT:
            return "VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT";
        default:
            return "Unhandled VkDescriptorPoolCreateFlagBits";
    }
}


static inline const char* string_VkDescriptorType(VkDescriptorType input_value)
{
    switch ((VkDescriptorType)input_value)
    {
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            return "VK_DESCRIPTOR_TYPE_SAMPLER";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        default:
            return "Unhandled VkDescriptorType";
    }
}


static inline const char* string_VkDisplayPlaneAlphaFlagBitsKHR(VkDisplayPlaneAlphaFlagBitsKHR input_value)
{
    switch ((VkDisplayPlaneAlphaFlagBitsKHR)input_value)
    {
        case VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR:
            return "VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR";
        case VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR:
            return "VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR";
        case VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR:
            return "VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR";
        case VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR:
            return "VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR";
        default:
            return "Unhandled VkDisplayPlaneAlphaFlagBitsKHR";
    }
}


static inline const char* string_VkDynamicState(VkDynamicState input_value)
{
    switch ((VkDynamicState)input_value)
    {
        case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
            return "VK_DYNAMIC_STATE_BLEND_CONSTANTS";
        case VK_DYNAMIC_STATE_DEPTH_BIAS:
            return "VK_DYNAMIC_STATE_DEPTH_BIAS";
        case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
            return "VK_DYNAMIC_STATE_DEPTH_BOUNDS";
        case VK_DYNAMIC_STATE_LINE_WIDTH:
            return "VK_DYNAMIC_STATE_LINE_WIDTH";
        case VK_DYNAMIC_STATE_SCISSOR:
            return "VK_DYNAMIC_STATE_SCISSOR";
        case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
            return "VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK";
        case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
            return "VK_DYNAMIC_STATE_STENCIL_REFERENCE";
        case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
            return "VK_DYNAMIC_STATE_STENCIL_WRITE_MASK";
        case VK_DYNAMIC_STATE_VIEWPORT:
            return "VK_DYNAMIC_STATE_VIEWPORT";
        default:
            return "Unhandled VkDynamicState";
    }
}


static inline const char* string_VkExternalMemoryFeatureFlagBitsNV(VkExternalMemoryFeatureFlagBitsNV input_value)
{
    switch ((VkExternalMemoryFeatureFlagBitsNV)input_value)
    {
        case VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV:
            return "VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV";
        case VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV:
            return "VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV";
        case VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV:
            return "VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV";
        default:
            return "Unhandled VkExternalMemoryFeatureFlagBitsNV";
    }
}


static inline const char* string_VkExternalMemoryHandleTypeFlagBitsNV(VkExternalMemoryHandleTypeFlagBitsNV input_value)
{
    switch ((VkExternalMemoryHandleTypeFlagBitsNV)input_value)
    {
        case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV:
            return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV";
        case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV:
            return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV";
        case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV:
            return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV";
        case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV:
            return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV";
        default:
            return "Unhandled VkExternalMemoryHandleTypeFlagBitsNV";
    }
}


static inline const char* string_VkFenceCreateFlagBits(VkFenceCreateFlagBits input_value)
{
    switch ((VkFenceCreateFlagBits)input_value)
    {
        case VK_FENCE_CREATE_SIGNALED_BIT:
            return "VK_FENCE_CREATE_SIGNALED_BIT";
        default:
            return "Unhandled VkFenceCreateFlagBits";
    }
}


static inline const char* string_VkFilter(VkFilter input_value)
{
    switch ((VkFilter)input_value)
    {
        case VK_FILTER_CUBIC_IMG:
            return "VK_FILTER_CUBIC_IMG";
        case VK_FILTER_LINEAR:
            return "VK_FILTER_LINEAR";
        case VK_FILTER_NEAREST:
            return "VK_FILTER_NEAREST";
        default:
            return "Unhandled VkFilter";
    }
}


static inline const char* string_VkFormat(VkFormat input_value)
{
    switch ((VkFormat)input_value)
    {
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            return "VK_FORMAT_B5G6R5_UNORM_PACK16";
        case VK_FORMAT_B8G8R8A8_SINT:
            return "VK_FORMAT_B8G8R8A8_SINT";
        case VK_FORMAT_B8G8R8A8_SNORM:
            return "VK_FORMAT_B8G8R8A8_SNORM";
        case VK_FORMAT_B8G8R8A8_SRGB:
            return "VK_FORMAT_B8G8R8A8_SRGB";
        case VK_FORMAT_B8G8R8A8_SSCALED:
            return "VK_FORMAT_B8G8R8A8_SSCALED";
        case VK_FORMAT_B8G8R8A8_UINT:
            return "VK_FORMAT_B8G8R8A8_UINT";
        case VK_FORMAT_B8G8R8A8_UNORM:
            return "VK_FORMAT_B8G8R8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_USCALED:
            return "VK_FORMAT_B8G8R8A8_USCALED";
        case VK_FORMAT_B8G8R8_SINT:
            return "VK_FORMAT_B8G8R8_SINT";
        case VK_FORMAT_B8G8R8_SNORM:
            return "VK_FORMAT_B8G8R8_SNORM";
        case VK_FORMAT_B8G8R8_SRGB:
            return "VK_FORMAT_B8G8R8_SRGB";
        case VK_FORMAT_B8G8R8_SSCALED:
            return "VK_FORMAT_B8G8R8_SSCALED";
        case VK_FORMAT_B8G8R8_UINT:
            return "VK_FORMAT_B8G8R8_UINT";
        case VK_FORMAT_B8G8R8_UNORM:
            return "VK_FORMAT_B8G8R8_UNORM";
        case VK_FORMAT_B8G8R8_USCALED:
            return "VK_FORMAT_B8G8R8_USCALED";
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
            return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
        case VK_FORMAT_BC2_SRGB_BLOCK:
            return "VK_FORMAT_BC2_SRGB_BLOCK";
        case VK_FORMAT_BC2_UNORM_BLOCK:
            return "VK_FORMAT_BC2_UNORM_BLOCK";
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return "VK_FORMAT_BC3_SRGB_BLOCK";
        case VK_FORMAT_BC3_UNORM_BLOCK:
            return "VK_FORMAT_BC3_UNORM_BLOCK";
        case VK_FORMAT_BC4_SNORM_BLOCK:
            return "VK_FORMAT_BC4_SNORM_BLOCK";
        case VK_FORMAT_BC4_UNORM_BLOCK:
            return "VK_FORMAT_BC4_UNORM_BLOCK";
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return "VK_FORMAT_BC5_SNORM_BLOCK";
        case VK_FORMAT_BC5_UNORM_BLOCK:
            return "VK_FORMAT_BC5_UNORM_BLOCK";
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
            return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return "VK_FORMAT_BC7_SRGB_BLOCK";
        case VK_FORMAT_BC7_UNORM_BLOCK:
            return "VK_FORMAT_BC7_UNORM_BLOCK";
        case VK_FORMAT_D16_UNORM:
            return "VK_FORMAT_D16_UNORM";
        case VK_FORMAT_D16_UNORM_S8_UINT:
            return "VK_FORMAT_D16_UNORM_S8_UINT";
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return "VK_FORMAT_D24_UNORM_S8_UINT";
        case VK_FORMAT_D32_SFLOAT:
            return "VK_FORMAT_D32_SFLOAT";
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return "VK_FORMAT_D32_SFLOAT_S8_UINT";
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case VK_FORMAT_R16G16B16A16_SINT:
            return "VK_FORMAT_R16G16B16A16_SINT";
        case VK_FORMAT_R16G16B16A16_SNORM:
            return "VK_FORMAT_R16G16B16A16_SNORM";
        case VK_FORMAT_R16G16B16A16_SSCALED:
            return "VK_FORMAT_R16G16B16A16_SSCALED";
        case VK_FORMAT_R16G16B16A16_UINT:
            return "VK_FORMAT_R16G16B16A16_UINT";
        case VK_FORMAT_R16G16B16A16_UNORM:
            return "VK_FORMAT_R16G16B16A16_UNORM";
        case VK_FORMAT_R16G16B16A16_USCALED:
            return "VK_FORMAT_R16G16B16A16_USCALED";
        case VK_FORMAT_R16G16B16_SFLOAT:
            return "VK_FORMAT_R16G16B16_SFLOAT";
        case VK_FORMAT_R16G16B16_SINT:
            return "VK_FORMAT_R16G16B16_SINT";
        case VK_FORMAT_R16G16B16_SNORM:
            return "VK_FORMAT_R16G16B16_SNORM";
        case VK_FORMAT_R16G16B16_SSCALED:
            return "VK_FORMAT_R16G16B16_SSCALED";
        case VK_FORMAT_R16G16B16_UINT:
            return "VK_FORMAT_R16G16B16_UINT";
        case VK_FORMAT_R16G16B16_UNORM:
            return "VK_FORMAT_R16G16B16_UNORM";
        case VK_FORMAT_R16G16B16_USCALED:
            return "VK_FORMAT_R16G16B16_USCALED";
        case VK_FORMAT_R16G16_SFLOAT:
            return "VK_FORMAT_R16G16_SFLOAT";
        case VK_FORMAT_R16G16_SINT:
            return "VK_FORMAT_R16G16_SINT";
        case VK_FORMAT_R16G16_SNORM:
            return "VK_FORMAT_R16G16_SNORM";
        case VK_FORMAT_R16G16_SSCALED:
            return "VK_FORMAT_R16G16_SSCALED";
        case VK_FORMAT_R16G16_UINT:
            return "VK_FORMAT_R16G16_UINT";
        case VK_FORMAT_R16G16_UNORM:
            return "VK_FORMAT_R16G16_UNORM";
        case VK_FORMAT_R16G16_USCALED:
            return "VK_FORMAT_R16G16_USCALED";
        case VK_FORMAT_R16_SFLOAT:
            return "VK_FORMAT_R16_SFLOAT";
        case VK_FORMAT_R16_SINT:
            return "VK_FORMAT_R16_SINT";
        case VK_FORMAT_R16_SNORM:
            return "VK_FORMAT_R16_SNORM";
        case VK_FORMAT_R16_SSCALED:
            return "VK_FORMAT_R16_SSCALED";
        case VK_FORMAT_R16_UINT:
            return "VK_FORMAT_R16_UINT";
        case VK_FORMAT_R16_UNORM:
            return "VK_FORMAT_R16_UNORM";
        case VK_FORMAT_R16_USCALED:
            return "VK_FORMAT_R16_USCALED";
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return "VK_FORMAT_R32G32B32A32_SFLOAT";
        case VK_FORMAT_R32G32B32A32_SINT:
            return "VK_FORMAT_R32G32B32A32_SINT";
        case VK_FORMAT_R32G32B32A32_UINT:
            return "VK_FORMAT_R32G32B32A32_UINT";
        case VK_FORMAT_R32G32B32_SFLOAT:
            return "VK_FORMAT_R32G32B32_SFLOAT";
        case VK_FORMAT_R32G32B32_SINT:
            return "VK_FORMAT_R32G32B32_SINT";
        case VK_FORMAT_R32G32B32_UINT:
            return "VK_FORMAT_R32G32B32_UINT";
        case VK_FORMAT_R32G32_SFLOAT:
            return "VK_FORMAT_R32G32_SFLOAT";
        case VK_FORMAT_R32G32_SINT:
            return "VK_FORMAT_R32G32_SINT";
        case VK_FORMAT_R32G32_UINT:
            return "VK_FORMAT_R32G32_UINT";
        case VK_FORMAT_R32_SFLOAT:
            return "VK_FORMAT_R32_SFLOAT";
        case VK_FORMAT_R32_SINT:
            return "VK_FORMAT_R32_SINT";
        case VK_FORMAT_R32_UINT:
            return "VK_FORMAT_R32_UINT";
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
        case VK_FORMAT_R4G4_UNORM_PACK8:
            return "VK_FORMAT_R4G4_UNORM_PACK8";
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            return "VK_FORMAT_R5G6B5_UNORM_PACK16";
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return "VK_FORMAT_R64G64B64A64_SFLOAT";
        case VK_FORMAT_R64G64B64A64_SINT:
            return "VK_FORMAT_R64G64B64A64_SINT";
        case VK_FORMAT_R64G64B64A64_UINT:
            return "VK_FORMAT_R64G64B64A64_UINT";
        case VK_FORMAT_R64G64B64_SFLOAT:
            return "VK_FORMAT_R64G64B64_SFLOAT";
        case VK_FORMAT_R64G64B64_SINT:
            return "VK_FORMAT_R64G64B64_SINT";
        case VK_FORMAT_R64G64B64_UINT:
            return "VK_FORMAT_R64G64B64_UINT";
        case VK_FORMAT_R64G64_SFLOAT:
            return "VK_FORMAT_R64G64_SFLOAT";
        case VK_FORMAT_R64G64_SINT:
            return "VK_FORMAT_R64G64_SINT";
        case VK_FORMAT_R64G64_UINT:
            return "VK_FORMAT_R64G64_UINT";
        case VK_FORMAT_R64_SFLOAT:
            return "VK_FORMAT_R64_SFLOAT";
        case VK_FORMAT_R64_SINT:
            return "VK_FORMAT_R64_SINT";
        case VK_FORMAT_R64_UINT:
            return "VK_FORMAT_R64_UINT";
        case VK_FORMAT_R8G8B8A8_SINT:
            return "VK_FORMAT_R8G8B8A8_SINT";
        case VK_FORMAT_R8G8B8A8_SNORM:
            return "VK_FORMAT_R8G8B8A8_SNORM";
        case VK_FORMAT_R8G8B8A8_SRGB:
            return "VK_FORMAT_R8G8B8A8_SRGB";
        case VK_FORMAT_R8G8B8A8_SSCALED:
            return "VK_FORMAT_R8G8B8A8_SSCALED";
        case VK_FORMAT_R8G8B8A8_UINT:
            return "VK_FORMAT_R8G8B8A8_UINT";
        case VK_FORMAT_R8G8B8A8_UNORM:
            return "VK_FORMAT_R8G8B8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_USCALED:
            return "VK_FORMAT_R8G8B8A8_USCALED";
        case VK_FORMAT_R8G8B8_SINT:
            return "VK_FORMAT_R8G8B8_SINT";
        case VK_FORMAT_R8G8B8_SNORM:
            return "VK_FORMAT_R8G8B8_SNORM";
        case VK_FORMAT_R8G8B8_SRGB:
            return "VK_FORMAT_R8G8B8_SRGB";
        case VK_FORMAT_R8G8B8_SSCALED:
            return "VK_FORMAT_R8G8B8_SSCALED";
        case VK_FORMAT_R8G8B8_UINT:
            return "VK_FORMAT_R8G8B8_UINT";
        case VK_FORMAT_R8G8B8_UNORM:
            return "VK_FORMAT_R8G8B8_UNORM";
        case VK_FORMAT_R8G8B8_USCALED:
            return "VK_FORMAT_R8G8B8_USCALED";
        case VK_FORMAT_R8G8_SINT:
            return "VK_FORMAT_R8G8_SINT";
        case VK_FORMAT_R8G8_SNORM:
            return "VK_FORMAT_R8G8_SNORM";
        case VK_FORMAT_R8G8_SRGB:
            return "VK_FORMAT_R8G8_SRGB";
        case VK_FORMAT_R8G8_SSCALED:
            return "VK_FORMAT_R8G8_SSCALED";
        case VK_FORMAT_R8G8_UINT:
            return "VK_FORMAT_R8G8_UINT";
        case VK_FORMAT_R8G8_UNORM:
            return "VK_FORMAT_R8G8_UNORM";
        case VK_FORMAT_R8G8_USCALED:
            return "VK_FORMAT_R8G8_USCALED";
        case VK_FORMAT_R8_SINT:
            return "VK_FORMAT_R8_SINT";
        case VK_FORMAT_R8_SNORM:
            return "VK_FORMAT_R8_SNORM";
        case VK_FORMAT_R8_SRGB:
            return "VK_FORMAT_R8_SRGB";
        case VK_FORMAT_R8_SSCALED:
            return "VK_FORMAT_R8_SSCALED";
        case VK_FORMAT_R8_UINT:
            return "VK_FORMAT_R8_UINT";
        case VK_FORMAT_R8_UNORM:
            return "VK_FORMAT_R8_UNORM";
        case VK_FORMAT_R8_USCALED:
            return "VK_FORMAT_R8_USCALED";
        case VK_FORMAT_S8_UINT:
            return "VK_FORMAT_S8_UINT";
        case VK_FORMAT_UNDEFINED:
            return "VK_FORMAT_UNDEFINED";
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return "VK_FORMAT_X8_D24_UNORM_PACK32";
        default:
            return "Unhandled VkFormat";
    }
}


static inline const char* string_VkFormatFeatureFlagBits(VkFormatFeatureFlagBits input_value)
{
    switch ((VkFormatFeatureFlagBits)input_value)
    {
        case VK_FORMAT_FEATURE_BLIT_DST_BIT:
            return "VK_FORMAT_FEATURE_BLIT_DST_BIT";
        case VK_FORMAT_FEATURE_BLIT_SRC_BIT:
            return "VK_FORMAT_FEATURE_BLIT_SRC_BIT";
        case VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT:
            return "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT";
        case VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT:
            return "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT";
        case VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT:
            return "VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT";
        case VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT:
            return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT";
        case VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG:
            return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG";
        case VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT:
            return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT";
        case VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT:
            return "VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT";
        case VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT:
            return "VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT";
        case VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT:
            return "VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT";
        case VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT:
            return "VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT";
        case VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT:
            return "VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT";
        case VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT:
            return "VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT";
        default:
            return "Unhandled VkFormatFeatureFlagBits";
    }
}


static inline const char* string_VkFrontFace(VkFrontFace input_value)
{
    switch ((VkFrontFace)input_value)
    {
        case VK_FRONT_FACE_CLOCKWISE:
            return "VK_FRONT_FACE_CLOCKWISE";
        case VK_FRONT_FACE_COUNTER_CLOCKWISE:
            return "VK_FRONT_FACE_COUNTER_CLOCKWISE";
        default:
            return "Unhandled VkFrontFace";
    }
}


static inline const char* string_VkImageAspectFlagBits(VkImageAspectFlagBits input_value)
{
    switch ((VkImageAspectFlagBits)input_value)
    {
        case VK_IMAGE_ASPECT_COLOR_BIT:
            return "VK_IMAGE_ASPECT_COLOR_BIT";
        case VK_IMAGE_ASPECT_DEPTH_BIT:
            return "VK_IMAGE_ASPECT_DEPTH_BIT";
        case VK_IMAGE_ASPECT_METADATA_BIT:
            return "VK_IMAGE_ASPECT_METADATA_BIT";
        case VK_IMAGE_ASPECT_STENCIL_BIT:
            return "VK_IMAGE_ASPECT_STENCIL_BIT";
        default:
            return "Unhandled VkImageAspectFlagBits";
    }
}


static inline const char* string_VkImageCreateFlagBits(VkImageCreateFlagBits input_value)
{
    switch ((VkImageCreateFlagBits)input_value)
    {
        case VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:
            return "VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT";
        case VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT:
            return "VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT";
        case VK_IMAGE_CREATE_SPARSE_ALIASED_BIT:
            return "VK_IMAGE_CREATE_SPARSE_ALIASED_BIT";
        case VK_IMAGE_CREATE_SPARSE_BINDING_BIT:
            return "VK_IMAGE_CREATE_SPARSE_BINDING_BIT";
        case VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT:
            return "VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT";
        default:
            return "Unhandled VkImageCreateFlagBits";
    }
}


static inline const char* string_VkImageLayout(VkImageLayout input_value)
{
    switch ((VkImageLayout)input_value)
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL";
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL";
        case VK_IMAGE_LAYOUT_GENERAL:
            return "VK_IMAGE_LAYOUT_GENERAL";
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return "VK_IMAGE_LAYOUT_PREINITIALIZED";
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR";
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL";
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return "VK_IMAGE_LAYOUT_UNDEFINED";
        default:
            return "Unhandled VkImageLayout";
    }
}


static inline const char* string_VkImageTiling(VkImageTiling input_value)
{
    switch ((VkImageTiling)input_value)
    {
        case VK_IMAGE_TILING_LINEAR:
            return "VK_IMAGE_TILING_LINEAR";
        case VK_IMAGE_TILING_OPTIMAL:
            return "VK_IMAGE_TILING_OPTIMAL";
        default:
            return "Unhandled VkImageTiling";
    }
}


static inline const char* string_VkImageType(VkImageType input_value)
{
    switch ((VkImageType)input_value)
    {
        case VK_IMAGE_TYPE_1D:
            return "VK_IMAGE_TYPE_1D";
        case VK_IMAGE_TYPE_2D:
            return "VK_IMAGE_TYPE_2D";
        case VK_IMAGE_TYPE_3D:
            return "VK_IMAGE_TYPE_3D";
        default:
            return "Unhandled VkImageType";
    }
}


static inline const char* string_VkImageUsageFlagBits(VkImageUsageFlagBits input_value)
{
    switch ((VkImageUsageFlagBits)input_value)
    {
        case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
            return "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
        case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
            return "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
        case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT:
            return "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
        case VK_IMAGE_USAGE_SAMPLED_BIT:
            return "VK_IMAGE_USAGE_SAMPLED_BIT";
        case VK_IMAGE_USAGE_STORAGE_BIT:
            return "VK_IMAGE_USAGE_STORAGE_BIT";
        case VK_IMAGE_USAGE_TRANSFER_DST_BIT:
            return "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
        case VK_IMAGE_USAGE_TRANSFER_SRC_BIT:
            return "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
        case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT:
            return "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
        default:
            return "Unhandled VkImageUsageFlagBits";
    }
}


static inline const char* string_VkImageViewType(VkImageViewType input_value)
{
    switch ((VkImageViewType)input_value)
    {
        case VK_IMAGE_VIEW_TYPE_1D:
            return "VK_IMAGE_VIEW_TYPE_1D";
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
            return "VK_IMAGE_VIEW_TYPE_1D_ARRAY";
        case VK_IMAGE_VIEW_TYPE_2D:
            return "VK_IMAGE_VIEW_TYPE_2D";
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
            return "VK_IMAGE_VIEW_TYPE_2D_ARRAY";
        case VK_IMAGE_VIEW_TYPE_3D:
            return "VK_IMAGE_VIEW_TYPE_3D";
        case VK_IMAGE_VIEW_TYPE_CUBE:
            return "VK_IMAGE_VIEW_TYPE_CUBE";
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
            return "VK_IMAGE_VIEW_TYPE_CUBE_ARRAY";
        default:
            return "Unhandled VkImageViewType";
    }
}


static inline const char* string_VkIndexType(VkIndexType input_value)
{
    switch ((VkIndexType)input_value)
    {
        case VK_INDEX_TYPE_UINT16:
            return "VK_INDEX_TYPE_UINT16";
        case VK_INDEX_TYPE_UINT32:
            return "VK_INDEX_TYPE_UINT32";
        default:
            return "Unhandled VkIndexType";
    }
}


static inline const char* string_VkInternalAllocationType(VkInternalAllocationType input_value)
{
    switch ((VkInternalAllocationType)input_value)
    {
        case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE:
            return "VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE";
        default:
            return "Unhandled VkInternalAllocationType";
    }
}


static inline const char* string_VkLogicOp(VkLogicOp input_value)
{
    switch ((VkLogicOp)input_value)
    {
        case VK_LOGIC_OP_AND:
            return "VK_LOGIC_OP_AND";
        case VK_LOGIC_OP_AND_INVERTED:
            return "VK_LOGIC_OP_AND_INVERTED";
        case VK_LOGIC_OP_AND_REVERSE:
            return "VK_LOGIC_OP_AND_REVERSE";
        case VK_LOGIC_OP_CLEAR:
            return "VK_LOGIC_OP_CLEAR";
        case VK_LOGIC_OP_COPY:
            return "VK_LOGIC_OP_COPY";
        case VK_LOGIC_OP_COPY_INVERTED:
            return "VK_LOGIC_OP_COPY_INVERTED";
        case VK_LOGIC_OP_EQUIVALENT:
            return "VK_LOGIC_OP_EQUIVALENT";
        case VK_LOGIC_OP_INVERT:
            return "VK_LOGIC_OP_INVERT";
        case VK_LOGIC_OP_NAND:
            return "VK_LOGIC_OP_NAND";
        case VK_LOGIC_OP_NOR:
            return "VK_LOGIC_OP_NOR";
        case VK_LOGIC_OP_NO_OP:
            return "VK_LOGIC_OP_NO_OP";
        case VK_LOGIC_OP_OR:
            return "VK_LOGIC_OP_OR";
        case VK_LOGIC_OP_OR_INVERTED:
            return "VK_LOGIC_OP_OR_INVERTED";
        case VK_LOGIC_OP_OR_REVERSE:
            return "VK_LOGIC_OP_OR_REVERSE";
        case VK_LOGIC_OP_SET:
            return "VK_LOGIC_OP_SET";
        case VK_LOGIC_OP_XOR:
            return "VK_LOGIC_OP_XOR";
        default:
            return "Unhandled VkLogicOp";
    }
}


static inline const char* string_VkMemoryHeapFlagBits(VkMemoryHeapFlagBits input_value)
{
    switch ((VkMemoryHeapFlagBits)input_value)
    {
        case VK_MEMORY_HEAP_DEVICE_LOCAL_BIT:
            return "VK_MEMORY_HEAP_DEVICE_LOCAL_BIT";
        default:
            return "Unhandled VkMemoryHeapFlagBits";
    }
}


static inline const char* string_VkMemoryPropertyFlagBits(VkMemoryPropertyFlagBits input_value)
{
    switch ((VkMemoryPropertyFlagBits)input_value)
    {
        case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:
            return "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
        case VK_MEMORY_PROPERTY_HOST_CACHED_BIT:
            return "VK_MEMORY_PROPERTY_HOST_CACHED_BIT";
        case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
            return "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT";
        case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:
            return "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT";
        case VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT:
            return "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT";
        default:
            return "Unhandled VkMemoryPropertyFlagBits";
    }
}


static inline const char* string_VkPhysicalDeviceType(VkPhysicalDeviceType input_value)
{
    switch ((VkPhysicalDeviceType)input_value)
    {
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "VK_PHYSICAL_DEVICE_TYPE_CPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
        default:
            return "Unhandled VkPhysicalDeviceType";
    }
}


static inline const char* string_VkPipelineBindPoint(VkPipelineBindPoint input_value)
{
    switch ((VkPipelineBindPoint)input_value)
    {
        case VK_PIPELINE_BIND_POINT_COMPUTE:
            return "VK_PIPELINE_BIND_POINT_COMPUTE";
        case VK_PIPELINE_BIND_POINT_GRAPHICS:
            return "VK_PIPELINE_BIND_POINT_GRAPHICS";
        default:
            return "Unhandled VkPipelineBindPoint";
    }
}


static inline const char* string_VkPipelineCacheHeaderVersion(VkPipelineCacheHeaderVersion input_value)
{
    switch ((VkPipelineCacheHeaderVersion)input_value)
    {
        case VK_PIPELINE_CACHE_HEADER_VERSION_ONE:
            return "VK_PIPELINE_CACHE_HEADER_VERSION_ONE";
        default:
            return "Unhandled VkPipelineCacheHeaderVersion";
    }
}


static inline const char* string_VkPipelineCreateFlagBits(VkPipelineCreateFlagBits input_value)
{
    switch ((VkPipelineCreateFlagBits)input_value)
    {
        case VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT:
            return "VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT";
        case VK_PIPELINE_CREATE_DERIVATIVE_BIT:
            return "VK_PIPELINE_CREATE_DERIVATIVE_BIT";
        case VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT:
            return "VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT";
        default:
            return "Unhandled VkPipelineCreateFlagBits";
    }
}


static inline const char* string_VkPipelineStageFlagBits(VkPipelineStageFlagBits input_value)
{
    switch ((VkPipelineStageFlagBits)input_value)
    {
        case VK_PIPELINE_STAGE_ALL_COMMANDS_BIT:
            return "VK_PIPELINE_STAGE_ALL_COMMANDS_BIT";
        case VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT:
            return "VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT";
        case VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT:
            return "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT";
        case VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
            return "VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT";
        case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:
            return "VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT";
        case VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT:
            return "VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT";
        case VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT:
            return "VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT";
        case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
            return "VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT";
        case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:
            return "VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT";
        case VK_PIPELINE_STAGE_HOST_BIT:
            return "VK_PIPELINE_STAGE_HOST_BIT";
        case VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT:
            return "VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT";
        case VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT:
            return "VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT";
        case VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT:
            return "VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT";
        case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:
            return "VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT";
        case VK_PIPELINE_STAGE_TRANSFER_BIT:
            return "VK_PIPELINE_STAGE_TRANSFER_BIT";
        case VK_PIPELINE_STAGE_VERTEX_INPUT_BIT:
            return "VK_PIPELINE_STAGE_VERTEX_INPUT_BIT";
        case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:
            return "VK_PIPELINE_STAGE_VERTEX_SHADER_BIT";
        default:
            return "Unhandled VkPipelineStageFlagBits";
    }
}


static inline const char* string_VkPolygonMode(VkPolygonMode input_value)
{
    switch ((VkPolygonMode)input_value)
    {
        case VK_POLYGON_MODE_FILL:
            return "VK_POLYGON_MODE_FILL";
        case VK_POLYGON_MODE_LINE:
            return "VK_POLYGON_MODE_LINE";
        case VK_POLYGON_MODE_POINT:
            return "VK_POLYGON_MODE_POINT";
        default:
            return "Unhandled VkPolygonMode";
    }
}


static inline const char* string_VkPresentModeKHR(VkPresentModeKHR input_value)
{
    switch ((VkPresentModeKHR)input_value)
    {
        case VK_PRESENT_MODE_FIFO_KHR:
            return "VK_PRESENT_MODE_FIFO_KHR";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "VK_PRESENT_MODE_IMMEDIATE_KHR";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return "VK_PRESENT_MODE_MAILBOX_KHR";
        default:
            return "Unhandled VkPresentModeKHR";
    }
}


static inline const char* string_VkPrimitiveTopology(VkPrimitiveTopology input_value)
{
    switch ((VkPrimitiveTopology)input_value)
    {
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
            return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST";
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
            return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY";
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP";
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
            return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY";
        case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
            return "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST";
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
            return "VK_PRIMITIVE_TOPOLOGY_POINT_LIST";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
            return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
            return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY";
        default:
            return "Unhandled VkPrimitiveTopology";
    }
}


static inline const char* string_VkQueryControlFlagBits(VkQueryControlFlagBits input_value)
{
    switch ((VkQueryControlFlagBits)input_value)
    {
        case VK_QUERY_CONTROL_PRECISE_BIT:
            return "VK_QUERY_CONTROL_PRECISE_BIT";
        default:
            return "Unhandled VkQueryControlFlagBits";
    }
}


static inline const char* string_VkQueryPipelineStatisticFlagBits(VkQueryPipelineStatisticFlagBits input_value)
{
    switch ((VkQueryPipelineStatisticFlagBits)input_value)
    {
        case VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT";
        case VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT:
            return "VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT";
        default:
            return "Unhandled VkQueryPipelineStatisticFlagBits";
    }
}


static inline const char* string_VkQueryResultFlagBits(VkQueryResultFlagBits input_value)
{
    switch ((VkQueryResultFlagBits)input_value)
    {
        case VK_QUERY_RESULT_64_BIT:
            return "VK_QUERY_RESULT_64_BIT";
        case VK_QUERY_RESULT_PARTIAL_BIT:
            return "VK_QUERY_RESULT_PARTIAL_BIT";
        case VK_QUERY_RESULT_WAIT_BIT:
            return "VK_QUERY_RESULT_WAIT_BIT";
        case VK_QUERY_RESULT_WITH_AVAILABILITY_BIT:
            return "VK_QUERY_RESULT_WITH_AVAILABILITY_BIT";
        default:
            return "Unhandled VkQueryResultFlagBits";
    }
}


static inline const char* string_VkQueryType(VkQueryType input_value)
{
    switch ((VkQueryType)input_value)
    {
        case VK_QUERY_TYPE_OCCLUSION:
            return "VK_QUERY_TYPE_OCCLUSION";
        case VK_QUERY_TYPE_PIPELINE_STATISTICS:
            return "VK_QUERY_TYPE_PIPELINE_STATISTICS";
        case VK_QUERY_TYPE_TIMESTAMP:
            return "VK_QUERY_TYPE_TIMESTAMP";
        default:
            return "Unhandled VkQueryType";
    }
}


static inline const char* string_VkQueueFlagBits(VkQueueFlagBits input_value)
{
    switch ((VkQueueFlagBits)input_value)
    {
        case VK_QUEUE_COMPUTE_BIT:
            return "VK_QUEUE_COMPUTE_BIT";
        case VK_QUEUE_GRAPHICS_BIT:
            return "VK_QUEUE_GRAPHICS_BIT";
        case VK_QUEUE_SPARSE_BINDING_BIT:
            return "VK_QUEUE_SPARSE_BINDING_BIT";
        case VK_QUEUE_TRANSFER_BIT:
            return "VK_QUEUE_TRANSFER_BIT";
        default:
            return "Unhandled VkQueueFlagBits";
    }
}


static inline const char* string_VkRasterizationOrderAMD(VkRasterizationOrderAMD input_value)
{
    switch ((VkRasterizationOrderAMD)input_value)
    {
        case VK_RASTERIZATION_ORDER_RELAXED_AMD:
            return "VK_RASTERIZATION_ORDER_RELAXED_AMD";
        case VK_RASTERIZATION_ORDER_STRICT_AMD:
            return "VK_RASTERIZATION_ORDER_STRICT_AMD";
        default:
            return "Unhandled VkRasterizationOrderAMD";
    }
}


static inline const char* string_VkResult(VkResult input_value)
{
    switch ((VkResult)input_value)
    {
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        default:
            return "Unhandled VkResult";
    }
}


static inline const char* string_VkSampleCountFlagBits(VkSampleCountFlagBits input_value)
{
    switch ((VkSampleCountFlagBits)input_value)
    {
        case VK_SAMPLE_COUNT_16_BIT:
            return "VK_SAMPLE_COUNT_16_BIT";
        case VK_SAMPLE_COUNT_1_BIT:
            return "VK_SAMPLE_COUNT_1_BIT";
        case VK_SAMPLE_COUNT_2_BIT:
            return "VK_SAMPLE_COUNT_2_BIT";
        case VK_SAMPLE_COUNT_32_BIT:
            return "VK_SAMPLE_COUNT_32_BIT";
        case VK_SAMPLE_COUNT_4_BIT:
            return "VK_SAMPLE_COUNT_4_BIT";
        case VK_SAMPLE_COUNT_64_BIT:
            return "VK_SAMPLE_COUNT_64_BIT";
        case VK_SAMPLE_COUNT_8_BIT:
            return "VK_SAMPLE_COUNT_8_BIT";
        default:
            return "Unhandled VkSampleCountFlagBits";
    }
}


static inline const char* string_VkSamplerAddressMode(VkSamplerAddressMode input_value)
{
    switch ((VkSamplerAddressMode)input_value)
    {
        case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
            return "VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER";
        case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
            return "VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE";
        case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
            return "VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT";
        case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
            return "VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE";
        case VK_SAMPLER_ADDRESS_MODE_REPEAT:
            return "VK_SAMPLER_ADDRESS_MODE_REPEAT";
        default:
            return "Unhandled VkSamplerAddressMode";
    }
}


static inline const char* string_VkSamplerMipmapMode(VkSamplerMipmapMode input_value)
{
    switch ((VkSamplerMipmapMode)input_value)
    {
        case VK_SAMPLER_MIPMAP_MODE_LINEAR:
            return "VK_SAMPLER_MIPMAP_MODE_LINEAR";
        case VK_SAMPLER_MIPMAP_MODE_NEAREST:
            return "VK_SAMPLER_MIPMAP_MODE_NEAREST";
        default:
            return "Unhandled VkSamplerMipmapMode";
    }
}


static inline const char* string_VkShaderStageFlagBits(VkShaderStageFlagBits input_value)
{
    switch ((VkShaderStageFlagBits)input_value)
    {
        case VK_SHADER_STAGE_ALL:
            return "VK_SHADER_STAGE_ALL";
        case VK_SHADER_STAGE_ALL_GRAPHICS:
            return "VK_SHADER_STAGE_ALL_GRAPHICS";
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return "VK_SHADER_STAGE_COMPUTE_BIT";
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return "VK_SHADER_STAGE_FRAGMENT_BIT";
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return "VK_SHADER_STAGE_GEOMETRY_BIT";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT";
        case VK_SHADER_STAGE_VERTEX_BIT:
            return "VK_SHADER_STAGE_VERTEX_BIT";
        default:
            return "Unhandled VkShaderStageFlagBits";
    }
}


static inline const char* string_VkSharingMode(VkSharingMode input_value)
{
    switch ((VkSharingMode)input_value)
    {
        case VK_SHARING_MODE_CONCURRENT:
            return "VK_SHARING_MODE_CONCURRENT";
        case VK_SHARING_MODE_EXCLUSIVE:
            return "VK_SHARING_MODE_EXCLUSIVE";
        default:
            return "Unhandled VkSharingMode";
    }
}


static inline const char* string_VkSparseImageFormatFlagBits(VkSparseImageFormatFlagBits input_value)
{
    switch ((VkSparseImageFormatFlagBits)input_value)
    {
        case VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT:
            return "VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT";
        case VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT:
            return "VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT";
        case VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT:
            return "VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT";
        default:
            return "Unhandled VkSparseImageFormatFlagBits";
    }
}


static inline const char* string_VkSparseMemoryBindFlagBits(VkSparseMemoryBindFlagBits input_value)
{
    switch ((VkSparseMemoryBindFlagBits)input_value)
    {
        case VK_SPARSE_MEMORY_BIND_METADATA_BIT:
            return "VK_SPARSE_MEMORY_BIND_METADATA_BIT";
        default:
            return "Unhandled VkSparseMemoryBindFlagBits";
    }
}


static inline const char* string_VkStencilFaceFlagBits(VkStencilFaceFlagBits input_value)
{
    switch ((VkStencilFaceFlagBits)input_value)
    {
        case VK_STENCIL_FACE_BACK_BIT:
            return "VK_STENCIL_FACE_BACK_BIT";
        case VK_STENCIL_FACE_FRONT_BIT:
            return "VK_STENCIL_FACE_FRONT_BIT";
        case VK_STENCIL_FRONT_AND_BACK:
            return "VK_STENCIL_FRONT_AND_BACK";
        default:
            return "Unhandled VkStencilFaceFlagBits";
    }
}


static inline const char* string_VkStencilOp(VkStencilOp input_value)
{
    switch ((VkStencilOp)input_value)
    {
        case VK_STENCIL_OP_DECREMENT_AND_CLAMP:
            return "VK_STENCIL_OP_DECREMENT_AND_CLAMP";
        case VK_STENCIL_OP_DECREMENT_AND_WRAP:
            return "VK_STENCIL_OP_DECREMENT_AND_WRAP";
        case VK_STENCIL_OP_INCREMENT_AND_CLAMP:
            return "VK_STENCIL_OP_INCREMENT_AND_CLAMP";
        case VK_STENCIL_OP_INCREMENT_AND_WRAP:
            return "VK_STENCIL_OP_INCREMENT_AND_WRAP";
        case VK_STENCIL_OP_INVERT:
            return "VK_STENCIL_OP_INVERT";
        case VK_STENCIL_OP_KEEP:
            return "VK_STENCIL_OP_KEEP";
        case VK_STENCIL_OP_REPLACE:
            return "VK_STENCIL_OP_REPLACE";
        case VK_STENCIL_OP_ZERO:
            return "VK_STENCIL_OP_ZERO";
        default:
            return "Unhandled VkStencilOp";
    }
}


static inline const char* string_VkStructureType(VkStructureType input_value)
{
    switch ((VkStructureType)input_value)
    {
        case VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_APPLICATION_INFO:
            return "VK_STRUCTURE_TYPE_APPLICATION_INFO";
        case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
            return "VK_STRUCTURE_TYPE_BIND_SPARSE_INFO";
        case VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO";
        case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            return "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER";
        case VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO";
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO:
            return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO";
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO:
            return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO";
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO:
            return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO";
        case VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO";
        case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            return "VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET";
        case VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT:
            return "VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT";
        case VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT:
            return "VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT";
        case VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT:
            return "VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT";
        case VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT:
            return "VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT";
        case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV:
            return "VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV";
        case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV:
            return "VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV";
        case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV:
            return "VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV";
        case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO";
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO:
            return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO";
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO";
        case VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR:
            return "VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR";
        case VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_EVENT_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_EVENT_CREATE_INFO";
        case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV:
            return "VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV";
        case VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV:
            return "VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV";
        case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV:
            return "VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV";
        case VK_STRUCTURE_TYPE_FENCE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_FENCE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO";
        case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
            return "VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER";
        case VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO";
        case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV:
            return "VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV";
        case VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE:
            return "VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE";
        case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO:
            return "VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO";
        case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
            return "VK_STRUCTURE_TYPE_MEMORY_BARRIER";
        case VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD:
            return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD";
        case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_PRESENT_INFO_KHR:
            return "VK_STRUCTURE_TYPE_PRESENT_INFO_KHR";
        case VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO";
        case VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO:
            return "VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO";
        case VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO";
        case VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO";
        case VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO:
            return "VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO";
        case VK_STRUCTURE_TYPE_SUBMIT_INFO:
            return "VK_STRUCTURE_TYPE_SUBMIT_INFO";
        case VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT:
            return "VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT";
        case VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV:
            return "VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV";
        case VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            return "VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET";
        case VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR";
        case VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR:
            return "VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR";
        default:
            return "Unhandled VkStructureType";
    }
}


static inline const char* string_VkSubpassContents(VkSubpassContents input_value)
{
    switch ((VkSubpassContents)input_value)
    {
        case VK_SUBPASS_CONTENTS_INLINE:
            return "VK_SUBPASS_CONTENTS_INLINE";
        case VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS:
            return "VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS";
        default:
            return "Unhandled VkSubpassContents";
    }
}


static inline const char* string_VkSurfaceTransformFlagBitsKHR(VkSurfaceTransformFlagBitsKHR input_value)
{
    switch ((VkSurfaceTransformFlagBitsKHR)input_value)
    {
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR";
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR";
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR";
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR";
        case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR";
        case VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR";
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR";
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR";
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
            return "VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR";
        default:
            return "Unhandled VkSurfaceTransformFlagBitsKHR";
    }
}


static inline const char* string_VkSystemAllocationScope(VkSystemAllocationScope input_value)
{
    switch ((VkSystemAllocationScope)input_value)
    {
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_CACHE";
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            return "VK_SYSTEM_ALLOCATION_SCOPE_COMMAND";
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_DEVICE";
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE";
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            return "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT";
        default:
            return "Unhandled VkSystemAllocationScope";
    }
}


static inline const char* string_VkValidationCheckEXT(VkValidationCheckEXT input_value)
{
    switch ((VkValidationCheckEXT)input_value)
    {
        case VK_VALIDATION_CHECK_ALL_EXT:
            return "VK_VALIDATION_CHECK_ALL_EXT";
        default:
            return "Unhandled VkValidationCheckEXT";
    }
}


static inline const char* string_VkVertexInputRate(VkVertexInputRate input_value)
{
    switch ((VkVertexInputRate)input_value)
    {
        case VK_VERTEX_INPUT_RATE_INSTANCE:
            return "VK_VERTEX_INPUT_RATE_INSTANCE";
        case VK_VERTEX_INPUT_RATE_VERTEX:
            return "VK_VERTEX_INPUT_RATE_VERTEX";
        default:
            return "Unhandled VkVertexInputRate";
    }
}

