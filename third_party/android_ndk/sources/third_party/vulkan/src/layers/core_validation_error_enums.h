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
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */
#ifndef CORE_VALIDATION_ERROR_ENUMS_H_
#define CORE_VALIDATION_ERROR_ENUMS_H_

// Mem Tracker ERROR codes
enum MEM_TRACK_ERROR {
    MEMTRACK_NONE,                         // Used for INFO & other non-error messages
    MEMTRACK_INVALID_CB,                   // Cmd Buffer invalid
    MEMTRACK_INVALID_MEM_OBJ,              // Invalid Memory Object
    MEMTRACK_INVALID_ALIASING,             // Invalid Memory Aliasing
    MEMTRACK_INTERNAL_ERROR,               // Bug in Mem Track Layer internal data structures
    MEMTRACK_FREED_MEM_REF,                // MEM Obj freed while it still has obj and/or CB refs
    MEMTRACK_INVALID_OBJECT,               // Attempting to reference generic VK Object that is invalid
    MEMTRACK_MEMORY_LEAK,                  // Failure to call vkFreeMemory on Mem Obj prior to DestroyDevice
    MEMTRACK_INVALID_STATE,                // Memory not in the correct state
    MEMTRACK_RESET_CB_WHILE_IN_FLIGHT,     // vkResetCommandBuffer() called on a CB that hasn't completed
    MEMTRACK_INVALID_FENCE_STATE,          // Invalid Fence State signaled or used
    MEMTRACK_REBIND_OBJECT,                // Non-sparse object bindings are immutable
    MEMTRACK_INVALID_USAGE_FLAG,           // Usage flags specified at image/buffer create conflict w/ use of object
    MEMTRACK_INVALID_MAP,                  // Size flag specified at alloc is too small for mapping range
    MEMTRACK_INVALID_MEM_TYPE,             // Memory Type mismatch
    MEMTRACK_INVALID_MEM_REGION,           // Memory region for object bound to an allocation is invalid
    MEMTRACK_OBJECT_NOT_BOUND,             // Image or Buffer used without having memory bound to it
};

// Draw State ERROR codes
enum DRAW_STATE_ERROR {
    // TODO: Remove the comments here or expand them. There isn't any additional information in the
    // comments than in the name in almost all cases.
    DRAWSTATE_NONE,                          // Used for INFO & other non-error messages
    DRAWSTATE_INTERNAL_ERROR,                // Error with DrawState internal data structures
    DRAWSTATE_NO_PIPELINE_BOUND,             // Unable to identify a bound pipeline
    DRAWSTATE_INVALID_SET,                   // Invalid DS
    DRAWSTATE_INVALID_RENDER_AREA,           // Invalid renderArea
    DRAWSTATE_INVALID_LAYOUT,                // Invalid DS layout
    DRAWSTATE_INVALID_IMAGE_LAYOUT,          // Invalid Image layout
    DRAWSTATE_INVALID_PIPELINE,              // Invalid Pipeline handle referenced
    DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, // Attempt to create a pipeline
                                             // with invalid state
    DRAWSTATE_INVALID_COMMAND_BUFFER,        // Invalid CommandBuffer referenced
    DRAWSTATE_INVALID_BARRIER,               // Invalid Barrier
    DRAWSTATE_INVALID_BUFFER,                // Invalid Buffer
    DRAWSTATE_INVALID_IMAGE,                 // Invalid Image
    DRAWSTATE_INVALID_BUFFER_VIEW,           // Invalid BufferView
    DRAWSTATE_INVALID_IMAGE_VIEW,            // Invalid ImageView
    DRAWSTATE_INVALID_QUERY,                 // Invalid Query
    DRAWSTATE_INVALID_QUERY_POOL,            // Invalid QueryPool
    DRAWSTATE_INVALID_DESCRIPTOR_POOL,       // Invalid DescriptorPool
    DRAWSTATE_INVALID_COMMAND_POOL,          // Invalid CommandPool
    DRAWSTATE_INVALID_FENCE,                 // Invalid Fence
    DRAWSTATE_INVALID_EVENT,                 // Invalid Event
    DRAWSTATE_INVALID_SAMPLER,               // Invalid Sampler
    DRAWSTATE_INVALID_FRAMEBUFFER,           // Invalid Framebuffer
    DRAWSTATE_INVALID_DEVICE_MEMORY,         // Invalid DeviceMemory
    DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS,       // binding in vkCmdBindVertexData() too
                                             // large for PSO's
                                             // pVertexBindingDescriptions array
    DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR,     // binding offset in
                                             // vkCmdBindIndexBuffer() out of
                                             // alignment based on indexType
    // DRAWSTATE_MISSING_DOT_PROGRAM,              // No "dot" program in order
    // to generate png image
    DRAWSTATE_OUT_OF_MEMORY,                          // malloc failed
    DRAWSTATE_INVALID_DESCRIPTOR_SET,                 // Descriptor Set handle is unknown
    DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH,               // Type in layout vs. update are not the
                                                      // same
    DRAWSTATE_DESCRIPTOR_STAGEFLAGS_MISMATCH,         // StageFlags in layout are not
                                                      // the same throughout a single
                                                      // VkWriteDescriptorSet update
    DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS,        // Descriptors set for update out
                                                      // of bounds for corresponding
                                                      // layout section
    DRAWSTATE_DESCRIPTOR_POOL_EMPTY,                  // Attempt to allocate descriptor from a
                                                      // pool with no more descriptors of that
                                                      // type available
    DRAWSTATE_CANT_FREE_FROM_NON_FREE_POOL,           // Invalid to call
                                                      // vkFreeDescriptorSets on Sets
                                                      // allocated from a NON_FREE Pool
    DRAWSTATE_INVALID_WRITE_UPDATE,                   // Attempting a write update to a descriptor
                                                      // set with invalid update state
    DRAWSTATE_INVALID_COPY_UPDATE,                    // Attempting copy update to a descriptor set
                                                      // with invalid state
    DRAWSTATE_INVALID_UPDATE_STRUCT,                  // Struct in DS Update tree is of invalid
                                                      // type
    DRAWSTATE_NUM_SAMPLES_MISMATCH,                   // Number of samples in bound PSO does not
                                                      // match number in FB of current RenderPass
    DRAWSTATE_NO_END_COMMAND_BUFFER,                  // Must call vkEndCommandBuffer() before
                                                      // QueueSubmit on that commandBuffer
    DRAWSTATE_NO_BEGIN_COMMAND_BUFFER,                // Binding cmds or calling End on CB that
                                                      // never had vkBeginCommandBuffer()
                                                      // called on it
    DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, // Cmd Buffer created with
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    // flag is submitted
    // multiple times
    DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, // vkCmdExecuteCommands() called
                                                // with a primary commandBuffer
                                                // in pCommandBuffers array
    DRAWSTATE_VIEWPORT_NOT_BOUND,               // Draw submitted with no viewport state bound
    DRAWSTATE_SCISSOR_NOT_BOUND,                // Draw submitted with no scissor state bound
    DRAWSTATE_LINE_WIDTH_NOT_BOUND,             // Draw submitted with no line width state
                                                // bound
    DRAWSTATE_DEPTH_BIAS_NOT_BOUND,             // Draw submitted with no depth bias state
                                                // bound
    DRAWSTATE_BLEND_NOT_BOUND,                  // Draw submitted with no blend state bound when
                                                // color write enabled
    DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND,           // Draw submitted with no depth bounds
                                                // state bound when depth enabled
    DRAWSTATE_STENCIL_NOT_BOUND,                // Draw submitted with no stencil state bound
                                                // when stencil enabled
    DRAWSTATE_INDEX_BUFFER_NOT_BOUND,           // Draw submitted with no depth-stencil
                                                // state bound when depth write enabled
    DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE,    // Draw submitted PSO Pipeline
                                                // layout that's not compatible
                                                // with layout from
                                                // BindDescriptorSets
    DRAWSTATE_RENDERPASS_INCOMPATIBLE,          // Incompatible renderpasses between
                                                // secondary cmdBuffer and primary
                                                // cmdBuffer or framebuffer
    DRAWSTATE_FRAMEBUFFER_INCOMPATIBLE,         // Incompatible framebuffer between
                                                // secondary cmdBuffer and active
                                                // renderPass
    DRAWSTATE_INVALID_FRAMEBUFFER_CREATE_INFO,  // Invalid VkFramebufferCreateInfo state
    DRAWSTATE_INVALID_RENDERPASS,               // Use of a NULL or otherwise invalid
                                                // RenderPass object
    DRAWSTATE_INVALID_RENDERPASS_CMD,           // Invalid cmd submitted while a
                                                // RenderPass is active
    DRAWSTATE_NO_ACTIVE_RENDERPASS,             // Rendering cmd submitted without an active
                                                // RenderPass
    DRAWSTATE_INVALID_IMAGE_USAGE,              // Image attachment location conflicts with
                                                // image's USAGE flags
    DRAWSTATE_INVALID_ATTACHMENT_INDEX,         // Attachment reference contains an index
                                                // that is out-of-bounds
    DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED,       // DescriptorSet bound but it was
                                                // never updated. This is a warning
                                                // code.
    DRAWSTATE_DESCRIPTOR_SET_NOT_BOUND,         // DescriptorSet used by pipeline at
                                                // draw time is not bound, or has been
                                                // disturbed (which would have flagged
                                                // previous warning)
    DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT,     // DescriptorSets bound with
                                                // different number of dynamic
                                                // descriptors that were included in
                                                // dynamicOffsetCount
    DRAWSTATE_CLEAR_CMD_BEFORE_DRAW,            // Clear cmd issued before any Draw in
                                                // CommandBuffer, should use RenderPass Ops
                                                // instead
    DRAWSTATE_BEGIN_CB_INVALID_STATE,           // CB state at Begin call is bad. Can be
                                                // Primary/Secondary CB created with
                                                // mismatched FB/RP information or CB in
                                                // RECORDING state
    DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE,      // CmdBuffer is being used in
                                                // violation of
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    // rules (i.e. simultaneous use w/o
    // that bit set)
    DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, // Attempting to call Reset (or
                                            // Begin on recorded cmdBuffer) that
                                            // was allocated from Pool w/o
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    // bit set
    DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH,             // Count for viewports and scissors
                                                     // mismatch and/or state doesn't match
                                                     // count
    DRAWSTATE_INVALID_IMAGE_ASPECT,                  // Image aspect is invalid for the current
                                                     // operation
    DRAWSTATE_MISSING_ATTACHMENT_REFERENCE,          // Attachment reference must be
                                                     // present in active subpass
    DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR,              // A Descriptor of *_SAMPLER type is
                                                     // being updated with an invalid or bad
                                                     // Sampler
    DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, // Descriptors of
                                                     // *COMBINED_IMAGE_SAMPLER
                                                     // type are being updated
                                                     // where some, but not all,
                                                     // of the updates use
                                                     // immutable samplers
    DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR,            // A Descriptor of *_IMAGE or
                                                     // *_ATTACHMENT type is being updated
                                                     // with an invalid or bad ImageView
    DRAWSTATE_BUFFERVIEW_DESCRIPTOR_ERROR,           // A Descriptor of *_TEXEL_BUFFER
                                                     // type is being updated with an
                                                     // invalid or bad BufferView
    DRAWSTATE_BUFFERINFO_DESCRIPTOR_ERROR,           // A Descriptor of
    // *_[UNIFORM|STORAGE]_BUFFER_[DYNAMIC]
    // type is being updated with an
    // invalid or bad BufferView
    DRAWSTATE_DYNAMIC_OFFSET_OVERFLOW,       // At draw time the dynamic offset
                                             // combined with buffer offset and range
                                             // oversteps size of buffer
    DRAWSTATE_DOUBLE_DESTROY,                // Destroying an object twice
    DRAWSTATE_OBJECT_INUSE,                  // Destroying or modifying an object in use by a
                                             // command buffer
    DRAWSTATE_QUEUE_FORWARD_PROGRESS,        // Queue cannot guarantee forward progress
    DRAWSTATE_INVALID_BUFFER_MEMORY_OFFSET,  // Dynamic Buffer Offset
                                             // violates memory requirements limit
    DRAWSTATE_INVALID_TEXEL_BUFFER_OFFSET,   // Dynamic Texel Buffer Offsets
                                             // violate device limit
    DRAWSTATE_INVALID_UNIFORM_BUFFER_OFFSET, // Dynamic Uniform Buffer Offsets
                                             // violate device limit
    DRAWSTATE_INVALID_STORAGE_BUFFER_OFFSET, // Dynamic Storage Buffer Offsets
                                             // violate device limit
    DRAWSTATE_INDEPENDENT_BLEND,             // If independent blending is not enabled, all
                                             // elements of pAttachmentsMustBeIdentical
    DRAWSTATE_DISABLED_LOGIC_OP,             // If logic operations is not enabled, logicOpEnable
                                             // must be VK_FALSE
    DRAWSTATE_INVALID_QUEUE_INDEX,           // Specified queue index exceeds number
                                             // of queried queue families
    DRAWSTATE_INVALID_QUEUE_FAMILY,          // Command buffer submitted on queue is from
                                             // a different queue family
    DRAWSTATE_IMAGE_TRANSFER_GRANULARITY,    // Violation of queue family's image transfer
                                             // granularity
    DRAWSTATE_PUSH_CONSTANTS_ERROR,          // Push constants exceed maxPushConstantSize
    DRAWSTATE_INVALID_SUBPASS_INDEX,         // Stepping beyond last subpass, or not reaching it
    DRAWSTATE_SWAPCHAIN_NO_SYNC_FOR_ACQUIRE, // AcquireNextImageKHR with no sync object
    DRAWSTATE_SWAPCHAIN_INVALID_IMAGE,       // QueuePresentKHR with image index out of range
    DRAWSTATE_SWAPCHAIN_IMAGE_NOT_ACQUIRED,  // QueuePresentKHR with image not acquired by app
    DRAWSTATE_SWAPCHAIN_ALREADY_EXISTS,      // Surface has an existing swapchain that is not being replaced
    DRAWSTATE_SWAPCHAIN_WRONG_SURFACE,       // Swapchain being replaced is not attached to the same surface
};

// Shader Checker ERROR codes
enum SHADER_CHECKER_ERROR {
    SHADER_CHECKER_NONE,
    SHADER_CHECKER_INTERFACE_TYPE_MISMATCH,    // Type mismatch between shader stages or shader and pipeline
    SHADER_CHECKER_OUTPUT_NOT_CONSUMED,        // Entry appears in output interface, but missing in input
    SHADER_CHECKER_INPUT_NOT_PRODUCED,         // Entry appears in input interface, but missing in output
    SHADER_CHECKER_NON_SPIRV_SHADER,           // Shader image is not SPIR-V
    SHADER_CHECKER_INCONSISTENT_SPIRV,         // General inconsistency within a SPIR-V module
    SHADER_CHECKER_UNKNOWN_STAGE,              // Stage is not supported by analysis
    SHADER_CHECKER_INCONSISTENT_VI,            // VI state contains conflicting binding or attrib descriptions
    SHADER_CHECKER_MISSING_DESCRIPTOR,         // Shader attempts to use a descriptor binding not declared in the layout
    SHADER_CHECKER_BAD_SPECIALIZATION,         // Specialization map entry points outside specialization data block
    SHADER_CHECKER_MISSING_ENTRYPOINT,         // Shader module does not contain the requested entrypoint
    SHADER_CHECKER_PUSH_CONSTANT_OUT_OF_RANGE, // Push constant variable is not in a push constant range
    SHADER_CHECKER_PUSH_CONSTANT_NOT_ACCESSIBLE_FROM_STAGE, // Push constant range exists, but not accessible from stage
    SHADER_CHECKER_DESCRIPTOR_TYPE_MISMATCH,                // Descriptor type does not match shader resource type
    SHADER_CHECKER_DESCRIPTOR_NOT_ACCESSIBLE_FROM_STAGE,    // Descriptor used by shader, but not accessible from stage
    SHADER_CHECKER_FEATURE_NOT_ENABLED,                     // Shader uses capability requiring a feature not enabled on device
    SHADER_CHECKER_BAD_CAPABILITY,                          // Shader uses capability not supported by Vulkan (OpenCL features)
    SHADER_CHECKER_MISSING_INPUT_ATTACHMENT,   // Shader uses an input attachment but not declared in subpass
    SHADER_CHECKER_INPUT_ATTACHMENT_TYPE_MISMATCH,          // Shader input attachment type does not match subpass format
};

// Device Limits ERROR codes
enum DEV_LIMITS_ERROR {
    DEVLIMITS_NONE,                          // Used for INFO & other non-error messages
    DEVLIMITS_INVALID_INSTANCE,              // Invalid instance used
    DEVLIMITS_INVALID_PHYSICAL_DEVICE,       // Invalid physical device used
    DEVLIMITS_MISSING_QUERY_COUNT,           // Did not make initial call to an API to query the count
    DEVLIMITS_MUST_QUERY_COUNT,              // Failed to make initial call to an API to query the count
    DEVLIMITS_INVALID_FEATURE_REQUESTED,     // App requested a feature not supported by physical device
    DEVLIMITS_COUNT_MISMATCH,                // App requesting a count value different than actual value
    DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,  // Invalid queue requested based on queue family properties
};
#endif // CORE_VALIDATION_ERROR_ENUMS_H_
