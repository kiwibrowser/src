#pragma once

#ifdef _WIN32

#pragma warning( disable : 4065 )

#endif

#include <vulkan/vulkan.h>


static inline uint32_t validate_VkAccessFlagBits(VkAccessFlagBits input_value)
{
    if (input_value > (VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkAttachmentDescriptionFlagBits(VkAttachmentDescriptionFlagBits input_value)
{
    if (input_value > (VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkAttachmentLoadOp(VkAttachmentLoadOp input_value)
{
    switch ((VkAttachmentLoadOp)input_value)
    {
        case VK_ATTACHMENT_LOAD_OP_CLEAR:
        case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
        case VK_ATTACHMENT_LOAD_OP_LOAD:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkAttachmentStoreOp(VkAttachmentStoreOp input_value)
{
    switch ((VkAttachmentStoreOp)input_value)
    {
        case VK_ATTACHMENT_STORE_OP_DONT_CARE:
        case VK_ATTACHMENT_STORE_OP_STORE:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkBlendFactor(VkBlendFactor input_value)
{
    switch ((VkBlendFactor)input_value)
    {
        case VK_BLEND_FACTOR_CONSTANT_ALPHA:
        case VK_BLEND_FACTOR_CONSTANT_COLOR:
        case VK_BLEND_FACTOR_DST_ALPHA:
        case VK_BLEND_FACTOR_DST_COLOR:
        case VK_BLEND_FACTOR_ONE:
        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
        case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
        case VK_BLEND_FACTOR_SRC1_ALPHA:
        case VK_BLEND_FACTOR_SRC1_COLOR:
        case VK_BLEND_FACTOR_SRC_ALPHA:
        case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
        case VK_BLEND_FACTOR_SRC_COLOR:
        case VK_BLEND_FACTOR_ZERO:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkBlendOp(VkBlendOp input_value)
{
    switch ((VkBlendOp)input_value)
    {
        case VK_BLEND_OP_ADD:
        case VK_BLEND_OP_MAX:
        case VK_BLEND_OP_MIN:
        case VK_BLEND_OP_REVERSE_SUBTRACT:
        case VK_BLEND_OP_SUBTRACT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkBorderColor(VkBorderColor input_value)
{
    switch ((VkBorderColor)input_value)
    {
        case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
        case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
        case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
        case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
        case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
        case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkBufferCreateFlagBits(VkBufferCreateFlagBits input_value)
{
    if (input_value > (VK_BUFFER_CREATE_SPARSE_BINDING_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_ALIASED_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkBufferUsageFlagBits(VkBufferUsageFlagBits input_value)
{
    if (input_value > (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkColorComponentFlagBits(VkColorComponentFlagBits input_value)
{
    if (input_value > (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkColorSpaceKHR(VkColorSpaceKHR input_value)
{
    switch ((VkColorSpaceKHR)input_value)
    {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkCommandBufferLevel(VkCommandBufferLevel input_value)
{
    switch ((VkCommandBufferLevel)input_value)
    {
        case VK_COMMAND_BUFFER_LEVEL_PRIMARY:
        case VK_COMMAND_BUFFER_LEVEL_SECONDARY:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkCommandBufferResetFlagBits(VkCommandBufferResetFlagBits input_value)
{
    if (input_value > (VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkCommandBufferUsageFlagBits(VkCommandBufferUsageFlagBits input_value)
{
    if (input_value > (VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkCommandPoolCreateFlagBits(VkCommandPoolCreateFlagBits input_value)
{
    if (input_value > (VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkCommandPoolResetFlagBits(VkCommandPoolResetFlagBits input_value)
{
    if (input_value > (VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkCompareOp(VkCompareOp input_value)
{
    switch ((VkCompareOp)input_value)
    {
        case VK_COMPARE_OP_ALWAYS:
        case VK_COMPARE_OP_EQUAL:
        case VK_COMPARE_OP_GREATER:
        case VK_COMPARE_OP_GREATER_OR_EQUAL:
        case VK_COMPARE_OP_LESS:
        case VK_COMPARE_OP_LESS_OR_EQUAL:
        case VK_COMPARE_OP_NEVER:
        case VK_COMPARE_OP_NOT_EQUAL:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkComponentSwizzle(VkComponentSwizzle input_value)
{
    switch ((VkComponentSwizzle)input_value)
    {
        case VK_COMPONENT_SWIZZLE_A:
        case VK_COMPONENT_SWIZZLE_B:
        case VK_COMPONENT_SWIZZLE_G:
        case VK_COMPONENT_SWIZZLE_IDENTITY:
        case VK_COMPONENT_SWIZZLE_ONE:
        case VK_COMPONENT_SWIZZLE_R:
        case VK_COMPONENT_SWIZZLE_ZERO:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkCompositeAlphaFlagBitsKHR(VkCompositeAlphaFlagBitsKHR input_value)
{
    if (input_value > (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR | VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR))
        return 0;
    return 1;
}


static inline uint32_t validate_VkCullModeFlagBits(VkCullModeFlagBits input_value)
{
    if (input_value > (VK_CULL_MODE_NONE | VK_CULL_MODE_FRONT_BIT | VK_CULL_MODE_BACK_BIT | VK_CULL_MODE_FRONT_AND_BACK))
        return 0;
    return 1;
}


static inline uint32_t validate_VkDebugReportErrorEXT(VkDebugReportErrorEXT input_value)
{
    switch ((VkDebugReportErrorEXT)input_value)
    {
        case VK_DEBUG_REPORT_ERROR_CALLBACK_REF_EXT:
        case VK_DEBUG_REPORT_ERROR_NONE_EXT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkDebugReportFlagBitsEXT(VkDebugReportFlagBitsEXT input_value)
{
    if (input_value > (VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkDebugReportObjectTypeEXT(VkDebugReportObjectTypeEXT input_value)
{
    switch ((VkDebugReportObjectTypeEXT)input_value)
    {
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
        case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkDependencyFlagBits(VkDependencyFlagBits input_value)
{
    if (input_value > (VK_DEPENDENCY_BY_REGION_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkDescriptorPoolCreateFlagBits(VkDescriptorPoolCreateFlagBits input_value)
{
    if (input_value > (VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkDescriptorType(VkDescriptorType input_value)
{
    switch ((VkDescriptorType)input_value)
    {
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkDisplayPlaneAlphaFlagBitsKHR(VkDisplayPlaneAlphaFlagBitsKHR input_value)
{
    if (input_value > (VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR | VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR | VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR | VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR))
        return 0;
    return 1;
}


static inline uint32_t validate_VkDynamicState(VkDynamicState input_value)
{
    switch ((VkDynamicState)input_value)
    {
        case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
        case VK_DYNAMIC_STATE_DEPTH_BIAS:
        case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
        case VK_DYNAMIC_STATE_LINE_WIDTH:
        case VK_DYNAMIC_STATE_SCISSOR:
        case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
        case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
        case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
        case VK_DYNAMIC_STATE_VIEWPORT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkExternalMemoryFeatureFlagBitsNV(VkExternalMemoryFeatureFlagBitsNV input_value)
{
    if (input_value > (VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV | VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV | VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV))
        return 0;
    return 1;
}


static inline uint32_t validate_VkExternalMemoryHandleTypeFlagBitsNV(VkExternalMemoryHandleTypeFlagBitsNV input_value)
{
    if (input_value > (VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV | VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV | VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV | VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV))
        return 0;
    return 1;
}


static inline uint32_t validate_VkFenceCreateFlagBits(VkFenceCreateFlagBits input_value)
{
    if (input_value > (VK_FENCE_CREATE_SIGNALED_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkFilter(VkFilter input_value)
{
    switch ((VkFilter)input_value)
    {
        case VK_FILTER_CUBIC_IMG:
        case VK_FILTER_LINEAR:
        case VK_FILTER_NEAREST:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkFormat(VkFormat input_value)
{
    switch ((VkFormat)input_value)
    {
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_R4G4_UNORM_PACK8:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64_SFLOAT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64_SFLOAT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_S8_UINT:
        case VK_FORMAT_UNDEFINED:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkFormatFeatureFlagBits(VkFormatFeatureFlagBits input_value)
{
    if (input_value > (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT | VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT | VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT | VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG))
        return 0;
    return 1;
}


static inline uint32_t validate_VkFrontFace(VkFrontFace input_value)
{
    switch ((VkFrontFace)input_value)
    {
        case VK_FRONT_FACE_CLOCKWISE:
        case VK_FRONT_FACE_COUNTER_CLOCKWISE:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkImageAspectFlagBits(VkImageAspectFlagBits input_value)
{
    if (input_value > (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkImageCreateFlagBits(VkImageCreateFlagBits input_value)
{
    if (input_value > (VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_ALIASED_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkImageLayout(VkImageLayout input_value)
{
    switch ((VkImageLayout)input_value)
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        case VK_IMAGE_LAYOUT_GENERAL:
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkImageTiling(VkImageTiling input_value)
{
    switch ((VkImageTiling)input_value)
    {
        case VK_IMAGE_TILING_LINEAR:
        case VK_IMAGE_TILING_OPTIMAL:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkImageType(VkImageType input_value)
{
    switch ((VkImageType)input_value)
    {
        case VK_IMAGE_TYPE_1D:
        case VK_IMAGE_TYPE_2D:
        case VK_IMAGE_TYPE_3D:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkImageUsageFlagBits(VkImageUsageFlagBits input_value)
{
    if (input_value > (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkImageViewType(VkImageViewType input_value)
{
    switch ((VkImageViewType)input_value)
    {
        case VK_IMAGE_VIEW_TYPE_1D:
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
        case VK_IMAGE_VIEW_TYPE_2D:
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        case VK_IMAGE_VIEW_TYPE_3D:
        case VK_IMAGE_VIEW_TYPE_CUBE:
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkIndexType(VkIndexType input_value)
{
    switch ((VkIndexType)input_value)
    {
        case VK_INDEX_TYPE_UINT16:
        case VK_INDEX_TYPE_UINT32:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkInternalAllocationType(VkInternalAllocationType input_value)
{
    switch ((VkInternalAllocationType)input_value)
    {
        case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkLogicOp(VkLogicOp input_value)
{
    switch ((VkLogicOp)input_value)
    {
        case VK_LOGIC_OP_AND:
        case VK_LOGIC_OP_AND_INVERTED:
        case VK_LOGIC_OP_AND_REVERSE:
        case VK_LOGIC_OP_CLEAR:
        case VK_LOGIC_OP_COPY:
        case VK_LOGIC_OP_COPY_INVERTED:
        case VK_LOGIC_OP_EQUIVALENT:
        case VK_LOGIC_OP_INVERT:
        case VK_LOGIC_OP_NAND:
        case VK_LOGIC_OP_NOR:
        case VK_LOGIC_OP_NO_OP:
        case VK_LOGIC_OP_OR:
        case VK_LOGIC_OP_OR_INVERTED:
        case VK_LOGIC_OP_OR_REVERSE:
        case VK_LOGIC_OP_SET:
        case VK_LOGIC_OP_XOR:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkMemoryHeapFlagBits(VkMemoryHeapFlagBits input_value)
{
    if (input_value > (VK_MEMORY_HEAP_DEVICE_LOCAL_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkMemoryPropertyFlagBits(VkMemoryPropertyFlagBits input_value)
{
    if (input_value > (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkPhysicalDeviceType(VkPhysicalDeviceType input_value)
{
    switch ((VkPhysicalDeviceType)input_value)
    {
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkPipelineBindPoint(VkPipelineBindPoint input_value)
{
    switch ((VkPipelineBindPoint)input_value)
    {
        case VK_PIPELINE_BIND_POINT_COMPUTE:
        case VK_PIPELINE_BIND_POINT_GRAPHICS:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkPipelineCacheHeaderVersion(VkPipelineCacheHeaderVersion input_value)
{
    switch ((VkPipelineCacheHeaderVersion)input_value)
    {
        case VK_PIPELINE_CACHE_HEADER_VERSION_ONE:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkPipelineCreateFlagBits(VkPipelineCreateFlagBits input_value)
{
    if (input_value > (VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT | VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT | VK_PIPELINE_CREATE_DERIVATIVE_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkPipelineStageFlagBits(VkPipelineStageFlagBits input_value)
{
    if (input_value > (VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkPolygonMode(VkPolygonMode input_value)
{
    switch ((VkPolygonMode)input_value)
    {
        case VK_POLYGON_MODE_FILL:
        case VK_POLYGON_MODE_LINE:
        case VK_POLYGON_MODE_POINT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkPresentModeKHR(VkPresentModeKHR input_value)
{
    switch ((VkPresentModeKHR)input_value)
    {
        case VK_PRESENT_MODE_FIFO_KHR:
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkPrimitiveTopology(VkPrimitiveTopology input_value)
{
    switch ((VkPrimitiveTopology)input_value)
    {
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
        case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkQueryControlFlagBits(VkQueryControlFlagBits input_value)
{
    if (input_value > (VK_QUERY_CONTROL_PRECISE_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkQueryPipelineStatisticFlagBits(VkQueryPipelineStatisticFlagBits input_value)
{
    if (input_value > (VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT | VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT | VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkQueryResultFlagBits(VkQueryResultFlagBits input_value)
{
    if (input_value > (VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT | VK_QUERY_RESULT_PARTIAL_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkQueryType(VkQueryType input_value)
{
    switch ((VkQueryType)input_value)
    {
        case VK_QUERY_TYPE_OCCLUSION:
        case VK_QUERY_TYPE_PIPELINE_STATISTICS:
        case VK_QUERY_TYPE_TIMESTAMP:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkQueueFlagBits(VkQueueFlagBits input_value)
{
    if (input_value > (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkRasterizationOrderAMD(VkRasterizationOrderAMD input_value)
{
    switch ((VkRasterizationOrderAMD)input_value)
    {
        case VK_RASTERIZATION_ORDER_RELAXED_AMD:
        case VK_RASTERIZATION_ORDER_STRICT_AMD:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkResult(VkResult input_value)
{
    switch ((VkResult)input_value)
    {
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_INITIALIZATION_FAILED:
        case VK_ERROR_INVALID_SHADER_NV:
        case VK_ERROR_LAYER_NOT_PRESENT:
        case VK_ERROR_MEMORY_MAP_FAILED:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_TOO_MANY_OBJECTS:
        case VK_ERROR_VALIDATION_FAILED_EXT:
        case VK_EVENT_RESET:
        case VK_EVENT_SET:
        case VK_INCOMPLETE:
        case VK_NOT_READY:
        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
        case VK_TIMEOUT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkSampleCountFlagBits(VkSampleCountFlagBits input_value)
{
    if (input_value > (VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT | VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkSamplerAddressMode(VkSamplerAddressMode input_value)
{
    switch ((VkSamplerAddressMode)input_value)
    {
        case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
        case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
        case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
        case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
        case VK_SAMPLER_ADDRESS_MODE_REPEAT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkSamplerMipmapMode(VkSamplerMipmapMode input_value)
{
    switch ((VkSamplerMipmapMode)input_value)
    {
        case VK_SAMPLER_MIPMAP_MODE_LINEAR:
        case VK_SAMPLER_MIPMAP_MODE_NEAREST:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkShaderStageFlagBits(VkShaderStageFlagBits input_value)
{
    if (input_value > (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_ALL))
        return 0;
    return 1;
}


static inline uint32_t validate_VkSharingMode(VkSharingMode input_value)
{
    switch ((VkSharingMode)input_value)
    {
        case VK_SHARING_MODE_CONCURRENT:
        case VK_SHARING_MODE_EXCLUSIVE:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkSparseImageFormatFlagBits(VkSparseImageFormatFlagBits input_value)
{
    if (input_value > (VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT | VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT | VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkSparseMemoryBindFlagBits(VkSparseMemoryBindFlagBits input_value)
{
    if (input_value > (VK_SPARSE_MEMORY_BIND_METADATA_BIT))
        return 0;
    return 1;
}


static inline uint32_t validate_VkStencilFaceFlagBits(VkStencilFaceFlagBits input_value)
{
    if (input_value > (VK_STENCIL_FACE_FRONT_BIT | VK_STENCIL_FACE_BACK_BIT | VK_STENCIL_FRONT_AND_BACK))
        return 0;
    return 1;
}


static inline uint32_t validate_VkStencilOp(VkStencilOp input_value)
{
    switch ((VkStencilOp)input_value)
    {
        case VK_STENCIL_OP_DECREMENT_AND_CLAMP:
        case VK_STENCIL_OP_DECREMENT_AND_WRAP:
        case VK_STENCIL_OP_INCREMENT_AND_CLAMP:
        case VK_STENCIL_OP_INCREMENT_AND_WRAP:
        case VK_STENCIL_OP_INVERT:
        case VK_STENCIL_OP_KEEP:
        case VK_STENCIL_OP_REPLACE:
        case VK_STENCIL_OP_ZERO:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkStructureType(VkStructureType input_value)
{
    switch ((VkStructureType)input_value)
    {
        case VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_APPLICATION_INFO:
        case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
        case VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
        case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
        case VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO:
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO:
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO:
        case VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO:
        case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
        case VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT:
        case VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT:
        case VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT:
        case VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT:
        case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV:
        case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV:
        case VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV:
        case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO:
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO:
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
        case VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR:
        case VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV:
        case VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV:
        case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV:
        case VK_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
        case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
        case VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV:
        case VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE:
        case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO:
        case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
        case VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD:
        case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_PRESENT_INFO_KHR:
        case VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        case VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO:
        case VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
        case VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        case VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_SUBMIT_INFO:
        case VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT:
        case VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV:
        case VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        case VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR:
        case VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkSubpassContents(VkSubpassContents input_value)
{
    switch ((VkSubpassContents)input_value)
    {
        case VK_SUBPASS_CONTENTS_INLINE:
        case VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkSurfaceTransformFlagBitsKHR(VkSurfaceTransformFlagBitsKHR input_value)
{
    if (input_value > (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR | VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR | VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR | VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR | VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR))
        return 0;
    return 1;
}


static inline uint32_t validate_VkSystemAllocationScope(VkSystemAllocationScope input_value)
{
    switch ((VkSystemAllocationScope)input_value)
    {
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkValidationCheckEXT(VkValidationCheckEXT input_value)
{
    switch ((VkValidationCheckEXT)input_value)
    {
        case VK_VALIDATION_CHECK_ALL_EXT:
            return 1;
        default:
            return 0;
    }
}


static inline uint32_t validate_VkVertexInputRate(VkVertexInputRate input_value)
{
    switch ((VkVertexInputRate)input_value)
    {
        case VK_VERTEX_INPUT_RATE_INSTANCE:
        case VK_VERTEX_INPUT_RATE_VERTEX:
            return 1;
        default:
            return 0;
    }
}

