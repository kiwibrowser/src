/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015-2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Tony Barbour <tony@LunarG.com>
 * Author: Cody Northrop <cnorthrop@google.com>
 */

#ifdef ANDROID
#include "vulkan_wrapper.h"
#else
#include <vulkan/vulkan.h>
#endif

#if defined(ANDROID) && defined(VALIDATION_APK)
#include <android/log.h>
#include <android_native_app_glue.h>
#endif

#include "icd-spv.h"
#include "test_common.h"
#include "vk_layer_config.h"
#include "vkrenderframework.h"
#include <unordered_set>
#include "vk_validation_error_messages.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#define PARAMETER_VALIDATION_TESTS 1
#define MEM_TRACKER_TESTS 1
#define OBJ_TRACKER_TESTS 1
#define DRAW_STATE_TESTS 1
#define THREADING_TESTS 1
#define SHADER_CHECKER_TESTS 1
#define DEVICE_LIMITS_TESTS 1
#define IMAGE_TESTS 1

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
struct Vertex {
    float posX, posY, posZ, posW; // Position data
    float r, g, b, a;             // Color
};

#define XYZ1(_x_, _y_, _z_) (_x_), (_y_), (_z_), 1.f

typedef enum _BsoFailSelect {
    BsoFailNone = 0x00000000,
    BsoFailLineWidth = 0x00000001,
    BsoFailDepthBias = 0x00000002,
    BsoFailViewport = 0x00000004,
    BsoFailScissor = 0x00000008,
    BsoFailBlend = 0x00000010,
    BsoFailDepthBounds = 0x00000020,
    BsoFailStencilReadMask = 0x00000040,
    BsoFailStencilWriteMask = 0x00000080,
    BsoFailStencilReference = 0x00000100,
    BsoFailCmdClearAttachments = 0x00000200,
    BsoFailIndexBuffer = 0x00000400,
} BsoFailSelect;

struct vktriangle_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[3][4];
    float color[3][4];
};

static const char bindStateVertShaderText[] = "#version 450\n"
                                              "vec2 vertices[3];\n"
                                              "out gl_PerVertex {\n"
                                              "    vec4 gl_Position;\n"
                                              "};\n"
                                              "void main() {\n"
                                              "      vertices[0] = vec2(-1.0, -1.0);\n"
                                              "      vertices[1] = vec2( 1.0, -1.0);\n"
                                              "      vertices[2] = vec2( 0.0,  1.0);\n"
                                              "   gl_Position = vec4(vertices[gl_VertexIndex % 3], 0.0, 1.0);\n"
                                              "}\n";

static const char bindStateFragShaderText[] = "#version 450\n"
                                              "\n"
                                              "layout(location = 0) out vec4 uFragColor;\n"
                                              "void main(){\n"
                                              "   uFragColor = vec4(0,1,0,1);\n"
                                              "}\n";

static VKAPI_ATTR VkBool32 VKAPI_CALL myDbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
                                                void *pUserData);

// ErrorMonitor Usage:
//
// Call SetDesiredFailureMsg with: a string to be compared against all
// encountered log messages, or a validation error enum identifying
// desired error message. Passing NULL or VALIDATION_ERROR_MAX_ENUM
// will match all log messages. logMsg will return true for skipCall
// only if msg is matched or NULL.
//
// Call DesiredMsgFound to determine if the desired failure message
// was encountered.
class ErrorMonitor {
  public:
    ErrorMonitor() {
        test_platform_thread_create_mutex(&m_mutex);
        test_platform_thread_lock_mutex(&m_mutex);
        m_msgFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
        m_bailout = NULL;
        test_platform_thread_unlock_mutex(&m_mutex);
    }

    ~ErrorMonitor() { test_platform_thread_delete_mutex(&m_mutex); }

    // ErrorMonitor will look for an error message containing the specified string
    void SetDesiredFailureMsg(VkFlags msgFlags, const char *msgString) {
        // Also discard all collected messages to this point
        test_platform_thread_lock_mutex(&m_mutex);
        m_failure_message_strings.clear();
        // If we are looking for a matching string, ignore any IDs
        m_desired_message_ids.clear();
        m_otherMsgs.clear();
        m_desired_message_strings.insert(msgString);
        m_msgFound = VK_FALSE;
        m_msgFlags = msgFlags;
        test_platform_thread_unlock_mutex(&m_mutex);
    }

    // ErrorMonitor will look for a message ID matching the specified one
    void SetDesiredFailureMsg(VkFlags msgFlags, UNIQUE_VALIDATION_ERROR_CODE msg_id) {
        // Also discard all collected messages to this point
        test_platform_thread_lock_mutex(&m_mutex);
        m_failure_message_strings.clear();
        // If we are looking for IDs don't look for strings
        m_desired_message_strings.clear();
        m_otherMsgs.clear();
        m_desired_message_ids.insert(msg_id);
        m_msgFound = VK_FALSE;
        m_msgFlags = msgFlags;
        test_platform_thread_unlock_mutex(&m_mutex);
    }

    VkBool32 CheckForDesiredMsg(uint32_t message_code, const char *msgString) {
        VkBool32 result = VK_FALSE;
        test_platform_thread_lock_mutex(&m_mutex);
        if (m_bailout != NULL) {
            *m_bailout = true;
        }
        string errorString(msgString);
        bool found_expected = false;

        for (auto desired_msg : m_desired_message_strings) {
            if (desired_msg.length() == 0) {
                // An empty desired_msg string "" indicates a positive test - not expecting an error.
                // Return true to avoid calling layers/driver with this error.
                // And don't erase the "" string, so it remains if another error is found.
                result = VK_TRUE;
            } else if (errorString.find(desired_msg) != string::npos) {
                found_expected = true;
                m_failure_message_strings.insert(errorString);
                m_msgFound = VK_TRUE;
                result = VK_TRUE;
                // We only want one match for each expected error so remove from set here
                // Since we're about the break the loop it's ok to remove from set we're iterating over
                m_desired_message_strings.erase(desired_msg);
                break;
            }
        }
        for (auto desired_id : m_desired_message_ids) {
            if (desired_id == VALIDATION_ERROR_MAX_ENUM) {
                // A message ID set to MAX_ENUM indicates a positive test - not expecting an error.
                // Return true to avoid calling layers/driver with this error.
                result = VK_TRUE;
            } else if (desired_id == message_code) {
                // Double-check that the string matches the error enum
                if (errorString.find(validation_error_map[desired_id]) != string::npos) {
                    found_expected = true;
                    result = VK_TRUE;
                    m_msgFound = VK_TRUE;
                    m_desired_message_ids.erase(desired_id);
                    break;
                } else {
                    // Treat this message as a regular unexpected error, but print a warning jic
                    printf("Message (%s) from MessageID %d does not correspond to expected message from error Database (%s)\n",
                           errorString.c_str(), desired_id, validation_error_map[desired_id]);
                }
            }
        }

        if (!found_expected) {
            printf("Unexpected: %s\n", msgString);
            m_otherMsgs.push_back(errorString);
        }
        test_platform_thread_unlock_mutex(&m_mutex);
        return result;
    }

    vector<string> GetOtherFailureMsgs(void) { return m_otherMsgs; }

    VkDebugReportFlagsEXT GetMessageFlags(void) { return m_msgFlags; }

    VkBool32 DesiredMsgFound(void) { return m_msgFound; }

    void SetBailout(bool *bailout) { m_bailout = bailout; }

    void DumpFailureMsgs(void) {
        vector<string> otherMsgs = GetOtherFailureMsgs();
        cout << "Other error messages logged for this test were:" << endl;
        for (auto iter = otherMsgs.begin(); iter != otherMsgs.end(); iter++) {
            cout << "     " << *iter << endl;
        }
    }

    // Helpers

    // ExpectSuccess now takes an optional argument allowing a custom combination of debug flags
    void ExpectSuccess(VkDebugReportFlagsEXT message_flag_mask = VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        m_msgFlags = message_flag_mask;
        // Match ANY message matching specified type
        SetDesiredFailureMsg(message_flag_mask, "");
    }

    void VerifyFound() {
        // Not seeing the desired message is a failure. /Before/ throwing, dump any other messages.
        if (!DesiredMsgFound()) {
            DumpFailureMsgs();
            for (auto desired_msg : m_desired_message_strings) {
                FAIL() << "Did not receive expected error '" << desired_msg << "'";
            }
        }
    }

    void VerifyNotFound() {
        // ExpectSuccess() configured us to match anything. Any error is a failure.
        if (DesiredMsgFound()) {
            DumpFailureMsgs();
            for (auto msg : m_failure_message_strings) {
                FAIL() << "Expected to succeed but got error: " << msg;
            }
        }
    }

  private:
    VkFlags m_msgFlags;
    std::unordered_set<uint32_t>m_desired_message_ids;
    std::unordered_set<string> m_desired_message_strings;
    std::unordered_set<string> m_failure_message_strings;
    vector<string> m_otherMsgs;
    test_platform_thread_mutex m_mutex;
    bool *m_bailout;
    VkBool32 m_msgFound;
};

static VKAPI_ATTR VkBool32 VKAPI_CALL myDbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
                                                void *pUserData) {
    ErrorMonitor *errMonitor = (ErrorMonitor *)pUserData;
    if (msgFlags & errMonitor->GetMessageFlags()) {
        return errMonitor->CheckForDesiredMsg(msgCode, pMsg);
    }
    return false;
}

class VkLayerTest : public VkRenderFramework {
  public:
    VkResult BeginCommandBuffer(VkCommandBufferObj &commandBuffer);
    VkResult EndCommandBuffer(VkCommandBufferObj &commandBuffer);
    void VKTriangleTest(const char *vertShaderText, const char *fragShaderText, BsoFailSelect failMask);
    void GenericDrawPreparation(VkCommandBufferObj *commandBuffer, VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet,
                                BsoFailSelect failMask);
    void GenericDrawPreparation(VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet, BsoFailSelect failMask) {
        GenericDrawPreparation(m_commandBuffer, pipelineobj, descriptorSet, failMask);
    }

    /* Convenience functions that use built-in command buffer */
    VkResult BeginCommandBuffer() { return BeginCommandBuffer(*m_commandBuffer); }
    VkResult EndCommandBuffer() { return EndCommandBuffer(*m_commandBuffer); }
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        m_commandBuffer->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                     uint32_t firstInstance) {
        m_commandBuffer->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
    void QueueCommandBuffer(bool checkSuccess = true) { m_commandBuffer->QueueCommandBuffer(checkSuccess); }
    void QueueCommandBuffer(const VkFence &fence) { m_commandBuffer->QueueCommandBuffer(fence); }
    void BindVertexBuffer(VkConstantBufferObj *vertexBuffer, VkDeviceSize offset, uint32_t binding) {
        m_commandBuffer->BindVertexBuffer(vertexBuffer, offset, binding);
    }
    void BindIndexBuffer(VkIndexBufferObj *indexBuffer, VkDeviceSize offset) {
        m_commandBuffer->BindIndexBuffer(indexBuffer, offset);
    }

  protected:
    ErrorMonitor *m_errorMonitor;
    bool m_enableWSI;

    virtual void SetUp() {
        std::vector<const char *> instance_layer_names;
        std::vector<const char *> instance_extension_names;
        std::vector<const char *> device_extension_names;

        instance_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        /*
         * Since CreateDbgMsgCallback is an instance level extension call
         * any extension / layer that utilizes that feature also needs
         * to be enabled at create instance time.
         */
        // Use Threading layer first to protect others from
        // ThreadCommandBufferCollision test
        instance_layer_names.push_back("VK_LAYER_GOOGLE_threading");
        instance_layer_names.push_back("VK_LAYER_LUNARG_parameter_validation");
        instance_layer_names.push_back("VK_LAYER_LUNARG_object_tracker");
        instance_layer_names.push_back("VK_LAYER_LUNARG_core_validation");
        instance_layer_names.push_back("VK_LAYER_LUNARG_image");
        instance_layer_names.push_back("VK_LAYER_LUNARG_swapchain");
        instance_layer_names.push_back("VK_LAYER_GOOGLE_unique_objects");

        if (m_enableWSI) {
            instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
            device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef NEED_TO_TEST_THIS_ON_PLATFORM
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
            instance_extension_names.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif // VK_USE_PLATFORM_ANDROID_KHR
#if defined(VK_USE_PLATFORM_MIR_KHR)
            instance_extension_names.push_back(VK_KHR_MIR_SURFACE_EXTENSION_NAME);
#endif // VK_USE_PLATFORM_MIR_KHR
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
            instance_extension_names.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif // VK_USE_PLATFORM_WIN32_KHR
#endif // NEED_TO_TEST_THIS_ON_PLATFORM
#if defined(VK_USE_PLATFORM_XCB_KHR)
            instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
            instance_extension_names.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif // VK_USE_PLATFORM_XLIB_KHR
        }

        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pApplicationName = "layer_tests";
        this->app_info.applicationVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION_1_0;

        m_errorMonitor = new ErrorMonitor;
        InitFramework(instance_layer_names, instance_extension_names, device_extension_names, myDbgFunc, m_errorMonitor);
    }

    virtual void TearDown() {
        // Clean up resources before we reset
        ShutdownFramework();
        delete m_errorMonitor;
    }

    VkLayerTest() { m_enableWSI = false; }
};

VkResult VkLayerTest::BeginCommandBuffer(VkCommandBufferObj &commandBuffer) {
    VkResult result;

    result = commandBuffer.BeginCommandBuffer();

    /*
     * For render test all drawing happens in a single render pass
     * on a single command buffer.
     */
    if (VK_SUCCESS == result && renderPass()) {
        commandBuffer.BeginRenderPass(renderPassBeginInfo());
    }

    return result;
}

VkResult VkLayerTest::EndCommandBuffer(VkCommandBufferObj &commandBuffer) {
    VkResult result;

    if (renderPass()) {
        commandBuffer.EndRenderPass();
    }

    result = commandBuffer.EndCommandBuffer();

    return result;
}

void VkLayerTest::VKTriangleTest(const char *vertShaderText, const char *fragShaderText, BsoFailSelect failMask) {
    // Create identity matrix
    int i;
    struct vktriangle_vs_uniform data;

    glm::mat4 Projection = glm::mat4(1.0f);
    glm::mat4 View = glm::mat4(1.0f);
    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 MVP = Projection * View * Model;
    const int matrixSize = sizeof(MVP);
    const int bufSize = sizeof(vktriangle_vs_uniform) / sizeof(float);

    memcpy(&data.mvp, &MVP[0][0], matrixSize);

    static const Vertex tri_data[] = {
        {XYZ1(-1, -1, 0), XYZ1(1.f, 0.f, 0.f)}, {XYZ1(1, -1, 0), XYZ1(0.f, 1.f, 0.f)}, {XYZ1(0, 1, 0), XYZ1(0.f, 0.f, 1.f)},
    };

    for (i = 0; i < 3; i++) {
        data.position[i][0] = tri_data[i].posX;
        data.position[i][1] = tri_data[i].posY;
        data.position[i][2] = tri_data[i].posZ;
        data.position[i][3] = tri_data[i].posW;
        data.color[i][0] = tri_data[i].r;
        data.color[i][1] = tri_data[i].g;
        data.color[i][2] = tri_data[i].b;
        data.color[i][3] = tri_data[i].a;
    }

    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj constantBuffer(m_device, bufSize * 2, sizeof(float), (const void *)&data,
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    VkShaderObj vs(m_device, vertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj ps(m_device, fragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);
    if (failMask & BsoFailLineWidth) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_LINE_WIDTH);
        VkPipelineInputAssemblyStateCreateInfo ia_state = {};
        ia_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        pipelineobj.SetInputAssembly(&ia_state);
    }
    if (failMask & BsoFailDepthBias) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_DEPTH_BIAS);
        VkPipelineRasterizationStateCreateInfo rs_state = {};
        rs_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs_state.depthBiasEnable = VK_TRUE;
        rs_state.lineWidth = 1.0f;
        pipelineobj.SetRasterization(&rs_state);
    }
    // Viewport and scissors must stay in synch or other errors will occur than
    // the ones we want
    if (failMask & BsoFailViewport) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_VIEWPORT);
    }
    if (failMask & BsoFailScissor) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_SCISSOR);
    }
    if (failMask & BsoFailBlend) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
        VkPipelineColorBlendAttachmentState att_state = {};
        att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
        att_state.blendEnable = VK_TRUE;
        pipelineobj.AddColorAttachment(0, &att_state);
    }
    if (failMask & BsoFailDepthBounds) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
    }
    if (failMask & BsoFailStencilReadMask) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
    }
    if (failMask & BsoFailStencilWriteMask) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
    }
    if (failMask & BsoFailStencilReference) {
        pipelineobj.MakeDynamic(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    }

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, constantBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet, failMask);

    // render triangle
    if (failMask & BsoFailIndexBuffer) {
        // Use DrawIndexed w/o an index buffer bound
        DrawIndexed(3, 1, 0, 0, 0);
    } else {
        Draw(3, 1, 0, 0);
    }

    if (failMask & BsoFailCmdClearAttachments) {
        VkClearAttachment color_attachment = {};
        color_attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_attachment.colorAttachment = 1; // Someone who knew what they were doing would use 0 for the index;
        VkClearRect clear_rect = {{{0, 0}, {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)}}, 0, 0};

        vkCmdClearAttachments(m_commandBuffer->GetBufferHandle(), 1, &color_attachment, 1, &clear_rect);
    }

    // finalize recording of the command buffer
    EndCommandBuffer();

    QueueCommandBuffer();
}

void VkLayerTest::GenericDrawPreparation(VkCommandBufferObj *commandBuffer, VkPipelineObj &pipelineobj,
                                         VkDescriptorSetObj &descriptorSet, BsoFailSelect failMask) {
    if (m_depthStencil->Initialized()) {
        commandBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, m_depthStencil);
    } else {
        commandBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    }

    commandBuffer->PrepareAttachments();
    // Make sure depthWriteEnable is set so that Depth fail test will work
    // correctly
    // Make sure stencilTestEnable is set so that Stencil fail test will work
    // correctly
    VkStencilOpState stencil = {};
    stencil.failOp = VK_STENCIL_OP_KEEP;
    stencil.passOp = VK_STENCIL_OP_KEEP;
    stencil.depthFailOp = VK_STENCIL_OP_KEEP;
    stencil.compareOp = VK_COMPARE_OP_NEVER;

    VkPipelineDepthStencilStateCreateInfo ds_ci = {};
    ds_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds_ci.pNext = NULL;
    ds_ci.depthTestEnable = VK_FALSE;
    ds_ci.depthWriteEnable = VK_TRUE;
    ds_ci.depthCompareOp = VK_COMPARE_OP_NEVER;
    ds_ci.depthBoundsTestEnable = VK_FALSE;
    if (failMask & BsoFailDepthBounds) {
        ds_ci.depthBoundsTestEnable = VK_TRUE;
        ds_ci.maxDepthBounds = 0.0f;
        ds_ci.minDepthBounds = 0.0f;
    }
    ds_ci.stencilTestEnable = VK_TRUE;
    ds_ci.front = stencil;
    ds_ci.back = stencil;

    pipelineobj.SetDepthStencil(&ds_ci);
    pipelineobj.SetViewport(m_viewports);
    pipelineobj.SetScissor(m_scissors);
    descriptorSet.CreateVKDescriptorSet(commandBuffer);
    VkResult err = pipelineobj.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());
    ASSERT_VK_SUCCESS(err);
    commandBuffer->BindPipeline(pipelineobj);
    commandBuffer->BindDescriptorSet(descriptorSet);
}

class VkPositiveLayerTest : public VkLayerTest {
  public:
  protected:
};

class VkWsiEnabledLayerTest : public VkLayerTest {
  public:
  protected:
    VkWsiEnabledLayerTest() { m_enableWSI = true; }
};

class VkBufferTest {
  public:
    enum eTestEnFlags {
        eDoubleDelete,
        eInvalidDeviceOffset,
        eInvalidMemoryOffset,
        eBindNullBuffer,
        eFreeInvalidHandle,
        eNone,
    };

    enum eTestConditions { eOffsetAlignment = 1 };

    static bool GetTestConditionValid(VkDeviceObj *aVulkanDevice, eTestEnFlags aTestFlag, VkBufferUsageFlags aBufferUsage = 0) {
        if (eInvalidDeviceOffset != aTestFlag && eInvalidMemoryOffset != aTestFlag) {
            return true;
        }
        VkDeviceSize offset_limit = 0;
        if (eInvalidMemoryOffset == aTestFlag) {
            VkBuffer vulkanBuffer;
            VkBufferCreateInfo buffer_create_info = {};
            buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_create_info.size = 32;
            buffer_create_info.usage = aBufferUsage;

            vkCreateBuffer(aVulkanDevice->device(), &buffer_create_info, nullptr, &vulkanBuffer);
            VkMemoryRequirements memory_reqs = {};

            vkGetBufferMemoryRequirements(aVulkanDevice->device(), vulkanBuffer, &memory_reqs);
            vkDestroyBuffer(aVulkanDevice->device(), vulkanBuffer, nullptr);
            offset_limit = memory_reqs.alignment;
        } else if ((VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) & aBufferUsage) {
            offset_limit = aVulkanDevice->props.limits.minTexelBufferOffsetAlignment;
        } else if (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT & aBufferUsage) {
            offset_limit = aVulkanDevice->props.limits.minUniformBufferOffsetAlignment;
        } else if (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT & aBufferUsage) {
            offset_limit = aVulkanDevice->props.limits.minStorageBufferOffsetAlignment;
        }
        if (eOffsetAlignment < offset_limit) {
            return true;
        }
        return false;
    }

    // A constructor which performs validation tests within construction.
    VkBufferTest(VkDeviceObj *aVulkanDevice, VkBufferUsageFlags aBufferUsage, eTestEnFlags aTestFlag = eNone)
        : AllocateCurrent(false), BoundCurrent(false), CreateCurrent(false), VulkanDevice(aVulkanDevice->device()) {

        if (eBindNullBuffer == aTestFlag) {
            VulkanMemory = 0;
            vkBindBufferMemory(VulkanDevice, VulkanBuffer, VulkanMemory, 0);
        } else {
            VkBufferCreateInfo buffer_create_info = {};
            buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_create_info.size = 32;
            buffer_create_info.usage = aBufferUsage;

            vkCreateBuffer(VulkanDevice, &buffer_create_info, nullptr, &VulkanBuffer);

            CreateCurrent = true;

            VkMemoryRequirements memory_requirements;
            vkGetBufferMemoryRequirements(VulkanDevice, VulkanBuffer, &memory_requirements);

            VkMemoryAllocateInfo memory_allocate_info = {};
            memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memory_allocate_info.allocationSize = memory_requirements.size;
            bool pass = aVulkanDevice->phy().set_memory_type(memory_requirements.memoryTypeBits, &memory_allocate_info,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            if (!pass) {
                vkDestroyBuffer(VulkanDevice, VulkanBuffer, nullptr);
                return;
            }

            vkAllocateMemory(VulkanDevice, &memory_allocate_info, NULL, &VulkanMemory);
            AllocateCurrent = true;
            // NB: 1 is intentionally an invalid offset value
            const bool offset_en = eInvalidDeviceOffset == aTestFlag || eInvalidMemoryOffset == aTestFlag;
            vkBindBufferMemory(VulkanDevice, VulkanBuffer, VulkanMemory, offset_en ? eOffsetAlignment : 0);
            BoundCurrent = true;

            InvalidDeleteEn = (eFreeInvalidHandle == aTestFlag);
        }
    }

    ~VkBufferTest() {
        if (CreateCurrent) {
            vkDestroyBuffer(VulkanDevice, VulkanBuffer, nullptr);
        }
        if (AllocateCurrent) {
            if (InvalidDeleteEn) {
                union {
                    VkDeviceMemory device_memory;
                    unsigned long long index_access;
                } bad_index;

                bad_index.device_memory = VulkanMemory;
                bad_index.index_access++;

                vkFreeMemory(VulkanDevice, bad_index.device_memory, nullptr);
            }
            vkFreeMemory(VulkanDevice, VulkanMemory, nullptr);
        }
    }

    bool GetBufferCurrent() { return AllocateCurrent && BoundCurrent && CreateCurrent; }

    const VkBuffer &GetBuffer() { return VulkanBuffer; }

    void TestDoubleDestroy() {
        // Destroy the buffer but leave the flag set, which will cause
        // the buffer to be destroyed again in the destructor.
        vkDestroyBuffer(VulkanDevice, VulkanBuffer, nullptr);
    }

  protected:
    bool AllocateCurrent;
    bool BoundCurrent;
    bool CreateCurrent;
    bool InvalidDeleteEn;

    VkBuffer VulkanBuffer;
    VkDevice VulkanDevice;
    VkDeviceMemory VulkanMemory;
};

class VkVerticesObj {
  public:
    VkVerticesObj(VkDeviceObj *aVulkanDevice, unsigned aAttributeCount, unsigned aBindingCount, unsigned aByteStride,
                  VkDeviceSize aVertexCount, const float *aVerticies)
        : BoundCurrent(false), AttributeCount(aAttributeCount), BindingCount(aBindingCount), BindId(BindIdGenerator),
          PipelineVertexInputStateCreateInfo(),
          VulkanMemoryBuffer(aVulkanDevice, 1, static_cast<int>(aByteStride * aVertexCount),
                             reinterpret_cast<const void *>(aVerticies), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        BindIdGenerator++; // NB: This can wrap w/misuse

        VertexInputAttributeDescription = new VkVertexInputAttributeDescription[AttributeCount];
        VertexInputBindingDescription = new VkVertexInputBindingDescription[BindingCount];

        PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttributeDescription;
        PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = AttributeCount;
        PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = VertexInputBindingDescription;
        PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = BindingCount;
        PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        unsigned i = 0;
        do {
            VertexInputAttributeDescription[i].binding = BindId;
            VertexInputAttributeDescription[i].location = i;
            VertexInputAttributeDescription[i].format = VK_FORMAT_R32G32B32_SFLOAT;
            VertexInputAttributeDescription[i].offset = sizeof(float) * aByteStride;
            i++;
        } while (AttributeCount < i);

        i = 0;
        do {
            VertexInputBindingDescription[i].binding = BindId;
            VertexInputBindingDescription[i].stride = aByteStride;
            VertexInputBindingDescription[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            i++;
        } while (BindingCount < i);
    }

    ~VkVerticesObj() {
        if (VertexInputAttributeDescription) {
            delete[] VertexInputAttributeDescription;
        }
        if (VertexInputBindingDescription) {
            delete[] VertexInputBindingDescription;
        }
    }

    bool AddVertexInputToPipe(VkPipelineObj &aPipelineObj) {
        aPipelineObj.AddVertexInputAttribs(VertexInputAttributeDescription, AttributeCount);
        aPipelineObj.AddVertexInputBindings(VertexInputBindingDescription, BindingCount);
        return true;
    }

    void BindVertexBuffers(VkCommandBuffer aCommandBuffer, unsigned aOffsetCount = 0, VkDeviceSize *aOffsetList = nullptr) {
        VkDeviceSize *offsetList;
        unsigned offsetCount;

        if (aOffsetCount) {
            offsetList = aOffsetList;
            offsetCount = aOffsetCount;
        } else {
            offsetList = new VkDeviceSize[1]();
            offsetCount = 1;
        }

        vkCmdBindVertexBuffers(aCommandBuffer, BindId, offsetCount, &VulkanMemoryBuffer.handle(), offsetList);
        BoundCurrent = true;

        if (!aOffsetCount) {
            delete[] offsetList;
        }
    }

  protected:
    static uint32_t BindIdGenerator;

    bool BoundCurrent;
    unsigned AttributeCount;
    unsigned BindingCount;
    uint32_t BindId;

    VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;
    VkVertexInputAttributeDescription *VertexInputAttributeDescription;
    VkVertexInputBindingDescription *VertexInputBindingDescription;
    VkConstantBufferObj VulkanMemoryBuffer;
};

uint32_t VkVerticesObj::BindIdGenerator;
// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************
#if PARAMETER_VALIDATION_TESTS
TEST_F(VkLayerTest, RequiredParameter) {
    TEST_DESCRIPTION("Specify VK_NULL_HANDLE, NULL, and 0 for required handle, "
                     "pointer, array, and array count parameters");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pFeatures specified as NULL");
    // Specify NULL for a pointer to a handle
    // Expected to trigger an error with
    // parameter_validation::validate_required_pointer
    vkGetPhysicalDeviceFeatures(gpu(), NULL);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "required parameter pQueueFamilyPropertyCount specified as NULL");
    // Specify NULL for pointer to array count
    // Expected to trigger an error with parameter_validation::validate_array
    vkGetPhysicalDeviceQueueFamilyProperties(gpu(), NULL, NULL);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "parameter viewportCount must be greater than 0");
    // Specify 0 for a required array count
    // Expected to trigger an error with parameter_validation::validate_array
    VkViewport view_port = {};
    m_commandBuffer->SetViewport(0, 0, &view_port);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pViewports specified as NULL");
    // Specify NULL for a required array
    // Expected to trigger an error with parameter_validation::validate_array
    m_commandBuffer->SetViewport(0, 1, NULL);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter memory specified as VK_NULL_HANDLE");
    // Specify VK_NULL_HANDLE for a required handle
    // Expected to trigger an error with
    // parameter_validation::validate_required_handle
    vkUnmapMemory(device(), VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "required parameter pFences[0] specified as VK_NULL_HANDLE");
    // Specify VK_NULL_HANDLE for a required handle array entry
    // Expected to trigger an error with
    // parameter_validation::validate_required_handle_array
    VkFence fence = VK_NULL_HANDLE;
    vkResetFences(device(), 1, &fence);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pAllocateInfo specified as NULL");
    // Specify NULL for a required struct pointer
    // Expected to trigger an error with
    // parameter_validation::validate_struct_type
    VkDeviceMemory memory = VK_NULL_HANDLE;
    vkAllocateMemory(device(), NULL, NULL, &memory);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "value of faceMask must not be 0");
    // Specify 0 for a required VkFlags parameter
    // Expected to trigger an error with parameter_validation::validate_flags
    m_commandBuffer->SetStencilReference(0, 0);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "value of pSubmits[0].pWaitDstStageMask[0] must not be 0");
    // Specify 0 for a required VkFlags array entry
    // Expected to trigger an error with
    // parameter_validation::validate_flags_array
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkPipelineStageFlags stageFlags = 0;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphore;
    submitInfo.pWaitDstStageMask = &stageFlags;
    vkQueueSubmit(m_device->m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ReservedParameter) {
    TEST_DESCRIPTION("Specify a non-zero value for a reserved parameter");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " must be 0");
    // Specify 0 for a reserved VkFlags parameter
    // Expected to trigger an error with
    // parameter_validation::validate_reserved_flags
    VkEvent event_handle = VK_NULL_HANDLE;
    VkEventCreateInfo event_info = {};
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    event_info.flags = 1;
    vkCreateEvent(device(), &event_info, NULL, &event_handle);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidStructSType) {
    TEST_DESCRIPTION("Specify an invalid VkStructureType for a Vulkan "
                     "structure's sType field");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "parameter pAllocateInfo->sType must be");
    // Zero struct memory, effectively setting sType to
    // VK_STRUCTURE_TYPE_APPLICATION_INFO
    // Expected to trigger an error with
    // parameter_validation::validate_struct_type
    VkMemoryAllocateInfo alloc_info = {};
    VkDeviceMemory memory = VK_NULL_HANDLE;
    vkAllocateMemory(device(), &alloc_info, NULL, &memory);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "parameter pSubmits[0].sType must be");
    // Zero struct memory, effectively setting sType to
    // VK_STRUCTURE_TYPE_APPLICATION_INFO
    // Expected to trigger an error with
    // parameter_validation::validate_struct_type_array
    VkSubmitInfo submit_info = {};
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidStructPNext) {
    TEST_DESCRIPTION("Specify an invalid value for a Vulkan structure's pNext field");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, "value of pCreateInfo->pNext must be NULL");
    // Set VkMemoryAllocateInfo::pNext to a non-NULL value, when pNext must be NULL.
    // Need to pick a function that has no allowed pNext structure types.
    // Expected to trigger an error with parameter_validation::validate_struct_pnext
    VkEvent event = VK_NULL_HANDLE;
    VkEventCreateInfo event_alloc_info = {};
    // Zero-initialization will provide the correct sType
    VkApplicationInfo app_info = {};
    event_alloc_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    event_alloc_info.pNext = &app_info;
    vkCreateEvent(device(), &event_alloc_info, NULL, &event);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                         " chain includes a structure with unexpected VkStructureType ");
    // Set VkMemoryAllocateInfo::pNext to a non-NULL value, but use
    // a function that has allowed pNext structure types and specify
    // a structure type that is not allowed.
    // Expected to trigger an error with parameter_validation::validate_struct_pnext
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryAllocateInfo memory_alloc_info = {};
    memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc_info.pNext = &app_info;
    vkAllocateMemory(device(), &memory_alloc_info, NULL, &memory);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, UnrecognizedValue) {
    TEST_DESCRIPTION("Specify unrecognized Vulkan enumeration, flags, and VkBool32 values");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "does not fall within the begin..end "
                                                                        "range of the core VkFormat "
                                                                        "enumeration tokens");
    // Specify an invalid VkFormat value
    // Expected to trigger an error with
    // parameter_validation::validate_ranged_enum
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(gpu(), static_cast<VkFormat>(8000), &format_properties);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "contains flag bits that are not recognized members of");
    // Specify an invalid VkFlags bitmask value
    // Expected to trigger an error with parameter_validation::validate_flags
    VkImageFormatProperties image_format_properties;
    vkGetPhysicalDeviceImageFormatProperties(gpu(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                                             static_cast<VkImageUsageFlags>(1 << 25), 0, &image_format_properties);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "contains flag bits that are not recognized members of");
    // Specify an invalid VkFlags array entry
    // Expected to trigger an error with
    // parameter_validation::validate_flags_array
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkPipelineStageFlags stage_flags = static_cast<VkPipelineStageFlags>(1 << 25);
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &semaphore;
    submit_info.pWaitDstStageMask = &stage_flags;
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, "is neither VK_TRUE nor VK_FALSE");
    // Specify an invalid VkBool32 value
    // Expected to trigger a warning with
    // parameter_validation::validate_bool32
    VkSampler sampler = VK_NULL_HANDLE;
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = NULL;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 1.0;
    sampler_info.maxAnisotropy = 1;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_info.minLod = 1.0;
    sampler_info.maxLod = 1.0;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    // Not VK_TRUE or VK_FALSE
    sampler_info.anisotropyEnable = 3;
    vkCreateSampler(m_device->device(), &sampler_info, NULL, &sampler);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, FailedReturnValue) {
    TEST_DESCRIPTION("Check for a message describing a VkResult failure code");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Find an unsupported image format
    VkFormat unsupported = VK_FORMAT_UNDEFINED;
    for (int f = VK_FORMAT_BEGIN_RANGE; f <= VK_FORMAT_END_RANGE; f++) {
        VkFormat format = static_cast<VkFormat>(f);
        VkFormatProperties fProps = m_device->format_properties(format);
        if (format != VK_FORMAT_UNDEFINED && fProps.linearTilingFeatures == 0 && fProps.optimalTilingFeatures == 0) {
            unsupported = format;
            break;
        }
    }

    if (unsupported != VK_FORMAT_UNDEFINED) {
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                             "the requested format is not supported on this device");
        // Specify an unsupported VkFormat value to generate a
        // VK_ERROR_FORMAT_NOT_SUPPORTED return code
        // Expected to trigger a warning from
        // parameter_validation::validate_result
        VkImageFormatProperties image_format_properties;
        VkResult err = vkGetPhysicalDeviceImageFormatProperties(gpu(), unsupported, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                                                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0, &image_format_properties);
        ASSERT_TRUE(err == VK_ERROR_FORMAT_NOT_SUPPORTED);
        m_errorMonitor->VerifyFound();
    }
}

TEST_F(VkLayerTest, UpdateBufferAlignment) {
    TEST_DESCRIPTION("Check alignment parameters for vkCmdUpdateBuffer");
    uint32_t updateData[] = {1, 2, 3, 4, 5, 6, 7, 8};

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_testing::Buffer buffer;
    buffer.init_as_dst(*m_device, (VkDeviceSize)20, reqs);

    BeginCommandBuffer();
    // Introduce failure by using dstOffset that is not multiple of 4
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is not a multiple of 4");
    m_commandBuffer->UpdateBuffer(buffer.handle(), 1, 4, updateData);
    m_errorMonitor->VerifyFound();

    // Introduce failure by using dataSize that is not multiple of 4
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is not a multiple of 4");
    m_commandBuffer->UpdateBuffer(buffer.handle(), 0, 6, updateData);
    m_errorMonitor->VerifyFound();

    // Introduce failure by using dataSize that is < 0
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "must be greater than zero and less than or equal to 65536");
    m_commandBuffer->UpdateBuffer(buffer.handle(), 0, -44, updateData);
    m_errorMonitor->VerifyFound();

    // Introduce failure by using dataSize that is > 65536
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "must be greater than zero and less than or equal to 65536");
    m_commandBuffer->UpdateBuffer(buffer.handle(), 0, 80000, updateData);
    m_errorMonitor->VerifyFound();

    EndCommandBuffer();
}

TEST_F(VkLayerTest, FillBufferAlignment) {
    TEST_DESCRIPTION("Check alignment parameters for vkCmdFillBuffer");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_testing::Buffer buffer;
    buffer.init_as_dst(*m_device, (VkDeviceSize)20, reqs);

    BeginCommandBuffer();

    // Introduce failure by using dstOffset that is not multiple of 4
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is not a multiple of 4");
    m_commandBuffer->FillBuffer(buffer.handle(), 1, 4, 0x11111111);
    m_errorMonitor->VerifyFound();

    // Introduce failure by using size that is not multiple of 4
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is not a multiple of 4");
    m_commandBuffer->FillBuffer(buffer.handle(), 0, 6, 0x11111111);
    m_errorMonitor->VerifyFound();

    // Introduce failure by using size that is zero
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "must be greater than zero");
    m_commandBuffer->FillBuffer(buffer.handle(), 0, 0, 0x11111111);
    m_errorMonitor->VerifyFound();

    EndCommandBuffer();
}

TEST_F(VkLayerTest, PSOPolygonModeInvalid) {
    VkResult err;

    TEST_DESCRIPTION("Attempt to use a non-solid polygon fill mode in a "
                     "pipeline when this feature is not enabled.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    std::vector<const char *> device_extension_names;
    auto features = m_device->phy().features();
    // Artificially disable support for non-solid fill modes
    features.fillModeNonSolid = false;
    // The sacrificial device object
    VkDeviceObj test_device(0, gpu(), device_extension_names, &features);

    VkRenderpassObj render_pass(&test_device);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 0;
    pipeline_layout_ci.pSetLayouts = NULL;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(test_device.device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.pNext = nullptr;
    rs_ci.lineWidth = 1.0f;
    rs_ci.rasterizerDiscardEnable = true;

    VkShaderObj vs(&test_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(&test_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    // Set polygonMode to unsupported value POINT, should fail
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "polygonMode cannot be VK_POLYGON_MODE_POINT or VK_POLYGON_MODE_LINE");
    {
        VkPipelineObj pipe(&test_device);
        pipe.AddShader(&vs);
        pipe.AddShader(&fs);
        pipe.AddColorAttachment();
        // Introduce failure by setting unsupported polygon mode
        rs_ci.polygonMode = VK_POLYGON_MODE_POINT;
        pipe.SetRasterization(&rs_ci);
        pipe.CreateVKPipeline(pipeline_layout, render_pass.handle());
    }
    m_errorMonitor->VerifyFound();

    // Try again with polygonMode=LINE, should fail
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "polygonMode cannot be VK_POLYGON_MODE_POINT or VK_POLYGON_MODE_LINE");
    {
        VkPipelineObj pipe(&test_device);
        pipe.AddShader(&vs);
        pipe.AddShader(&fs);
        pipe.AddColorAttachment();
        // Introduce failure by setting unsupported polygon mode
        rs_ci.polygonMode = VK_POLYGON_MODE_LINE;
        pipe.SetRasterization(&rs_ci);
        pipe.CreateVKPipeline(pipeline_layout, render_pass.handle());
    }
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(test_device.device(), pipeline_layout, NULL);
}

#endif // PARAMETER_VALIDATION_TESTS

#if MEM_TRACKER_TESTS
#if 0
TEST_F(VkLayerTest, CallResetCommandBufferBeforeCompletion)
{
    vk_testing::Fence testFence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Resetting command buffer");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_testing::Buffer buffer;
    buffer.init_as_dst(*m_device, (VkDeviceSize)20, reqs);

    BeginCommandBuffer();
    m_commandBuffer->FillBuffer(buffer.handle(), 0, 4, 0x11111111);
    EndCommandBuffer();

    testFence.init(*m_device, fenceInfo);

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit( m_device->m_queue, 1, &submit_info, testFence.handle());
    ASSERT_VK_SUCCESS( err );

    // Introduce failure by calling begin again before checking fence
    vkResetCommandBuffer(m_commandBuffer->handle(), 0);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CallBeginCommandBufferBeforeCompletion)
{
    vk_testing::Fence testFence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Calling vkBeginCommandBuffer() on active command buffer");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();
    m_commandBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer();

    testFence.init(*m_device, fenceInfo);

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit( m_device->m_queue, 1, &submit_info, testFence.handle());
    ASSERT_VK_SUCCESS( err );

    VkCommandBufferInheritanceInfo hinfo = {};
    VkCommandBufferBeginInfo info = {};
    info.flags       = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.renderPass  = VK_NULL_HANDLE;
    info.subpass     = 0;
    info.framebuffer = VK_NULL_HANDLE;
    info.occlusionQueryEnable = VK_FALSE;
    info.queryFlags = 0;
    info.pipelineStatistics = 0;

    // Introduce failure by calling BCB again before checking fence
    vkBeginCommandBuffer(m_commandBuffer->handle(), &info);

    m_errorMonitor->VerifyFound();
}
#endif

TEST_F(VkLayerTest, InvalidMemoryAliasing) {
    TEST_DESCRIPTION("Create a buffer and image, allocate memory, and bind the "
                     "buffer and image to memory such that they will alias.");
    VkResult err;
    bool pass;
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkBuffer buffer, buffer2;
    VkImage image;
    VkImage image2;
    VkDeviceMemory mem;     // buffer will be bound first
    VkDeviceMemory mem_img; // image bound first
    VkMemoryRequirements buff_mem_reqs, img_mem_reqs;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = 256;
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    err = vkCreateBuffer(m_device->device(), &buf_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    vkGetBufferMemoryRequirements(m_device->device(), buffer, &buff_mem_reqs);

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_create_info.extent.width = 64;
    image_create_info.extent.height = 64;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    // Image tiling must be optimal to trigger error when aliasing linear buffer
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image2);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), image, &img_mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;
    // Ensure memory is big enough for both bindings
    alloc_info.allocationSize = buff_mem_reqs.size + img_mem_reqs.size;
    pass = m_device->phy().set_memory_type(buff_mem_reqs.memoryTypeBits & img_mem_reqs.memoryTypeBits, &alloc_info,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        vkDestroyImage(m_device->device(), image, NULL);
        return;
    }
    err = vkAllocateMemory(m_device->device(), &alloc_info, NULL, &mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is aliased with linear buffer 0x");
    // VALIDATION FAILURE due to image mapping overlapping buffer mapping
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    m_errorMonitor->VerifyFound();

    // Now correctly bind image2 to second mem allocation before incorrectly
    // aliasing buffer2
    err = vkCreateBuffer(m_device->device(), &buf_info, NULL, &buffer2);
    ASSERT_VK_SUCCESS(err);
    err = vkAllocateMemory(m_device->device(), &alloc_info, NULL, &mem_img);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image2, mem_img, 0);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "is aliased with non-linear image 0x");
    err = vkBindBufferMemory(m_device->device(), buffer2, mem_img, 0);
    m_errorMonitor->VerifyFound();

    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkDestroyBuffer(m_device->device(), buffer2, NULL);
    vkDestroyImage(m_device->device(), image, NULL);
    vkDestroyImage(m_device->device(), image2, NULL);
    vkFreeMemory(m_device->device(), mem, NULL);
    vkFreeMemory(m_device->device(), mem_img, NULL);
}

TEST_F(VkLayerTest, InvalidMemoryMapping) {
    TEST_DESCRIPTION("Attempt to map memory in a number of incorrect ways");
    VkResult err;
    bool pass;
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkBuffer buffer;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = 256;
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    err = vkCreateBuffer(m_device->device(), &buf_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    vkGetBufferMemoryRequirements(m_device->device(), buffer, &mem_reqs);
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    // Ensure memory is big enough for both bindings
    static const VkDeviceSize allocation_size = 0x10000;
    alloc_info.allocationSize = allocation_size;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }
    err = vkAllocateMemory(m_device->device(), &alloc_info, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    uint8_t *pData;
    // Attempt to map memory size 0 is invalid
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "VkMapMemory: Attempting to map memory range of size zero");
    err = vkMapMemory(m_device->device(), mem, 0, 0, 0, (void **)&pData);
    m_errorMonitor->VerifyFound();
    // Map memory twice
    err = vkMapMemory(m_device->device(), mem, 0, mem_reqs.size, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "VkMapMemory: Attempting to map memory on an already-mapped object ");
    err = vkMapMemory(m_device->device(), mem, 0, mem_reqs.size, 0, (void **)&pData);
    m_errorMonitor->VerifyFound();

    // Unmap the memory to avoid re-map error
    vkUnmapMemory(m_device->device(), mem);
    // overstep allocation with VK_WHOLE_SIZE
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " with size of VK_WHOLE_SIZE oversteps total array size 0x");
    err = vkMapMemory(m_device->device(), mem, allocation_size + 1, VK_WHOLE_SIZE, 0, (void **)&pData);
    m_errorMonitor->VerifyFound();
    // overstep allocation w/o VK_WHOLE_SIZE
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " oversteps total array size 0x");
    err = vkMapMemory(m_device->device(), mem, 1, allocation_size, 0, (void **)&pData);
    m_errorMonitor->VerifyFound();
    // Now error due to unmapping memory that's not mapped
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Unmapping Memory without memory being mapped: ");
    vkUnmapMemory(m_device->device(), mem);
    m_errorMonitor->VerifyFound();
    // Now map memory and cause errors due to flushing invalid ranges
    err = vkMapMemory(m_device->device(), mem, 16, VK_WHOLE_SIZE, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);
    VkMappedMemoryRange mmr = {};
    mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mmr.memory = mem;
    mmr.offset = 15; // Error b/c offset less than offset of mapped mem
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, ") is less than Memory Object's offset (");
    vkFlushMappedMemoryRanges(m_device->device(), 1, &mmr);
    m_errorMonitor->VerifyFound();
    // Now flush range that oversteps mapped range
    vkUnmapMemory(m_device->device(), mem);
    err = vkMapMemory(m_device->device(), mem, 0, 256, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);
    mmr.offset = 16;
    mmr.size = 256; // flushing bounds (272) exceed mapped bounds (256)
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, ") exceeds the Memory Object's upper-bound (");
    vkFlushMappedMemoryRanges(m_device->device(), 1, &mmr);
    m_errorMonitor->VerifyFound();

    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!pass) {
        vkFreeMemory(m_device->device(), mem, NULL);
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }
    // TODO : If we can get HOST_VISIBLE w/o HOST_COHERENT we can test cases of
    //  MEMTRACK_INVALID_MAP in validateAndCopyNoncoherentMemoryToDriver()

    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkFreeMemory(m_device->device(), mem, NULL);
}

TEST_F(VkLayerTest, EnableWsiBeforeUse) {
    VkResult err;
    bool pass;

    // FIXME: After we turn on this code for non-Linux platforms, uncomment the
    // following declaration (which is temporarily being moved below):
    //    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    uint32_t swapchain_image_count = 0;
    //    VkImage swapchain_images[1] = {VK_NULL_HANDLE};
    uint32_t image_index = 0;
    //    VkPresentInfoKHR present_info = {};

    ASSERT_NO_FATAL_FAILURE(InitState());

#ifdef NEED_TO_TEST_THIS_ON_PLATFORM
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    // Use the functions from the VK_KHR_android_surface extension without
    // enabling that extension:

    // Create a surface:
    VkAndroidSurfaceCreateInfoKHR android_create_info = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkCreateAndroidSurfaceKHR(instance(), &android_create_info, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();
#endif // VK_USE_PLATFORM_ANDROID_KHR

#if defined(VK_USE_PLATFORM_MIR_KHR)
    // Use the functions from the VK_KHR_mir_surface extension without enabling
    // that extension:

    // Create a surface:
    VkMirSurfaceCreateInfoKHR mir_create_info = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkCreateMirSurfaceKHR(instance(), &mir_create_info, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Tell whether an mir_connection supports presentation:
    MirConnection *mir_connection = NULL;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    vkGetPhysicalDeviceMirPresentationSupportKHR(gpu(), 0, mir_connection, visual_id);
    m_errorMonitor->VerifyFound();
#endif // VK_USE_PLATFORM_MIR_KHR

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    // Use the functions from the VK_KHR_wayland_surface extension without
    // enabling that extension:

    // Create a surface:
    VkWaylandSurfaceCreateInfoKHR wayland_create_info = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkCreateWaylandSurfaceKHR(instance(), &wayland_create_info, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Tell whether an wayland_display supports presentation:
    struct wl_display wayland_display = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    vkGetPhysicalDeviceWaylandPresentationSupportKHR(gpu(), 0, &wayland_display);
    m_errorMonitor->VerifyFound();
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#endif // NEED_TO_TEST_THIS_ON_PLATFORM

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    // FIXME: REMOVE THIS HERE, AND UNCOMMENT ABOVE, WHEN THIS TEST HAS BEEN PORTED
    // TO NON-LINUX PLATFORMS:
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    // Use the functions from the VK_KHR_win32_surface extension without
    // enabling that extension:

    // Create a surface:
    VkWin32SurfaceCreateInfoKHR win32_create_info = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkCreateWin32SurfaceKHR(instance(), &win32_create_info, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Tell whether win32 supports presentation:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    vkGetPhysicalDeviceWin32PresentationSupportKHR(gpu(), 0);
    m_errorMonitor->VerifyFound();
// Set this (for now, until all platforms are supported and tested):
#define NEED_TO_TEST_THIS_ON_PLATFORM
#endif // VK_USE_PLATFORM_WIN32_KHR

#if defined(VK_USE_PLATFORM_XCB_KHR)
    // FIXME: REMOVE THIS HERE, AND UNCOMMENT ABOVE, WHEN THIS TEST HAS BEEN PORTED
    // TO NON-LINUX PLATFORMS:
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    // Use the functions from the VK_KHR_xcb_surface extension without enabling
    // that extension:

    // Create a surface:
    VkXcbSurfaceCreateInfoKHR xcb_create_info = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkCreateXcbSurfaceKHR(instance(), &xcb_create_info, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Tell whether an xcb_visualid_t supports presentation:
    xcb_connection_t *xcb_connection = NULL;
    xcb_visualid_t visual_id = 0;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    vkGetPhysicalDeviceXcbPresentationSupportKHR(gpu(), 0, xcb_connection, visual_id);
    m_errorMonitor->VerifyFound();
// Set this (for now, until all platforms are supported and tested):
#define NEED_TO_TEST_THIS_ON_PLATFORM
#endif // VK_USE_PLATFORM_XCB_KHR

#if defined(VK_USE_PLATFORM_XLIB_KHR)
    // Use the functions from the VK_KHR_xlib_surface extension without enabling
    // that extension:

    // Create a surface:
    VkXlibSurfaceCreateInfoKHR xlib_create_info = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkCreateXlibSurfaceKHR(instance(), &xlib_create_info, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Tell whether an Xlib VisualID supports presentation:
    Display *dpy = NULL;
    VisualID visual = 0;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    vkGetPhysicalDeviceXlibPresentationSupportKHR(gpu(), 0, dpy, visual);
    m_errorMonitor->VerifyFound();
// Set this (for now, until all platforms are supported and tested):
#define NEED_TO_TEST_THIS_ON_PLATFORM
#endif // VK_USE_PLATFORM_XLIB_KHR

// Use the functions from the VK_KHR_surface extension without enabling
// that extension:

#ifdef NEED_TO_TEST_THIS_ON_PLATFORM
    // Destroy a surface:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    vkDestroySurfaceKHR(instance(), surface, NULL);
    m_errorMonitor->VerifyFound();

    // Check if surface supports presentation:
    VkBool32 supported = false;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkGetPhysicalDeviceSurfaceSupportKHR(gpu(), 0, surface, &supported);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Check surface capabilities:
    VkSurfaceCapabilitiesKHR capabilities = {};
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu(), surface, &capabilities);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Check surface formats:
    uint32_t format_count = 0;
    VkSurfaceFormatKHR *formats = NULL;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, &format_count, formats);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Check surface present modes:
    uint32_t present_mode_count = 0;
    VkSurfaceFormatKHR *present_modes = NULL;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, &present_mode_count, present_modes);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();
#endif // NEED_TO_TEST_THIS_ON_PLATFORM

    // Use the functions from the VK_KHR_swapchain extension without enabling
    // that extension:

    // Create a swapchain:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    err = vkCreateSwapchainKHR(m_device->device(), &swapchain_create_info, NULL, &swapchain);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Get the images from the swapchain:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkGetSwapchainImagesKHR(m_device->device(), swapchain, &swapchain_image_count, NULL);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Add a fence to avoid (justifiable) error about not providing fence OR semaphore
    VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
    VkFence fence;
    err = vkCreateFence(m_device->device(), &fci, nullptr, &fence);

    // Try to acquire an image:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    err = vkAcquireNextImageKHR(m_device->device(), swapchain, 0, VK_NULL_HANDLE, fence, &image_index);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    vkDestroyFence(m_device->device(), fence, nullptr);

    // Try to present an image:
    //
    // NOTE: Currently can't test this because a real swapchain is needed (as
    // opposed to the fake one we created) in order for the layer to lookup the
    // VkDevice used to enable the extension:

    // Destroy the swapchain:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "extension was not enabled for this");
    vkDestroySwapchainKHR(m_device->device(), swapchain, NULL);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, MapMemWithoutHostVisibleBit) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage image;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;

    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (!pass) { // If we can't find any unmappable memory this test doesn't
                 // make sense
        vkDestroyImage(m_device->device(), image, NULL);
        return;
    }

    // allocate memory
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    // Try to bind free memory that has been freed
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    ASSERT_VK_SUCCESS(err);

    // Map memory as if to initialize the image
    void *mappedAddress = NULL;
    err = vkMapMemory(m_device->device(), mem, 0, VK_WHOLE_SIZE, 0, &mappedAddress);

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), image, NULL);
    vkFreeMemory(m_device->device(), mem, NULL);
}

TEST_F(VkLayerTest, RebindMemory) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "which has already been bound to mem object");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage image;
    VkDeviceMemory mem1;
    VkDeviceMemory mem2;
    VkMemoryRequirements mem_reqs;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    // Introduce failure, do NOT set memProps to
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    mem_alloc.memoryTypeIndex = 1;
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_TRUE(pass);

    // allocate 2 memory objects
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem1);
    ASSERT_VK_SUCCESS(err);
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem2);
    ASSERT_VK_SUCCESS(err);

    // Bind first memory object to Image object
    err = vkBindImageMemory(m_device->device(), image, mem1, 0);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, try to bind a different memory object to
    // the same image object
    err = vkBindImageMemory(m_device->device(), image, mem2, 0);

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), image, NULL);
    vkFreeMemory(m_device->device(), mem1, NULL);
    vkFreeMemory(m_device->device(), mem2, NULL);
}

TEST_F(VkLayerTest, SubmitSignaledFence) {
    vk_testing::Fence testFence;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "submitted in SIGNALED state.  Fences "
                                                                        "must be reset before being submitted");

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();
    m_commandBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer();

    testFence.init(*m_device, fenceInfo);

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    vkQueueSubmit(m_device->m_queue, 1, &submit_info, testFence.handle());
    vkQueueWaitIdle(m_device->m_queue);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidUsageBits) {
    TEST_DESCRIPTION("Specify wrong usage for image then create conflicting view of image "
                     "Initialize buffer with wrong usage then perform copy expecting errors "
                     "from both the image and the buffer (2 calls)");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid usage flag for image ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkImageObj image(m_device);
    // Initialize image with USAGE_TRANSIENT_ATTACHMENT
    image.init(128, 128, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageView dsv;
    VkImageViewCreateInfo dsvci = {};
    dsvci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    dsvci.image = image.handle();
    dsvci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    dsvci.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    dsvci.subresourceRange.layerCount = 1;
    dsvci.subresourceRange.baseMipLevel = 0;
    dsvci.subresourceRange.levelCount = 1;
    dsvci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    // Create a view with depth / stencil aspect for image with different usage
    vkCreateImageView(m_device->device(), &dsvci, NULL, &dsv);

    m_errorMonitor->VerifyFound();

    // Initialize buffer with TRANSFER_DST usage
    vk_testing::Buffer buffer;
    VkMemoryPropertyFlags reqs = 0;
    buffer.init_as_dst(*m_device, 128 * 128, reqs);
    VkBufferImageCopy region = {};
    region.bufferRowLength = 128;
    region.bufferImageHeight = 128;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.height = 16;
    region.imageExtent.width = 16;
    region.imageExtent.depth = 1;

    // Buffer usage not set to TRANSFER_SRC and image usage not set to
    // TRANSFER_DST
    BeginCommandBuffer();

    // two separate errors from this call:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "image should have VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "buffer should have VK_BUFFER_USAGE_TRANSFER_SRC_BIT");

    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_errorMonitor->VerifyFound();
}


#endif // MEM_TRACKER_TESTS

#if OBJ_TRACKER_TESTS

TEST_F(VkLayerTest, LeakAnObject) {
    VkResult err;

    TEST_DESCRIPTION("Create a fence and destroy its device without first destroying the fence.");

    // Note that we have to create a new device since destroying the
    // framework's device causes Teardown() to fail and just calling Teardown
    // will destroy the errorMonitor.

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "has not been destroyed.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    const std::vector<VkQueueFamilyProperties> queue_props = m_device->queue_props;
    std::vector<VkDeviceQueueCreateInfo> queue_info;
    queue_info.reserve(queue_props.size());
    std::vector<std::vector<float>> queue_priorities;
    for (uint32_t i = 0; i < (uint32_t)queue_props.size(); i++) {
        VkDeviceQueueCreateInfo qi = {};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.pNext = NULL;
        qi.queueFamilyIndex = i;
        qi.queueCount = queue_props[i].queueCount;
        queue_priorities.emplace_back(qi.queueCount, 0.0f);
        qi.pQueuePriorities = queue_priorities[i].data();
        queue_info.push_back(qi);
    }

    std::vector<const char *> device_extension_names;

    // The sacrificial device object
    VkDevice testDevice;
    VkDeviceCreateInfo device_create_info = {};
    auto features = m_device->phy().features();
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = NULL;
    device_create_info.queueCreateInfoCount = queue_info.size();
    device_create_info.pQueueCreateInfos = queue_info.data();
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;
    device_create_info.pEnabledFeatures = &features;
    err = vkCreateDevice(gpu(), &device_create_info, NULL, &testDevice);
    ASSERT_VK_SUCCESS(err);

    VkFence fence;
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;
    err = vkCreateFence(testDevice, &fence_create_info, NULL, &fence);
    ASSERT_VK_SUCCESS(err);

    // Induce failure by not calling vkDestroyFence
    vkDestroyDevice(testDevice, NULL);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidCommandPoolConsistency) {

    TEST_DESCRIPTION("Allocate command buffers from one command pool and "
                     "attempt to delete them from another.");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "FreeCommandBuffers is attempting to free Command Buffer");

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkCommandPool command_pool_one;
    VkCommandPool command_pool_two;

    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool_one);

    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool_two);

    VkCommandBuffer command_buffer[9];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool_one;
    command_buffer_allocate_info.commandBufferCount = 9;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    vkFreeCommandBuffers(m_device->device(), command_pool_two, 4, &command_buffer[3]);

    m_errorMonitor->VerifyFound();

    vkDestroyCommandPool(m_device->device(), command_pool_one, NULL);
    vkDestroyCommandPool(m_device->device(), command_pool_two, NULL);
}

TEST_F(VkLayerTest, InvalidDescriptorPoolConsistency) {
    VkResult err;

    TEST_DESCRIPTION("Allocate descriptor sets from one DS pool and "
                     "attempt to delete them from another.");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "FreeDescriptorSets is attempting to free descriptorSet");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.flags = 0;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool_one;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool_one);
    ASSERT_VK_SUCCESS(err);

    // Create a second descriptor pool
    VkDescriptorPool ds_pool_two;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool_two);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool_one;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    err = vkFreeDescriptorSets(m_device->device(), ds_pool_two, 1, &descriptorSet);

    m_errorMonitor->VerifyFound();

    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool_one, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool_two, NULL);
}

TEST_F(VkLayerTest, CreateUnknownObject) {
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Image Object ");

    TEST_DESCRIPTION("Pass an invalid image object handle into a Vulkan API call.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Pass bogus handle into GetImageMemoryRequirements
    VkMemoryRequirements mem_reqs;
    uint64_t fakeImageHandle = 0xCADECADE;
    VkImage fauxImage = reinterpret_cast<VkImage &>(fakeImageHandle);

    vkGetImageMemoryRequirements(m_device->device(), fauxImage, &mem_reqs);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, PipelineNotBound) {
    VkResult err;

    TEST_DESCRIPTION("Pass in an invalid pipeline object handle into a Vulkan API call.");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Pipeline Object ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipeline badPipeline = (VkPipeline)((size_t)0xbaadb1be);

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, badPipeline);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, BindImageInvalidMemoryType) {
    VkResult err;

    TEST_DESCRIPTION("Test validation check for an invalid memory type index "
                     "during bind[Buffer|Image]Memory time");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image, allocate memory, set a bad typeIndex and then try to
    // bind it
    VkImage image;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;

    // Introduce Failure, select invalid TypeIndex
    VkPhysicalDeviceMemoryProperties memory_info;

    vkGetPhysicalDeviceMemoryProperties(gpu(), &memory_info);
    unsigned int i;
    for (i = 0; i < memory_info.memoryTypeCount; i++) {
        if ((mem_reqs.memoryTypeBits & (1 << i)) == 0) {
            mem_alloc.memoryTypeIndex = i;
            break;
        }
    }
    if (i >= memory_info.memoryTypeCount) {
        printf("No invalid memory type index could be found; skipped.\n");
        vkDestroyImage(m_device->device(), image, NULL);
        return;
    }

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "for this object type are not compatible with the memory");

    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    (void)err;

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), image, NULL);
    vkFreeMemory(m_device->device(), mem, NULL);
}

TEST_F(VkLayerTest, BindInvalidMemory) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Device Memory Object ");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage image;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;

    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_TRUE(pass);

    // allocate memory
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, free memory before binding
    vkFreeMemory(m_device->device(), mem, NULL);

    // Try to bind free memory that has been freed
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    // This may very well return an error.
    (void)err;

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), image, NULL);
}

TEST_F(VkLayerTest, BindMemoryToDestroyedObject) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Image Object ");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image object, allocate memory, destroy the object and then try
    // to bind it
    VkImage image;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_TRUE(pass);

    // Allocate memory
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, destroy Image object before binding
    vkDestroyImage(m_device->device(), image, NULL);
    ASSERT_VK_SUCCESS(err);

    // Now Try to bind memory to this destroyed object
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    // This may very well return an error.
    (void)err;

    m_errorMonitor->VerifyFound();

    vkFreeMemory(m_device->device(), mem, NULL);
}

#endif // OBJ_TRACKER_TESTS

#if DRAW_STATE_TESTS

TEST_F(VkLayerTest, ImageSampleCounts) {

    TEST_DESCRIPTION("Use bad sample counts in image transfer calls to trigger "
                     "validation errors.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkMemoryPropertyFlags reqs = 0;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 256;
    image_create_info.extent.height = 256;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.flags = 0;

    VkImageBlit blit_region = {};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcSubresource.mipLevel = 0;
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstSubresource.mipLevel = 0;

    // Create two images, the source with sampleCount = 2, and attempt to blit
    // between them
    {
        image_create_info.samples = VK_SAMPLE_COUNT_2_BIT;
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        vk_testing::Image src_image;
        src_image.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        vk_testing::Image dst_image;
        dst_image.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);
        m_commandBuffer->BeginCommandBuffer();
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "was created with a sample count "
                                                                            "of VK_SAMPLE_COUNT_2_BIT but "
                                                                            "must be VK_SAMPLE_COUNT_1_BIT");
        vkCmdBlitImage(m_commandBuffer->GetBufferHandle(), src_image.handle(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                       dst_image.handle(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, &blit_region, VK_FILTER_NEAREST);
        m_errorMonitor->VerifyFound();
        m_commandBuffer->EndCommandBuffer();
    }

    // Create two images, the dest with sampleCount = 4, and attempt to blit
    // between them
    {
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        vk_testing::Image src_image;
        src_image.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);
        image_create_info.samples = VK_SAMPLE_COUNT_4_BIT;
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        vk_testing::Image dst_image;
        dst_image.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);
        m_commandBuffer->BeginCommandBuffer();
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "was created with a sample count "
                                                                            "of VK_SAMPLE_COUNT_4_BIT but "
                                                                            "must be VK_SAMPLE_COUNT_1_BIT");
        vkCmdBlitImage(m_commandBuffer->GetBufferHandle(), src_image.handle(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                       dst_image.handle(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, &blit_region, VK_FILTER_NEAREST);
        m_errorMonitor->VerifyFound();
        m_commandBuffer->EndCommandBuffer();
    }

    VkBufferImageCopy copy_region = {};
    copy_region.bufferRowLength = 128;
    copy_region.bufferImageHeight = 128;
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.layerCount = 1;
    copy_region.imageExtent.height = 64;
    copy_region.imageExtent.width = 64;
    copy_region.imageExtent.depth = 1;

    // Create src buffer and dst image with sampleCount = 4 and attempt to copy
    // buffer to image
    {
        vk_testing::Buffer src_buffer;
        VkMemoryPropertyFlags reqs = 0;
        src_buffer.init_as_src(*m_device, 128 * 128 * 4, reqs);
        image_create_info.samples = VK_SAMPLE_COUNT_8_BIT;
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        vk_testing::Image dst_image;
        dst_image.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);
        m_commandBuffer->BeginCommandBuffer();
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "was created with a sample count "
                                                                            "of VK_SAMPLE_COUNT_8_BIT but "
                                                                            "must be VK_SAMPLE_COUNT_1_BIT");
        vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), src_buffer.handle(), dst_image.handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
        m_errorMonitor->VerifyFound();
        m_commandBuffer->EndCommandBuffer();
    }

    // Create dst buffer and src image with sampleCount = 2 and attempt to copy
    // image to buffer
    {
        vk_testing::Buffer dst_buffer;
        dst_buffer.init_as_dst(*m_device, 128 * 128 * 4, reqs);
        image_create_info.samples = VK_SAMPLE_COUNT_2_BIT;
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        vk_testing::Image src_image;
        src_image.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);
        m_commandBuffer->BeginCommandBuffer();
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "was created with a sample count "
                                                                            "of VK_SAMPLE_COUNT_2_BIT but "
                                                                            "must be VK_SAMPLE_COUNT_1_BIT");
        vkCmdCopyImageToBuffer(m_commandBuffer->GetBufferHandle(), src_image.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               dst_buffer.handle(), 1, &copy_region);
        m_errorMonitor->VerifyFound();
        m_commandBuffer->EndCommandBuffer();
    }
}

TEST_F(VkLayerTest, BlitImageFormats) {

    // Image blit with mismatched formats
    const char * expected_message =
        "vkCmdBlitImage: If one of srcImage and dstImage images has signed/unsigned integer format,"
        " the other one must also have signed/unsigned integer format";

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImageObj src_image(m_device);
    src_image.init(64, 64, VK_FORMAT_A2B10G10R10_UINT_PACK32, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_LINEAR, 0);
    VkImageObj dst_image(m_device);
    dst_image.init(64, 64, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_LINEAR, 0);
    VkImageObj dst_image2(m_device);
    dst_image2.init(64, 64, VK_FORMAT_R8G8B8A8_SINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_LINEAR, 0);

    VkImageBlit blitRegion = {};
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, expected_message);

    // Unsigned int vs not an int
    BeginCommandBuffer();
    vkCmdBlitImage(m_commandBuffer->handle(), src_image.image(), src_image.layout(), dst_image.image(),
                   dst_image.layout(), 1, &blitRegion, VK_FILTER_NEAREST);

    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, expected_message);

    // Unsigned int vs signed int
    vkCmdBlitImage(m_commandBuffer->handle(), src_image.image(), src_image.layout(), dst_image2.image(),
                   dst_image2.layout(), 1, &blitRegion, VK_FILTER_NEAREST);

    m_errorMonitor->VerifyFound();

    EndCommandBuffer();
}


TEST_F(VkLayerTest, DSImageTransferGranularityTests) {
    VkResult err;
    bool pass;

    TEST_DESCRIPTION("Tests for validaiton of Queue Family property minImageTransferGranularity.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    // If w/d/h granularity is 1, test is not meaningful
    // TODO: When virtual device limits are available, create a set of limits for this test that
    // will always have a granularity of > 1 for w, h, and d
    auto index = m_device->graphics_queue_node_index_;
    auto queue_family_properties = m_device->phy().queue_properties();

    if ((queue_family_properties[index].minImageTransferGranularity.depth < 4) ||
        (queue_family_properties[index].minImageTransferGranularity.width < 4) ||
        (queue_family_properties[index].minImageTransferGranularity.height < 4)) {
        return;
    }

    // Create two images of different types and try to copy between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 4;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    VkImageCopy copyRegion;
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset.x = 0;
    copyRegion.srcOffset.y = 0;
    copyRegion.srcOffset.z = 0;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset.x = 0;
    copyRegion.dstOffset.y = 0;
    copyRegion.dstOffset.z = 0;
    copyRegion.extent.width = 1;
    copyRegion.extent.height = 1;
    copyRegion.extent.depth = 1;

    // Introduce failure by setting srcOffset to a bad granularity value
    copyRegion.srcOffset.y = 3;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "queue family image transfer granularity");
    m_commandBuffer->CopyImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    m_errorMonitor->VerifyFound();

    // Introduce failure by setting extent to a bad granularity value
    copyRegion.srcOffset.y = 0;
    copyRegion.extent.width = 3;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "queue family image transfer granularity");
    m_commandBuffer->CopyImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    m_errorMonitor->VerifyFound();

    // Now do some buffer/image copies
    vk_testing::Buffer buffer;
    VkMemoryPropertyFlags reqs = 0;
    buffer.init_as_dst(*m_device, 128 * 128, reqs);
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 3;
    region.bufferImageHeight = 128;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.height = 16;
    region.imageExtent.width = 16;
    region.imageExtent.depth = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;

    // Introduce failure by setting bufferRowLength to a bad granularity value
    region.bufferRowLength = 3;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "queue family image transfer granularity");
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);
    m_errorMonitor->VerifyFound();
    region.bufferRowLength = 128;

    // Introduce failure by setting bufferOffset to a bad granularity value
    region.bufferOffset = 3;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "queue family image transfer granularity");
    vkCmdCopyImageToBuffer(m_commandBuffer->GetBufferHandle(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, buffer.handle(), 1,
                           &region);
    m_errorMonitor->VerifyFound();
    region.bufferOffset = 0;

    // Introduce failure by setting bufferImageHeight to a bad granularity value
    region.bufferImageHeight = 3;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "queue family image transfer granularity");
    vkCmdCopyImageToBuffer(m_commandBuffer->GetBufferHandle(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, buffer.handle(), 1,
                           &region);
    m_errorMonitor->VerifyFound();
    region.bufferImageHeight = 128;

    // Introduce failure by setting imageExtent to a bad granularity value
    region.imageExtent.width = 3;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "queue family image transfer granularity");
    vkCmdCopyImageToBuffer(m_commandBuffer->GetBufferHandle(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, buffer.handle(), 1,
                           &region);
    m_errorMonitor->VerifyFound();
    region.imageExtent.width = 16;

    // Introduce failure by setting imageOffset to a bad granularity value
    region.imageOffset.z = 3;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "queue family image transfer granularity");
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);
    m_errorMonitor->VerifyFound();

    EndCommandBuffer();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, MismatchedQueueFamiliesOnSubmit) {
    TEST_DESCRIPTION("Submit command buffer created using one queue family and "
                     "attempt to submit them on a queue created in a different "
                     "queue family.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // This test is meaningless unless we have multiple queue families
    auto queue_family_properties = m_device->phy().queue_properties();
    if (queue_family_properties.size() < 2) {
        return;
    }
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is being submitted on queue ");
    // Get safe index of another queue family
    uint32_t other_queue_family = (m_device->graphics_queue_node_index_ == 0) ? 1 : 0;
    ASSERT_NO_FATAL_FAILURE(InitState());
    // Create a second queue using a different queue family
    VkQueue other_queue;
    vkGetDeviceQueue(m_device->device(), other_queue_family, 0, &other_queue);

    // Record an empty cmd buffer
    VkCommandBufferBeginInfo cmdBufBeginDesc = {};
    cmdBufBeginDesc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(m_commandBuffer->handle(), &cmdBufBeginDesc);
    vkEndCommandBuffer(m_commandBuffer->handle());

    // And submit on the wrong queue
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(other_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, RenderPassPipelineSubpassMismatch) {
    TEST_DESCRIPTION("Use a pipeline for the wrong subpass in a render pass instance");
    ASSERT_NO_FATAL_FAILURE(InitState());

    // A renderpass with two subpasses, both writing the same attachment.
    VkAttachmentDescription attach[] = {
        { 0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        },
    };
    VkAttachmentReference ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpasses[] = {
        { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr,
            1, &ref, nullptr, nullptr, 0, nullptr },
        { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr,
            1, &ref, nullptr, nullptr, 0, nullptr },
    };
    VkSubpassDependency dep = {
        0, 1,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT
    };
    VkRenderPassCreateInfo rpci = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr,
        0, 1, attach, 2, subpasses, 1, &dep
    };
    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    VkImageObj image(m_device);
    image.init_no_layout(32, 32, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         VK_IMAGE_TILING_OPTIMAL, 0);
    VkImageView imageView = image.targetView(VK_FORMAT_R8G8B8A8_UNORM);

    VkFramebufferCreateInfo fbci = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr,
        0, rp, 1, &imageView, 32, 32, 1
    };
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fbci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    char const *vsSource =
        "#version 450\n"
        "void main() { gl_Position = vec4(1); }\n";
    char const *fsSource =
        "#version 450\n"
        "layout(location=0) out vec4 color;\n"
        "void main() { color = vec4(1); }\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    VkViewport view_port = {};
    m_viewports.push_back(view_port);
    pipe.SetViewport(m_viewports);
    VkRect2D rect = {};
    m_scissors.push_back(rect);
    pipe.SetScissor(m_scissors);

    VkPipelineLayoutCreateInfo plci = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr,
        0, 0, nullptr, 0, nullptr
    };
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);
    pipe.CreateVKPipeline(pl, rp);

    BeginCommandBuffer();

    VkRenderPassBeginInfo rpbi = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr,
        rp, fb, { { 0, 0, }, { 32, 32 } }, 0, nullptr
    };

    // subtest 1: bind in the wrong subpass
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdNextSubpass(m_commandBuffer->handle(), VK_SUBPASS_CONTENTS_INLINE);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "built for subpass 0 but used in subpass 1");
    vkCmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_errorMonitor->VerifyFound();

    vkCmdEndRenderPass(m_commandBuffer->handle());

    // subtest 2: bind in correct subpass, then transition to next subpass
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdNextSubpass(m_commandBuffer->handle(), VK_SUBPASS_CONTENTS_INLINE);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "built for subpass 0 but used in subpass 1");
    vkCmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_errorMonitor->VerifyFound();

    vkCmdEndRenderPass(m_commandBuffer->handle());

    EndCommandBuffer();

    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
}

TEST_F(VkLayerTest, RenderPassInvalidRenderArea) {
    TEST_DESCRIPTION("Generate INVALID_RENDER_AREA error by beginning renderpass"
                     "with extent outside of framebuffer");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot execute a render pass with renderArea "
                                                                        "not within the bound of the framebuffer.");

    // Framebuffer for render target is 256x256, exceed that for INVALID_RENDER_AREA
    m_renderPassBeginInfo.renderArea.extent.width = 257;
    m_renderPassBeginInfo.renderArea.extent.height = 257;
    BeginCommandBuffer();
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DisabledIndependentBlend) {
    TEST_DESCRIPTION("Generate INDEPENDENT_BLEND by disabling independent "
                     "blend and then specifying different blend states for two "
                     "attachements");
    VkPhysicalDeviceFeatures features = {};
    features.independentBlend = VK_FALSE;
    ASSERT_NO_FATAL_FAILURE(InitState(&features));

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Invalid Pipeline CreateInfo: If independent blend feature not "
                                         "enabled, all elements of pAttachments must be identical");

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    VkPipelineObj pipeline(m_device);
    VkRenderpassObj renderpass(m_device);
    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    pipeline.AddShader(&vs);

    VkPipelineColorBlendAttachmentState att_state1 = {}, att_state2 = {};
    att_state1.dstAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
    att_state1.blendEnable = VK_TRUE;
    att_state2.dstAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
    att_state2.blendEnable = VK_FALSE;
    pipeline.AddColorAttachment(0, &att_state1);
    pipeline.AddColorAttachment(1, &att_state2);
    pipeline.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderpass.handle());
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, RenderPassDepthStencilAttachmentUnused) {
    TEST_DESCRIPTION("Specify no depth attachement in renderpass then specify "
                     "depth attachments in subpass");
    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCreateRenderPass has no depth/stencil attachment, yet subpass");

    // Create a renderPass with a single color attachment
    VkAttachmentReference attach = {};
    attach.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    VkRenderPassCreateInfo rpci = {};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.attachmentCount = 1;
    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_B8G8R8A8_UNORM;
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    rpci.pAttachments = &attach_desc;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkRenderPass rp;
    subpass.pDepthStencilAttachment = &attach;
    subpass.pColorAttachments = NULL;
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, UnusedPreserveAttachment) {
    TEST_DESCRIPTION("Create a framebuffer where a subpass has a preserve "
                     "attachment reference of VK_ATTACHMENT_UNUSED");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "must not be VK_ATTACHMENT_UNUSED");

    VkAttachmentReference color_attach = {};
    color_attach.layout = VK_IMAGE_LAYOUT_GENERAL;
    color_attach.attachment = 0;
    uint32_t preserve_attachment = VK_ATTACHMENT_UNUSED;
    VkSubpassDescription subpass = {};
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attach;
    subpass.preserveAttachmentCount = 1;
    subpass.pPreserveAttachments = &preserve_attachment;

    VkRenderPassCreateInfo rpci = {};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.attachmentCount = 1;
    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_UNDEFINED;
    rpci.pAttachments = &attach_desc;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkRenderPass rp;
    VkResult result = vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);

    m_errorMonitor->VerifyFound();

    if (result == VK_SUCCESS) {
        vkDestroyRenderPass(m_device->device(), rp, NULL);
    }
}

TEST_F(VkLayerTest, CreateRenderPassResolveRequiresColorMsaa) {
    TEST_DESCRIPTION("Ensure that CreateRenderPass produces a validation error "
                     "when the source of a subpass multisample resolve "
                     "does not have multiple samples.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Subpass 0 requests multisample resolve from attachment 0 which has "
                                         "VK_SAMPLE_COUNT_1_BIT");

    VkAttachmentDescription attachments[] = {
        {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };

    VkAttachmentReference color = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference resolve = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &color, &resolve, nullptr, 0, nullptr};

    VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 2, attachments, 1, &subpass, 0, nullptr};

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);

    m_errorMonitor->VerifyFound();

    if (err == VK_SUCCESS)
        vkDestroyRenderPass(m_device->device(), rp, nullptr);
}

TEST_F(VkLayerTest, CreateRenderPassResolveRequiresSingleSampleDest) {
    TEST_DESCRIPTION("Ensure CreateRenderPass produces a validation error "
                     "when a subpass multisample resolve operation is "
                     "requested, and the destination of that resolve has "
                     "multiple samples.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Subpass 0 requests multisample resolve into attachment 1, which "
                                         "must have VK_SAMPLE_COUNT_1_BIT but has VK_SAMPLE_COUNT_4_BIT");

    VkAttachmentDescription attachments[] = {
        {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_4_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_4_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };

    VkAttachmentReference color = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference resolve = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &color, &resolve, nullptr, 0, nullptr};

    VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 2, attachments, 1, &subpass, 0, nullptr};

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);

    m_errorMonitor->VerifyFound();

    if (err == VK_SUCCESS)
        vkDestroyRenderPass(m_device->device(), rp, nullptr);
}

TEST_F(VkLayerTest, CreateRenderPassSubpassSampleCountConsistency) {
    TEST_DESCRIPTION("Ensure CreateRenderPass produces a validation error "
                     "when the color and depth attachments used by a subpass "
                     "have inconsistent sample counts");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Subpass 0 attempts to render to attachments with inconsistent sample counts");

    VkAttachmentDescription attachments[] = {
        {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_4_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };

    VkAttachmentReference color[] = {
        {
            0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };

    VkSubpassDescription subpass = {0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 2, color, nullptr, nullptr, 0, nullptr};

    VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 2, attachments, 1, &subpass, 0, nullptr};

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);

    m_errorMonitor->VerifyFound();

    if (err == VK_SUCCESS)
        vkDestroyRenderPass(m_device->device(), rp, nullptr);
}

TEST_F(VkLayerTest, FramebufferCreateErrors) {
    TEST_DESCRIPTION("Hit errors when attempting to create a framebuffer :\n"
                     " 1. Mismatch between framebuffer & renderPass attachmentCount\n"
                     " 2. Use a color image as depthStencil attachment\n"
                     " 3. Mismatch framebuffer & renderPass attachment formats\n"
                     " 4. Mismatch framebuffer & renderPass attachment #samples\n"
                     " 5. Framebuffer attachment w/ non-1 mip-levels\n"
                     " 6. Framebuffer attachment where dimensions don't match\n"
                     " 7. Framebuffer attachment w/o identity swizzle\n"
                     " 8. framebuffer dimensions exceed physical device limits\n");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCreateFramebuffer(): VkFramebufferCreateInfo attachmentCount of 2 "
                                         "does not match attachmentCount of 1 of ");

    // Create a renderPass with a single color attachment
    VkAttachmentReference attach = {};
    attach.layout = VK_IMAGE_LAYOUT_GENERAL;
    VkSubpassDescription subpass = {};
    subpass.pColorAttachments = &attach;
    VkRenderPassCreateInfo rpci = {};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.attachmentCount = 1;
    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_B8G8R8A8_UNORM;
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    rpci.pAttachments = &attach_desc;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    ASSERT_VK_SUCCESS(err);

    VkImageView ivs[2];
    ivs[0] = m_renderTargets[0]->targetView(VK_FORMAT_B8G8R8A8_UNORM);
    ivs[1] = m_renderTargets[0]->targetView(VK_FORMAT_B8G8R8A8_UNORM);
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = rp;
    // Set mis-matching attachmentCount
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = ivs;
    fb_info.width = 100;
    fb_info.height = 100;
    fb_info.layers = 1;

    VkFramebuffer fb;
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }
    vkDestroyRenderPass(m_device->device(), rp, NULL);

    // Create a renderPass with a depth-stencil attachment created with
    // IMAGE_USAGE_COLOR_ATTACHMENT
    // Add our color attachment to pDepthStencilAttachment
    subpass.pDepthStencilAttachment = &attach;
    subpass.pColorAttachments = NULL;
    VkRenderPass rp_ds;
    err = vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp_ds);
    ASSERT_VK_SUCCESS(err);
    // Set correct attachment count, but attachment has COLOR usage bit set
    fb_info.attachmentCount = 1;
    fb_info.renderPass = rp_ds;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " conflicts with the image's IMAGE_USAGE flags ");
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }
    vkDestroyRenderPass(m_device->device(), rp_ds, NULL);

    // Create new renderpass with alternate attachment format from fb
    attach_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    subpass.pDepthStencilAttachment = NULL;
    subpass.pColorAttachments = &attach;
    err = vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    ASSERT_VK_SUCCESS(err);

    // Cause error due to mis-matched formats between rp & fb
    //  rp attachment 0 now has RGBA8 but corresponding fb attach is BGRA8
    fb_info.renderPass = rp;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " has format of VK_FORMAT_B8G8R8A8_UNORM that does not match ");
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }
    vkDestroyRenderPass(m_device->device(), rp, NULL);

    // Create new renderpass with alternate sample count from fb
    attach_desc.format = VK_FORMAT_B8G8R8A8_UNORM;
    attach_desc.samples = VK_SAMPLE_COUNT_4_BIT;
    err = vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    ASSERT_VK_SUCCESS(err);

    // Cause error due to mis-matched sample count between rp & fb
    fb_info.renderPass = rp;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " has VK_SAMPLE_COUNT_1_BIT samples "
                                                                        "that do not match the "
                                                                        "VK_SAMPLE_COUNT_4_BIT ");
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }

    vkDestroyRenderPass(m_device->device(), rp, NULL);

    // Create a custom imageView with non-1 mip levels
    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageView view;
    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = image.handle();
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_B8G8R8A8_UNORM;
    ivci.subresourceRange.layerCount = 1;
    ivci.subresourceRange.baseMipLevel = 0;
    // Set level count 2 (only 1 is allowed for FB attachment)
    ivci.subresourceRange.levelCount = 2;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    err = vkCreateImageView(m_device->device(), &ivci, NULL, &view);
    ASSERT_VK_SUCCESS(err);
    // Re-create renderpass to have matching sample count
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    err = vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    ASSERT_VK_SUCCESS(err);

    fb_info.renderPass = rp;
    fb_info.pAttachments = &view;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " has mip levelCount of 2 but only ");
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }
    vkDestroyImageView(m_device->device(), view, NULL);
    // Update view to original color buffer and grow FB dimensions too big
    fb_info.pAttachments = ivs;
    fb_info.height = 1024;
    fb_info.width = 1024;
    fb_info.layers = 2;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " Attachment dimensions must be at "
                                                                        "least as large. ");
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }
    // Create view attachment with non-identity swizzle
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = image.handle();
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_B8G8R8A8_UNORM;
    ivci.subresourceRange.layerCount = 1;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.components.r = VK_COMPONENT_SWIZZLE_G;
    ivci.components.g = VK_COMPONENT_SWIZZLE_R;
    ivci.components.b = VK_COMPONENT_SWIZZLE_A;
    ivci.components.a = VK_COMPONENT_SWIZZLE_B;
    err = vkCreateImageView(m_device->device(), &ivci, NULL, &view);
    ASSERT_VK_SUCCESS(err);

    fb_info.pAttachments = &view;
    fb_info.height = 100;
    fb_info.width = 100;
    fb_info.layers = 1;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " has non-identy swizzle. All "
                                                                        "framebuffer attachments must have "
                                                                        "been created with the identity "
                                                                        "swizzle. ");
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }
    vkDestroyImageView(m_device->device(), view, NULL);
    // Request fb that exceeds max dimensions
    // reset attachment to color attachment
    fb_info.pAttachments = ivs;
    fb_info.width = m_device->props.limits.maxFramebufferWidth + 1;
    fb_info.height = m_device->props.limits.maxFramebufferHeight + 1;
    fb_info.layers = m_device->props.limits.maxFramebufferLayers + 1;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " Requested VkFramebufferCreateInfo "
                                                                        "dimensions exceed physical device "
                                                                        "limits. ");
    err = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);

    m_errorMonitor->VerifyFound();
    if (err == VK_SUCCESS) {
        vkDestroyFramebuffer(m_device->device(), fb, NULL);
    }

    vkDestroyRenderPass(m_device->device(), rp, NULL);
}

TEST_F(VkLayerTest, DynamicDepthBiasNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Depth Bias dynamic "
                     "state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic depth bias
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Dynamic depth bias state not set for this command buffer");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailDepthBias);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicLineWidthNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Line Width dynamic "
                     "state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic line width
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Dynamic line width state not set for this command buffer");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailLineWidth);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicViewportNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Viewport dynamic "
                     "state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic viewport state
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Dynamic viewport(s) 0 are used by pipeline state object, but were not provided");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailViewport);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicScissorNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Scissor dynamic "
                     "state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic scissor state
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Dynamic scissor(s) 0 are used by pipeline state object, but were not provided");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailScissor);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicBlendConstantsNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Blend Constants "
                     "dynamic state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic blend constant state
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Dynamic blend constants state not set for this command buffer");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailBlend);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicDepthBoundsNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Depth Bounds dynamic "
                     "state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    if (!m_device->phy().features().depthBounds) {
        printf("Device does not support depthBounds test; skipped.\n");
        return;
    }
    // Dynamic depth bounds
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Dynamic depth bounds state not set for this command buffer");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailDepthBounds);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicStencilReadNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Stencil Read dynamic "
                     "state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic stencil read mask
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Dynamic stencil read mask state not set for this command buffer");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailStencilReadMask);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicStencilWriteNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Stencil Write dynamic"
                     " state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic stencil write mask
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Dynamic stencil write mask state not set for this command buffer");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailStencilWriteMask);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DynamicStencilRefNotBound) {
    TEST_DESCRIPTION("Run a simple draw calls to validate failure when Stencil Ref dynamic "
                     "state is required but not correctly bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Dynamic stencil reference
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Dynamic stencil reference state not set for this command buffer");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailStencilReference);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, IndexBufferNotBound) {
    TEST_DESCRIPTION("Run an indexed draw call without an index buffer bound.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Index buffer object not bound to this command buffer when Indexed ");
    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailIndexBuffer);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CommandBufferTwoSubmits) {
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "was begun w/ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT set, but has "
                                         "been submitted");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // We luck out b/c by default the framework creates CB w/ the
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT set
    BeginCommandBuffer();
    m_commandBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer();

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    ASSERT_VK_SUCCESS(err);

    // Cause validation error by re-submitting cmd buffer that should only be
    // submitted once
    err = vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, AllocDescriptorFromEmptyPool) {
    // Initiate Draw w/o a PSO bound
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Unable to allocate 1 descriptors of "
                                                                        "type "
                                                                        "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Create Pool w/ 1 Sampler descriptor, but try to alloc Uniform Buffer
    // descriptor from it
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.flags = 0;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);

    m_errorMonitor->VerifyFound();

    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, FreeDescriptorFromOneShotPool) {
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "It is invalid to call vkFreeDescriptorSets() with a pool created "
                                         "without setting VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.flags = 0;
    // Not specifying VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT means
    // app can only call vkResetDescriptorPool on this pool.;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    err = vkFreeDescriptorSets(m_device->device(), ds_pool, 1, &descriptorSet);
    m_errorMonitor->VerifyFound();

    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidDescriptorPool) {
    // Attempt to clear Descriptor Pool with bad object.
    // ObjectTracker should catch this.

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Descriptor Pool Object 0xbaad6001");
    uint64_t fake_pool_handle = 0xbaad6001;
    VkDescriptorPool bad_pool = reinterpret_cast<VkDescriptorPool &>(fake_pool_handle);
    vkResetDescriptorPool(device(), bad_pool, 0);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkPositiveLayerTest, InvalidDescriptorSet) {
    // Attempt to bind an invalid Descriptor Set to a valid Command Buffer
    // ObjectTracker should catch this.
    // Create a valid cmd buffer
    // call vkCmdBindDescriptorSets w/ false Descriptor Set

    uint64_t fake_set_handle = 0xbaad6001;
    VkDescriptorSet bad_set = reinterpret_cast<VkDescriptorSet &>(fake_set_handle);
    VkResult err;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Descriptor Set Object 0xbaad6001");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDescriptorSetLayoutBinding layout_bindings[1] = {};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSetLayoutCreateInfo dslci = {};
    dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslci.pNext = NULL;
    dslci.bindingCount = 1;
    dslci.pBindings = layout_bindings;
    err = vkCreateDescriptorSetLayout(device(), &dslci, NULL, &descriptor_set_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayout pipeline_layout;
    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pNext = NULL;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &descriptor_set_layout;
    err = vkCreatePipelineLayout(device(), &plci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &bad_set, 0,
                            NULL);
    m_errorMonitor->VerifyFound();
    EndCommandBuffer();
    vkDestroyPipelineLayout(device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device(), descriptor_set_layout, NULL);
}

TEST_F(VkLayerTest, InvalidDescriptorSetLayout) {
    // Attempt to create a Pipeline Layout with an invalid Descriptor Set Layout.
    // ObjectTracker should catch this.
    uint64_t fake_layout_handle = 0xbaad6001;
    VkDescriptorSetLayout bad_layout = reinterpret_cast<VkDescriptorSetLayout &>(fake_layout_handle);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Descriptor Set Layout Object 0xbaad6001");
    ASSERT_NO_FATAL_FAILURE(InitState());
    VkPipelineLayout pipeline_layout;
    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pNext = NULL;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &bad_layout;
    vkCreatePipelineLayout(device(), &plci, NULL, &pipeline_layout);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, WriteDescriptorSetIntegrityCheck) {
    TEST_DESCRIPTION("This test verifies some requirements of chapter 13.2.3 of the Vulkan Spec "
                     "1) A uniform buffer update must have a valid buffer index."
                     "2) When using an array of descriptors in a single WriteDescriptor,"
                     "     the descriptor types and stageflags must all be the same."
                     "3) Immutable Sampler state must match across descriptors");

    const char *invalid_BufferInfo_ErrorMessage =
        "vkUpdateDescriptorSets: if pDescriptorWrites[0].descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, "
        "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC or "
        "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, pDescriptorWrites[0].pBufferInfo must not be NULL";
    const char *stateFlag_ErrorMessage = "Attempting write update to descriptor set ";
    const char *immutable_ErrorMessage = "Attempting write update to descriptor set ";

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_BufferInfo_ErrorMessage);

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkDescriptorPoolSize ds_type_count[4] = {};
    ds_type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count[0].descriptorCount = 1;
    ds_type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count[1].descriptorCount = 1;
    ds_type_count[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count[2].descriptorCount = 1;
    ds_type_count[3].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    ds_type_count[3].descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = sizeof(ds_type_count) / sizeof(VkDescriptorPoolSize);
    ds_pool_ci.pPoolSizes = ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding layout_binding[3] = {};
    layout_binding[0].binding = 0;
    layout_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding[0].descriptorCount = 1;
    layout_binding[0].stageFlags = VK_SHADER_STAGE_ALL;
    layout_binding[0].pImmutableSamplers = NULL;

    layout_binding[1].binding = 1;
    layout_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    layout_binding[1].descriptorCount = 1;
    layout_binding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_binding[1].pImmutableSamplers = NULL;

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;

    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    layout_binding[2].binding = 2;
    layout_binding[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    layout_binding[2].descriptorCount = 1;
    layout_binding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_binding[2].pImmutableSamplers = static_cast<VkSampler *>(&sampler);

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = sizeof(layout_binding) / sizeof(VkDescriptorSetLayoutBinding);
    ds_layout_ci.pBindings = layout_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    VkDescriptorSet descriptorSet;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    // 1) The uniform buffer is intentionally invalid here
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->VerifyFound();

    // Create a buffer to update the descriptor with
    uint32_t qfi = 0;
    VkBufferCreateInfo buffCI = {};
    buffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffCI.size = 1024;
    buffCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffCI.queueFamilyIndexCount = 1;
    buffCI.pQueueFamilyIndices = &qfi;

    VkBuffer dyub;
    err = vkCreateBuffer(m_device->device(), &buffCI, NULL, &dyub);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorBufferInfo buffInfo = {};
    buffInfo.buffer = dyub;
    buffInfo.offset = 0;
    buffInfo.range = 1024;

    descriptor_write.pBufferInfo = &buffInfo;
    descriptor_write.descriptorCount = 2;

    // 2) The stateFlags don't match between the first and second descriptor
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, stateFlag_ErrorMessage);
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->VerifyFound();

    // 3) The second descriptor has a null_ptr pImmutableSamplers and
    // the third descriptor contains an immutable sampler
    descriptor_write.dstBinding = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

    // Make pImageInfo index non-null to avoid complaints of it missing
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_write.pImageInfo = &imageInfo;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, immutable_ErrorMessage);
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->VerifyFound();

    vkDestroyBuffer(m_device->device(), dyub, NULL);
    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidCmdBufferBufferDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to a buffer dependency being destroyed.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkBuffer buffer;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buf_info.size = 256;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult err = vkCreateBuffer(m_device->device(), &buf_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    vkGetBufferMemoryRequirements(m_device->device(), buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = 256;
    bool pass = false;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }
    err = vkAllocateMemory(m_device->device(), &alloc_info, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);

    m_commandBuffer->BeginCommandBuffer();
    vkCmdFillBuffer(m_commandBuffer->GetBufferHandle(), buffer, 0, VK_WHOLE_SIZE, 0);
    m_commandBuffer->EndCommandBuffer();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound buffer ");
    // Destroy buffer dependency prior to submit to cause ERROR
    vkDestroyBuffer(m_device->device(), buffer, NULL);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->VerifyFound();
    vkFreeMemory(m_device->handle(), mem, NULL);
}

TEST_F(VkLayerTest, InvalidCmdBufferBufferViewDestroyed) {
    TEST_DESCRIPTION("Delete bufferView bound to cmd buffer, then attempt to submit cmd buffer.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count;
    ds_type_count.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding layout_binding;
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &layout_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    VkDescriptorSet descriptor_set;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkBuffer buffer;
    uint32_t queue_family_index = 0;
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = 1024;
    buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    buffer_create_info.queueFamilyIndexCount = 1;
    buffer_create_info.pQueueFamilyIndices = &queue_family_index;

    err = vkCreateBuffer(m_device->device(), &buffer_create_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    VkMemoryRequirements memory_reqs;
    VkDeviceMemory buffer_memory;

    VkMemoryAllocateInfo memory_info = {};
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.allocationSize = 0;
    memory_info.memoryTypeIndex = 0;

    vkGetBufferMemoryRequirements(m_device->device(), buffer, &memory_reqs);
    memory_info.allocationSize = memory_reqs.size;
    bool pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
    ASSERT_TRUE(pass);

    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &buffer_memory);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, buffer_memory, 0);
    ASSERT_VK_SUCCESS(err);

    VkBufferView view;
    VkBufferViewCreateInfo bvci = {};
    bvci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bvci.buffer = buffer;
    bvci.format = VK_FORMAT_R8_UNORM;
    bvci.range = VK_WHOLE_SIZE;

    err = vkCreateBufferView(m_device->device(), &bvci, NULL, &view);
    ASSERT_VK_SUCCESS(err);

    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    descriptor_write.pTexelBufferView = &view;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0, r8) uniform imageBuffer s;\n"
                           "layout(location=0) out vec4 x;\n"
                           "void main(){\n"
                           "   x = imageLoad(s, 0);\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot submit cmd buffer using deleted buffer view ");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound buffer view ");

    BeginCommandBuffer();
    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    vkCmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    VkRect2D scissor = {{0, 0}, {16, 16}};
    vkCmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    // Bind pipeline to cmd buffer
    vkCmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptor_set, 0, nullptr);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();

    // Delete BufferView in order to invalidate cmd buffer
    vkDestroyBufferView(m_device->device(), view, NULL);
    // Now attempt submit of cmd buffer
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();

    // Clean-up
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkFreeMemory(m_device->device(), buffer_memory, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidCmdBufferImageDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to an image dependency being destroyed.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImage image;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_create_info.flags = 0;
    VkResult err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);
    // Have to bind memory to image before recording cmd in cmd buffer using it
    VkMemoryRequirements mem_reqs;
    VkDeviceMemory image_mem;
    bool pass;
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &image_mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, image_mem, 0);
    ASSERT_VK_SUCCESS(err);

    m_commandBuffer->BeginCommandBuffer();
    VkClearColorValue ccv;
    ccv.float32[0] = 1.0f;
    ccv.float32[1] = 1.0f;
    ccv.float32[2] = 1.0f;
    ccv.float32[3] = 1.0f;
    VkImageSubresourceRange isr = {};
    isr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    isr.baseArrayLayer = 0;
    isr.baseMipLevel = 0;
    isr.layerCount = 1;
    isr.levelCount = 1;
    vkCmdClearColorImage(m_commandBuffer->GetBufferHandle(), image, VK_IMAGE_LAYOUT_GENERAL, &ccv, 1, &isr);
    m_commandBuffer->EndCommandBuffer();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound image ");
    // Destroy image dependency prior to submit to cause ERROR
    vkDestroyImage(m_device->device(), image, NULL);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->VerifyFound();
    vkFreeMemory(m_device->device(), image_mem, nullptr);
}

TEST_F(VkLayerTest, InvalidCmdBufferFramebufferImageDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to a framebuffer image dependency being destroyed.");
    VkFormatProperties format_properties;
    VkResult err = VK_SUCCESS;
    vkGetPhysicalDeviceFormatProperties(gpu(), VK_FORMAT_B8G8R8A8_UNORM, &format_properties);
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.pNext = NULL;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_ci.extent.width = 32;
    image_ci.extent.height = 32;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_ci.flags = 0;
    VkImage image;
    ASSERT_VK_SUCCESS(vkCreateImage(m_device->handle(), &image_ci, NULL, &image));

    VkMemoryRequirements memory_reqs;
    VkDeviceMemory image_memory;
    bool pass;
    VkMemoryAllocateInfo memory_info = {};
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.pNext = NULL;
    memory_info.allocationSize = 0;
    memory_info.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &memory_reqs);
    memory_info.allocationSize = memory_reqs.size;
    pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &image_memory);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, image_memory, 0);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo ivci = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_B8G8R8A8_UNORM,
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    VkImageView view;
    err = vkCreateImageView(m_device->device(), &ivci, nullptr, &view);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, m_renderPass, 1, &view, 32, 32, 1};
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    // Just use default renderpass with our framebuffer
    m_renderPassBeginInfo.framebuffer = fb;
    // Create Null cmd buffer for submit
    BeginCommandBuffer();
    EndCommandBuffer();
    // Destroy image attached to framebuffer to invalidate cmd buffer
    vkDestroyImage(m_device->device(), image, NULL);
    // Now attempt to submit cmd buffer and verify error
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound image ");
    QueueCommandBuffer(false);
    m_errorMonitor->VerifyFound();

    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyImageView(m_device->device(), view, nullptr);
    vkFreeMemory(m_device->device(), image_memory, nullptr);
}

TEST_F(VkLayerTest, FramebufferInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete in-use framebuffer.");
    VkFormatProperties format_properties;
    VkResult err = VK_SUCCESS;
    vkGetPhysicalDeviceFormatProperties(gpu(), VK_FORMAT_B8G8R8A8_UNORM, &format_properties);

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkImageObj image(m_device);
    image.init(256, 256, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());
    VkImageView view = image.targetView(VK_FORMAT_B8G8R8A8_UNORM);

    VkFramebufferCreateInfo fci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, m_renderPass, 1, &view, 256, 256, 1};
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    // Just use default renderpass with our framebuffer
    m_renderPassBeginInfo.framebuffer = fb;
    // Create Null cmd buffer for submit
    BeginCommandBuffer();
    EndCommandBuffer();
    // Submit cmd buffer to put it in-flight
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    // Destroy framebuffer while in-flight
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete framebuffer 0x");
    vkDestroyFramebuffer(m_device->device(), fb, NULL);
    m_errorMonitor->VerifyFound();
    // Wait for queue to complete so we can safely destroy everything
    vkQueueWaitIdle(m_device->m_queue);
    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
}

TEST_F(VkLayerTest, FramebufferImageInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete in-use image that's child of framebuffer.");
    VkFormatProperties format_properties;
    VkResult err = VK_SUCCESS;
    vkGetPhysicalDeviceFormatProperties(gpu(), VK_FORMAT_B8G8R8A8_UNORM, &format_properties);

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.pNext = NULL;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_ci.extent.width = 256;
    image_ci.extent.height = 256;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_ci.flags = 0;
    VkImage image;
    ASSERT_VK_SUCCESS(vkCreateImage(m_device->handle(), &image_ci, NULL, &image));

    VkMemoryRequirements memory_reqs;
    VkDeviceMemory image_memory;
    bool pass;
    VkMemoryAllocateInfo memory_info = {};
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.pNext = NULL;
    memory_info.allocationSize = 0;
    memory_info.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &memory_reqs);
    memory_info.allocationSize = memory_reqs.size;
    pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &image_memory);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, image_memory, 0);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo ivci = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_B8G8R8A8_UNORM,
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    VkImageView view;
    err = vkCreateImageView(m_device->device(), &ivci, nullptr, &view);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, m_renderPass, 1, &view, 256, 256, 1};
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    // Just use default renderpass with our framebuffer
    m_renderPassBeginInfo.framebuffer = fb;
    // Create Null cmd buffer for submit
    BeginCommandBuffer();
    EndCommandBuffer();
    // Submit cmd buffer to put it (and attached imageView) in-flight
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    // Submit cmd buffer to put framebuffer and children in-flight
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    // Destroy image attached to framebuffer while in-flight
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete image 0x");
    vkDestroyImage(m_device->device(), image, NULL);
    m_errorMonitor->VerifyFound();
    // Wait for queue to complete so we can safely destroy image and other objects
    vkQueueWaitIdle(m_device->m_queue);
    vkDestroyImage(m_device->device(), image, NULL);
    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyImageView(m_device->device(), view, nullptr);
    vkFreeMemory(m_device->device(), image_memory, nullptr);
}

TEST_F(VkLayerTest, ImageMemoryNotBound) {
    TEST_DESCRIPTION("Attempt to draw with an image which has not had memory bound to it.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImage image;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_create_info.flags = 0;
    VkResult err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);
    // Have to bind memory to image before recording cmd in cmd buffer using it
    VkMemoryRequirements mem_reqs;
    VkDeviceMemory image_mem;
    bool pass;
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &image_mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce error, do not call vkBindImageMemory(m_device->device(), image, image_mem, 0);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " used with no memory bound. Memory should be bound by calling vkBindImageMemory().");

    m_commandBuffer->BeginCommandBuffer();
    VkClearColorValue ccv;
    ccv.float32[0] = 1.0f;
    ccv.float32[1] = 1.0f;
    ccv.float32[2] = 1.0f;
    ccv.float32[3] = 1.0f;
    VkImageSubresourceRange isr = {};
    isr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    isr.baseArrayLayer = 0;
    isr.baseMipLevel = 0;
    isr.layerCount = 1;
    isr.levelCount = 1;
    vkCmdClearColorImage(m_commandBuffer->GetBufferHandle(), image, VK_IMAGE_LAYOUT_GENERAL, &ccv, 1, &isr);
    m_commandBuffer->EndCommandBuffer();

    m_errorMonitor->VerifyFound();
    vkDestroyImage(m_device->device(), image, NULL);
    vkFreeMemory(m_device->device(), image_mem, nullptr);
}

TEST_F(VkLayerTest, BufferMemoryNotBound) {
    TEST_DESCRIPTION("Attempt to copy from a buffer which has not had memory bound to it.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkBuffer buffer;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buf_info.size = 256;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult err = vkCreateBuffer(m_device->device(), &buf_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    vkGetBufferMemoryRequirements(m_device->device(), buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = 256;
    bool pass = false;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }
    err = vkAllocateMemory(m_device->device(), &alloc_info, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce failure by not calling vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " used with no memory bound. Memory should be bound by calling vkBindBufferMemory().");
    VkBufferImageCopy region = {};
    region.bufferRowLength = 128;
    region.bufferImageHeight = 128;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    region.imageSubresource.layerCount = 1;
    region.imageExtent.height = 4;
    region.imageExtent.width = 4;
    region.imageExtent.depth = 1;
    m_commandBuffer->BeginCommandBuffer();
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);
    m_commandBuffer->EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkFreeMemory(m_device->handle(), mem, NULL);
}

TEST_F(VkLayerTest, InvalidCmdBufferEventDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to an event dependency being destroyed.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkEvent event;
    VkEventCreateInfo evci = {};
    evci.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    VkResult result = vkCreateEvent(m_device->device(), &evci, NULL, &event);
    ASSERT_VK_SUCCESS(result);

    m_commandBuffer->BeginCommandBuffer();
    vkCmdSetEvent(m_commandBuffer->GetBufferHandle(), event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    m_commandBuffer->EndCommandBuffer();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound event ");
    // Destroy event dependency prior to submit to cause ERROR
    vkDestroyEvent(m_device->device(), event, NULL);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidCmdBufferQueryPoolDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to a query pool dependency being destroyed.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkQueryPool query_pool;
    VkQueryPoolCreateInfo qpci{};
    qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    qpci.queryType = VK_QUERY_TYPE_TIMESTAMP;
    qpci.queryCount = 1;
    VkResult result = vkCreateQueryPool(m_device->device(), &qpci, nullptr, &query_pool);
    ASSERT_VK_SUCCESS(result);

    m_commandBuffer->BeginCommandBuffer();
    vkCmdResetQueryPool(m_commandBuffer->GetBufferHandle(), query_pool, 0, 1);
    m_commandBuffer->EndCommandBuffer();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound query pool ");
    // Destroy query pool dependency prior to submit to cause ERROR
    vkDestroyQueryPool(m_device->device(), query_pool, NULL);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidCmdBufferPipelineDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to a pipeline dependency being destroyed.");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkResult err;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineViewportStateCreateInfo vp_state_ci = {};
    vp_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_ci.viewportCount = 1;
    VkViewport vp = {}; // Just need dummy vp to point to
    vp_state_ci.pViewports = &vp;
    vp_state_ci.scissorCount = 1;
    VkRect2D scissors = {}; // Dummy scissors to point to
    vp_state_ci.pScissors = &scissors;

    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    shaderStages[0] = vs.GetStageCreateInfo();
    shaderStages[1] = fs.GetStageCreateInfo();

    VkPipelineVertexInputStateCreateInfo vi_ci = {};
    vi_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia_ci = {};
    ia_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.rasterizerDiscardEnable = true;
    rs_ci.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState att = {};
    att.blendEnable = VK_FALSE;
    att.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo cb_ci = {};
    cb_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_ci.attachmentCount = 1;
    cb_ci.pAttachments = &att;

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.stageCount = 2;
    gp_ci.pStages = shaderStages;
    gp_ci.pVertexInputState = &vi_ci;
    gp_ci.pInputAssemblyState = &ia_ci;
    gp_ci.pViewportState = &vp_state_ci;
    gp_ci.pRasterizationState = &rs_ci;
    gp_ci.pColorBlendState = &cb_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;
    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);

    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);
    ASSERT_VK_SUCCESS(err);

    m_commandBuffer->BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    m_commandBuffer->EndCommandBuffer();
    // Now destroy pipeline in order to cause error when submitting
    vkDestroyPipeline(m_device->device(), pipeline, nullptr);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound pipeline ");

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->VerifyFound();
    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
}

TEST_F(VkLayerTest, InvalidCmdBufferDescriptorSetBufferDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to a bound descriptor set with a buffer dependency "
                     "being destroyed.");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    // Create a buffer to update the descriptor with
    uint32_t qfi = 0;
    VkBufferCreateInfo buffCI = {};
    buffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffCI.size = 1024;
    buffCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffCI.queueFamilyIndexCount = 1;
    buffCI.pQueueFamilyIndices = &qfi;

    VkBuffer buffer;
    err = vkCreateBuffer(m_device->device(), &buffCI, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);
    // Allocate memory and bind to buffer so we can make it to the appropriate
    // error
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 1024;
    mem_alloc.memoryTypeIndex = 0;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device->device(), buffer, &memReqs);
    bool pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &mem_alloc, 0);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }

    VkDeviceMemory mem;
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);
    // Correctly update descriptor to avoid "NOT_UPDATED" error
    VkDescriptorBufferInfo buffInfo = {};
    buffInfo.buffer = buffer;
    buffInfo.offset = 0;
    buffInfo.range = 1024;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.pBufferInfo = &buffInfo;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    // Create PSO to be used for draw-time errors below
    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 x;\n"
                           "layout(set=0) layout(binding=0) uniform foo { int x; int y; } bar;\n"
                           "void main(){\n"
                           "   x = vec4(bar.y);\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 0, NULL);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound buffer ");
    // Destroy buffer should invalidate the cmd buffer, causing error on submit
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    // Attempt to submit cmd buffer
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();
    // Cleanup
    vkFreeMemory(m_device->device(), mem, NULL);

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidCmdBufferDescriptorSetImageSamplerDestroyed) {
    TEST_DESCRIPTION("Attempt to draw with a command buffer that is invalid "
                     "due to a bound descriptor sets with a combined image "
                     "sampler having their image, sampler, and descriptor set "
                     "each respectively destroyed and then attempting to "
                     "submit associated cmd buffers. Attempt to destroy a "
                     "DescriptorSet that is in use.");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    // Create images to update the descriptor with
    VkImage image;
    VkImage image2;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image2);
    ASSERT_VK_SUCCESS(err);

    VkMemoryRequirements memory_reqs;
    VkDeviceMemory image_memory;
    bool pass;
    VkMemoryAllocateInfo memory_info = {};
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.pNext = NULL;
    memory_info.allocationSize = 0;
    memory_info.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &memory_reqs);
    // Allocate enough memory for both images
    memory_info.allocationSize = memory_reqs.size * 2;
    pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &image_memory);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, image_memory, 0);
    ASSERT_VK_SUCCESS(err);
    // Bind second image to memory right after first image
    err = vkBindImageMemory(m_device->device(), image2, image_memory, memory_reqs.size);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = tex_format;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageView view;
    VkImageView view2;
    err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);
    ASSERT_VK_SUCCESS(err);
    image_view_create_info.image = image2;
    err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view2);
    ASSERT_VK_SUCCESS(err);
    // Create Samplers
    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;
    VkSampler sampler2;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler2);
    ASSERT_VK_SUCCESS(err);
    // Update descriptor with image and sampler
    VkDescriptorImageInfo img_info = {};
    img_info.sampler = sampler;
    img_info.imageView = view;
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo = &img_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    // Create PSO to be used for draw-time errors below
    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0) uniform sampler2D s;\n"
                           "layout(location=0) out vec4 x;\n"
                           "void main(){\n"
                           "   x = texture(s, vec2(1));\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    // First error case is destroying sampler prior to cmd buffer submission
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot submit cmd buffer using deleted sampler ");
    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 0, NULL);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();
    // Destroy sampler invalidates the cmd buffer, causing error on submit
    vkDestroySampler(m_device->device(), sampler, NULL);
    // Attempt to submit cmd buffer
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();
    // Now re-update descriptor with valid sampler and delete image
    img_info.sampler = sampler2;
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound image ");
    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 0, NULL);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();
    // Destroy image invalidates the cmd buffer, causing error on submit
    vkDestroyImage(m_device->device(), image, NULL);
    // Attempt to submit cmd buffer
    submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();
    // Now update descriptor to be valid, but then free descriptor
    img_info.imageView = view2;
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " that is invalid because bound descriptor set ");
    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 0, NULL);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();
    // Destroy descriptor set invalidates the cb, causing error on submit
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot call vkFreeDescriptorSets() on descriptor set 0x");
    vkFreeDescriptorSets(m_device->device(), ds_pool, 1, &descriptorSet);
    m_errorMonitor->VerifyFound();
    // Attempt to submit cmd buffer
    submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();
    // Cleanup
    vkFreeMemory(m_device->device(), image_memory, NULL);
    vkDestroySampler(m_device->device(), sampler2, NULL);
    vkDestroyImage(m_device->device(), image2, NULL);
    vkDestroyImageView(m_device->device(), view, NULL);
    vkDestroyImageView(m_device->device(), view2, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, DescriptorPoolInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete a DescriptorPool with a DescriptorSet that is in use.");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptor_set;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    // Create image to update the descriptor with
    VkImageObj image(m_device);
    image.init(32, 32, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageView view = image.targetView(VK_FORMAT_B8G8R8A8_UNORM);
    // Create Sampler
    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);
    // Update descriptor with image and sampler
    VkDescriptorImageInfo img_info = {};
    img_info.sampler = sampler;
    img_info.imageView = view;
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo = &img_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    // Create PSO to be used for draw-time errors below
    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0) uniform sampler2D s;\n"
                           "layout(location=0) out vec4 x;\n"
                           "void main(){\n"
                           "   x = texture(s, vec2(1));\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptor_set, 0, NULL);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();
    // Submit cmd buffer to put pool in-flight
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    // Destroy pool while in-flight, causing error
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete descriptor pool ");
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
    m_errorMonitor->VerifyFound();
    vkQueueWaitIdle(m_device->m_queue);
    // Cleanup
    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, DescriptorImageUpdateNoMemoryBound) {
    TEST_DESCRIPTION("Attempt an image descriptor set update where image's bound memory has been freed.");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    // Create images to update the descriptor with
    VkImage image;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);
    // Initially bind memory to avoid error at bind view time. We'll break binding before update.
    VkMemoryRequirements memory_reqs;
    VkDeviceMemory image_memory;
    bool pass;
    VkMemoryAllocateInfo memory_info = {};
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.pNext = NULL;
    memory_info.allocationSize = 0;
    memory_info.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &memory_reqs);
    // Allocate enough memory for image
    memory_info.allocationSize = memory_reqs.size;
    pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &image_memory);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, image_memory, 0);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = tex_format;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageView view;
    err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);
    ASSERT_VK_SUCCESS(err);
    // Create Samplers
    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);
    // Update descriptor with image and sampler
    VkDescriptorImageInfo img_info = {};
    img_info.sampler = sampler;
    img_info.imageView = view;
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo = &img_info;
    // Break memory binding and attempt update
    vkFreeMemory(m_device->device(), image_memory, nullptr);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " previously bound memory was freed. Memory must not be freed prior to this operation.");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkUpdateDescriptorsSets() failed write update validation for Descriptor Set 0x");
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->VerifyFound();
    // Cleanup
    vkDestroyImage(m_device->device(), image, NULL);
    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyImageView(m_device->device(), view, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidPipeline) {
    // Attempt to bind an invalid Pipeline to a valid Command Buffer
    // ObjectTracker should catch this.
    // Create a valid cmd buffer
    // call vkCmdBindPipeline w/ false Pipeline
    uint64_t fake_pipeline_handle = 0xbaad6001;
    VkPipeline bad_pipeline = reinterpret_cast<VkPipeline &>(fake_pipeline_handle);
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Pipeline Object 0xbaad6001");
    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, bad_pipeline);
    m_errorMonitor->VerifyFound();

    // Now issue a draw call with no pipeline bound
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "At Draw/Dispatch time no valid VkPipeline is bound!");
    Draw(1, 0, 0, 0);
    m_errorMonitor->VerifyFound();

    // Finally same check once more but with Dispatch/Compute
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "At Draw/Dispatch time no valid VkPipeline is bound!");
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle()); // must be outside renderpass
    vkCmdDispatch(m_commandBuffer->GetBufferHandle(), 0, 0, 0);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DescriptorSetNotUpdated) {
    TEST_DESCRIPTION("Bind a descriptor set that hasn't been updated.");
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, " bound but it was never updated. ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    //  We shouldn't need a fragment shader but add it to be able to run
    //  on more devices
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidBufferViewObject) {
    // Create a single TEXEL_BUFFER descriptor and send it an invalid bufferView
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attempted write update to texel buffer "
                                                                        "descriptor with invalid buffer view");

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkBufferView view = (VkBufferView)((size_t)0xbaadbeef); // invalid bufferView object
    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    descriptor_write.pTexelBufferView = &view;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, CreateBufferViewNoMemoryBoundToBuffer) {
    TEST_DESCRIPTION("Attempt to create a buffer view with a buffer that has no memory bound to it.");

    VkResult err;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " used with no memory bound. Memory should be bound by calling vkBindBufferMemory().");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create a buffer with no bound memory and then attempt to create
    // a buffer view.
    VkBufferCreateInfo buff_ci = {};
    buff_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_ci.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    buff_ci.size = 256;
    buff_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer;
    err = vkCreateBuffer(m_device->device(), &buff_ci, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    VkBufferViewCreateInfo buff_view_ci = {};
    buff_view_ci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    buff_view_ci.buffer = buffer;
    buff_view_ci.format = VK_FORMAT_R8_UNORM;
    buff_view_ci.range = VK_WHOLE_SIZE;
    VkBufferView buff_view;
    err = vkCreateBufferView(m_device->device(), &buff_view_ci, NULL, &buff_view);

    m_errorMonitor->VerifyFound();
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    // If last error is success, it still created the view, so delete it.
    if (err == VK_SUCCESS) {
        vkDestroyBufferView(m_device->device(), buff_view, NULL);
    }
}

TEST_F(VkLayerTest, InvalidDynamicOffsetCases) {
    // Create a descriptorSet w/ dynamic descriptor and then hit 3 offset error
    // cases:
    // 1. No dynamicOffset supplied
    // 2. Too many dynamicOffsets supplied
    // 3. Dynamic offset oversteps buffer being updated
    VkResult err;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " requires 1 dynamicOffsets, but only "
                                                                        "0 dynamicOffsets are left in "
                                                                        "pDynamicOffsets ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    // Create a buffer to update the descriptor with
    uint32_t qfi = 0;
    VkBufferCreateInfo buffCI = {};
    buffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffCI.size = 1024;
    buffCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffCI.queueFamilyIndexCount = 1;
    buffCI.pQueueFamilyIndices = &qfi;

    VkBuffer dyub;
    err = vkCreateBuffer(m_device->device(), &buffCI, NULL, &dyub);
    ASSERT_VK_SUCCESS(err);
    // Allocate memory and bind to buffer so we can make it to the appropriate
    // error
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 1024;
    mem_alloc.memoryTypeIndex = 0;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device->device(), dyub, &memReqs);
    bool pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &mem_alloc, 0);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), dyub, NULL);
        return;
    }

    VkDeviceMemory mem;
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), dyub, mem, 0);
    ASSERT_VK_SUCCESS(err);
    // Correctly update descriptor to avoid "NOT_UPDATED" error
    VkDescriptorBufferInfo buffInfo = {};
    buffInfo.buffer = dyub;
    buffInfo.offset = 0;
    buffInfo.range = 1024;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptor_write.pBufferInfo = &buffInfo;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    BeginCommandBuffer();
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 0, NULL);
    m_errorMonitor->VerifyFound();
    uint32_t pDynOff[2] = {512, 756};
    // Now cause error b/c too many dynOffsets in array for # of dyn descriptors
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Attempting to bind 1 descriptorSets with 1 dynamic descriptors, but ");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 2, pDynOff);
    m_errorMonitor->VerifyFound();
    // Finally cause error due to dynamicOffset being too big
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " dynamic offset 512 combined with "
                                                                        "offset 0 and range 1024 that "
                                                                        "oversteps the buffer size of 1024");
    // Create PSO to be used for draw-time errors below
    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 x;\n"
                           "layout(set=0) layout(binding=0) uniform foo { int x; int y; } bar;\n"
                           "void main(){\n"
                           "   x = vec4(bar.y);\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    // This update should succeed, but offset size of 512 will overstep buffer
    // /w range 1024 & size 1024
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorSet, 1, pDynOff);
    Draw(1, 0, 0, 0);
    m_errorMonitor->VerifyFound();

    vkDestroyBuffer(m_device->device(), dyub, NULL);
    vkFreeMemory(m_device->device(), mem, NULL);

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, DescriptorBufferUpdateNoMemoryBound) {
    TEST_DESCRIPTION("Attempt to update a descriptor with a non-sparse buffer "
                     "that doesn't have memory bound");
    VkResult err;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " used with no memory bound. Memory should be bound by calling vkBindBufferMemory().");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkUpdateDescriptorsSets() failed write update validation for Descriptor Set 0x");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    // Create a buffer to update the descriptor with
    uint32_t qfi = 0;
    VkBufferCreateInfo buffCI = {};
    buffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffCI.size = 1024;
    buffCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffCI.queueFamilyIndexCount = 1;
    buffCI.pQueueFamilyIndices = &qfi;

    VkBuffer dyub;
    err = vkCreateBuffer(m_device->device(), &buffCI, NULL, &dyub);
    ASSERT_VK_SUCCESS(err);

    // Attempt to update descriptor without binding memory to it
    VkDescriptorBufferInfo buffInfo = {};
    buffInfo.buffer = dyub;
    buffInfo.offset = 0;
    buffInfo.range = 1024;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptor_write.pBufferInfo = &buffInfo;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->VerifyFound();

    vkDestroyBuffer(m_device->device(), dyub, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidPushConstants) {
    VkResult err;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkPipelineLayout pipeline_layout;
    VkPushConstantRange pc_range = {};
    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pushConstantRangeCount = 1;
    pipeline_layout_ci.pPushConstantRanges = &pc_range;

    //
    // Check for invalid push constant ranges in pipeline layouts.
    //
    struct PipelineLayoutTestCase {
        VkPushConstantRange const range;
        char const *msg;
    };

    const uint32_t too_big = m_device->props.limits.maxPushConstantsSize + 0x4;
    const std::array<PipelineLayoutTestCase, 10> range_tests = {{
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, 0},
         "vkCreatePipelineLayout() call has push constants index 0 with "
         "size 0."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, 1},
         "vkCreatePipelineLayout() call has push constants index 0 with "
         "size 1."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 4, 1},
         "vkCreatePipelineLayout() call has push constants index 0 with "
         "size 1."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 4, 0},
         "vkCreatePipelineLayout() call has push constants index 0 with "
         "size 0."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 1, 4},
         "vkCreatePipelineLayout() call has push constants index 0 with "
         "offset 1. Offset must"},
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, too_big},
         "vkCreatePipelineLayout() call has push constants index 0 "
         "with offset "},
        {{VK_SHADER_STAGE_VERTEX_BIT, too_big, too_big},
         "vkCreatePipelineLayout() call has push constants "
         "index 0 with offset "},
        {{VK_SHADER_STAGE_VERTEX_BIT, too_big, 4},
         "vkCreatePipelineLayout() call has push constants index 0 "
         "with offset "},
        {{VK_SHADER_STAGE_VERTEX_BIT, 0xFFFFFFF0, 0x00000020},
         "vkCreatePipelineLayout() call has push "
         "constants index 0 with offset "},
        {{VK_SHADER_STAGE_VERTEX_BIT, 0x00000020, 0xFFFFFFF0},
         "vkCreatePipelineLayout() call has push "
         "constants index 0 with offset "},
    }};

    // Check for invalid offset and size
    for (const auto &iter : range_tests) {
        pc_range = iter.range;
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, iter.msg);
        err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
        m_errorMonitor->VerifyFound();
        if (VK_SUCCESS == err) {
            vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
        }
    }

    // Check for invalid stage flag
    pc_range.offset = 0;
    pc_range.size = 16;
    pc_range.stageFlags = 0;
    m_errorMonitor->SetDesiredFailureMsg(
        VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "vkCreatePipelineLayout: value of pCreateInfo->pPushConstantRanges[0].stageFlags must not be 0");
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    m_errorMonitor->VerifyFound();
    if (VK_SUCCESS == err) {
        vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    }

    // Check for overlapping ranges
    const uint32_t ranges_per_test = 5;
    struct OverlappingRangeTestCase {
        VkPushConstantRange const ranges[ranges_per_test];
        char const *msg;
    };

    const std::array<OverlappingRangeTestCase, 5> overlapping_range_tests = {
        {{{{VK_SHADER_STAGE_VERTEX_BIT, 0, 4},
           {VK_SHADER_STAGE_VERTEX_BIT, 0, 4},
           {VK_SHADER_STAGE_VERTEX_BIT, 0, 4},
           {VK_SHADER_STAGE_VERTEX_BIT, 0, 4},
           {VK_SHADER_STAGE_VERTEX_BIT, 0, 4}},
          "vkCreatePipelineLayout() call has push constants with overlapping ranges:"},
         {
             {{VK_SHADER_STAGE_VERTEX_BIT, 0, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 4, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 8, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 12, 8},
              {VK_SHADER_STAGE_VERTEX_BIT, 16, 4}},
             "vkCreatePipelineLayout() call has push constants with overlapping ranges: 3:[12, 20), 4:[16, 20)",
         },
         {
             {{VK_SHADER_STAGE_VERTEX_BIT, 16, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 12, 8},
              {VK_SHADER_STAGE_VERTEX_BIT, 8, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 4, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 0, 4}},
             "vkCreatePipelineLayout() call has push constants with overlapping ranges: 0:[16, 20), 1:[12, 20)",
         },
         {
             {{VK_SHADER_STAGE_VERTEX_BIT, 16, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 8, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 4, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 12, 8},
              {VK_SHADER_STAGE_VERTEX_BIT, 0, 4}},
             "vkCreatePipelineLayout() call has push constants with overlapping ranges: 0:[16, 20), 3:[12, 20)",
         },
         {
             {{VK_SHADER_STAGE_VERTEX_BIT, 16, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 32, 4},
              {VK_SHADER_STAGE_VERTEX_BIT, 4, 96},
              {VK_SHADER_STAGE_VERTEX_BIT, 40, 8},
              {VK_SHADER_STAGE_VERTEX_BIT, 52, 4}},
             "vkCreatePipelineLayout() call has push constants with overlapping ranges:",
         }}};

    for (const auto &iter : overlapping_range_tests) {
        pipeline_layout_ci.pPushConstantRanges = iter.ranges;
        pipeline_layout_ci.pushConstantRangeCount = ranges_per_test;
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, iter.msg);
        err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
        m_errorMonitor->VerifyFound();
        if (VK_SUCCESS == err) {
            vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
        }
    }

    //
    // CmdPushConstants tests
    //
    const uint8_t dummy_values[100] = {};

    // Check for invalid offset and size and if range is within layout range(s)
    const std::array<PipelineLayoutTestCase, 11> cmd_range_tests = {{
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, 0}, "vkCmdPushConstants: parameter size must be greater than 0"},
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, 1},
         "vkCmdPushConstants() call has push constants with size 1. Size "
         "must be greater than zero and a multiple of 4."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 4, 1},
         "vkCmdPushConstants() call has push constants with size 1. Size "
         "must be greater than zero and a multiple of 4."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 1, 4},
         "vkCmdPushConstants() call has push constants with offset 1. "
         "Offset must be a multiple of 4."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 1, 4},
         "vkCmdPushConstants() call has push constants with offset 1. "
         "Offset must be a multiple of 4."},
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, 20},
         "vkCmdPushConstants() Push constant range [0, 20) with stageFlags = "
         "0x1 not within flag-matching ranges in pipeline layout"},
        {{VK_SHADER_STAGE_VERTEX_BIT, 60, 8},
         "vkCmdPushConstants() Push constant range [60, 68) with stageFlags = "
         "0x1 not within flag-matching ranges in pipeline layout"},
        {{VK_SHADER_STAGE_VERTEX_BIT, 76, 8},
         "vkCmdPushConstants() Push constant range [76, 84) with stageFlags = "
         "0x1 not within flag-matching ranges in pipeline layout"},
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, 80},
         "vkCmdPushConstants() Push constant range [0, 80) with stageFlags = "
         "0x1 not within flag-matching ranges in pipeline layout"},
        {{VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, 4},
         "vkCmdPushConstants() stageFlags = 0x2 do not match the stageFlags in "
         "any of the ranges in pipeline layout"},
        {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, 16},
         "vkCmdPushConstants() stageFlags = 0x3 do not match the stageFlags in "
         "any of the ranges in pipeline layout"},
    }};

    BeginCommandBuffer();

    // Setup ranges: [0,16) [64,80)
    const VkPushConstantRange pc_range2[] = {
        {VK_SHADER_STAGE_VERTEX_BIT, 64, 16}, {VK_SHADER_STAGE_VERTEX_BIT, 0, 16},
    };
    pipeline_layout_ci.pushConstantRangeCount = sizeof(pc_range2) / sizeof(VkPushConstantRange);
    pipeline_layout_ci.pPushConstantRanges = pc_range2;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);
    for (const auto &iter : cmd_range_tests) {
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, iter.msg);
        vkCmdPushConstants(m_commandBuffer->GetBufferHandle(), pipeline_layout, iter.range.stageFlags, iter.range.offset,
                           iter.range.size, dummy_values);
        m_errorMonitor->VerifyFound();
    }

    // Check for invalid stage flag
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdPushConstants: value of stageFlags must not be 0");
    vkCmdPushConstants(m_commandBuffer->GetBufferHandle(), pipeline_layout, 0, 0, 16, dummy_values);
    m_errorMonitor->VerifyFound();
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);

    // overlapping range tests with cmd
    const std::array<PipelineLayoutTestCase, 3> cmd_overlap_tests = {{
        {{VK_SHADER_STAGE_VERTEX_BIT, 0, 20},
         "vkCmdPushConstants() Push constant range [0, 20) with stageFlags = "
         "0x1 not within flag-matching ranges in pipeline layout"},
        {{VK_SHADER_STAGE_VERTEX_BIT, 16, 4},
         "vkCmdPushConstants() Push constant range [16, 20) with stageFlags = "
         "0x1 not within flag-matching ranges in pipeline layout"},
        {{VK_SHADER_STAGE_VERTEX_BIT, 40, 16},
         "vkCmdPushConstants() Push constant range [40, 56) with stageFlags = "
         "0x1 not within flag-matching ranges in pipeline layout"},
    }};
    // Setup ranges:  [0,16), [20,36), [36,44), [44,52), [80,92)
    const VkPushConstantRange pc_range3[] = {
        {VK_SHADER_STAGE_VERTEX_BIT, 20, 16}, {VK_SHADER_STAGE_VERTEX_BIT, 0, 16}, {VK_SHADER_STAGE_VERTEX_BIT, 44, 8},
        {VK_SHADER_STAGE_VERTEX_BIT, 80, 12}, {VK_SHADER_STAGE_VERTEX_BIT, 36, 8},
    };
    pipeline_layout_ci.pushConstantRangeCount = sizeof(pc_range3) / sizeof(VkPushConstantRange);
    pipeline_layout_ci.pPushConstantRanges = pc_range3;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);
    for (const auto &iter : cmd_overlap_tests) {
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, iter.msg);
        vkCmdPushConstants(m_commandBuffer->GetBufferHandle(), pipeline_layout, iter.range.stageFlags, iter.range.offset,
                           iter.range.size, dummy_values);
        m_errorMonitor->VerifyFound();
    }
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);

    EndCommandBuffer();
}

TEST_F(VkLayerTest, DescriptorSetCompatibility) {
    // Test various desriptorSet errors with bad binding combinations
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageTiling tiling;
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(gpu(), tex_format, &format_properties);
    if (format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
        tiling = VK_IMAGE_TILING_LINEAR;
    } else if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
        tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        printf("Device does not support VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT; "
               "skipped.\n");
        return;
    }

    static const uint32_t NUM_DESCRIPTOR_TYPES = 5;
    VkDescriptorPoolSize ds_type_count[NUM_DESCRIPTOR_TYPES] = {};
    ds_type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count[0].descriptorCount = 10;
    ds_type_count[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    ds_type_count[1].descriptorCount = 2;
    ds_type_count[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    ds_type_count[2].descriptorCount = 2;
    ds_type_count[3].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    ds_type_count[3].descriptorCount = 5;
    // TODO : LunarG ILO driver currently asserts in desc.c w/ INPUT_ATTACHMENT
    // type
    // ds_type_count[4].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    ds_type_count[4].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    ds_type_count[4].descriptorCount = 2;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 5;
    ds_pool_ci.poolSizeCount = NUM_DESCRIPTOR_TYPES;
    ds_pool_ci.pPoolSizes = ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    static const uint32_t MAX_DS_TYPES_IN_LAYOUT = 2;
    VkDescriptorSetLayoutBinding dsl_binding[MAX_DS_TYPES_IN_LAYOUT] = {};
    dsl_binding[0].binding = 0;
    dsl_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding[0].descriptorCount = 5;
    dsl_binding[0].stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding[0].pImmutableSamplers = NULL;

    // Create layout identical to set0 layout but w/ different stageFlags
    VkDescriptorSetLayoutBinding dsl_fs_stage_only = {};
    dsl_fs_stage_only.binding = 0;
    dsl_fs_stage_only.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_fs_stage_only.descriptorCount = 5;
    dsl_fs_stage_only.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Different stageFlags to cause error at
                                                                 // bind time
    dsl_fs_stage_only.pImmutableSamplers = NULL;
    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = dsl_binding;
    static const uint32_t NUM_LAYOUTS = 4;
    VkDescriptorSetLayout ds_layout[NUM_LAYOUTS] = {};
    VkDescriptorSetLayout ds_layout_fs_only = {};
    // Create 4 unique layouts for full pipelineLayout, and 1 special fs-only
    // layout for error case
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout[0]);
    ASSERT_VK_SUCCESS(err);
    ds_layout_ci.pBindings = &dsl_fs_stage_only;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout_fs_only);
    ASSERT_VK_SUCCESS(err);
    dsl_binding[0].binding = 0;
    dsl_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    dsl_binding[0].descriptorCount = 2;
    dsl_binding[1].binding = 1;
    dsl_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    dsl_binding[1].descriptorCount = 2;
    dsl_binding[1].stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding[1].pImmutableSamplers = NULL;
    ds_layout_ci.pBindings = dsl_binding;
    ds_layout_ci.bindingCount = 2;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout[1]);
    ASSERT_VK_SUCCESS(err);
    dsl_binding[0].binding = 0;
    dsl_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    dsl_binding[0].descriptorCount = 5;
    ds_layout_ci.bindingCount = 1;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout[2]);
    ASSERT_VK_SUCCESS(err);
    dsl_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    dsl_binding[0].descriptorCount = 2;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout[3]);
    ASSERT_VK_SUCCESS(err);

    static const uint32_t NUM_SETS = 4;
    VkDescriptorSet descriptorSet[NUM_SETS] = {};
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = NUM_LAYOUTS;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, descriptorSet);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorSet ds0_fs_only = {};
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &ds_layout_fs_only;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &ds0_fs_only);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = NUM_LAYOUTS;
    pipeline_layout_ci.pSetLayouts = ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);
    // Create pipelineLayout with only one setLayout
    pipeline_layout_ci.setLayoutCount = 1;
    VkPipelineLayout single_pipe_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &single_pipe_layout);
    ASSERT_VK_SUCCESS(err);
    // Create pipelineLayout with 2 descriptor setLayout at index 0
    pipeline_layout_ci.pSetLayouts = &ds_layout[3];
    VkPipelineLayout pipe_layout_one_desc;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipe_layout_one_desc);
    ASSERT_VK_SUCCESS(err);
    // Create pipelineLayout with 5 SAMPLER descriptor setLayout at index 0
    pipeline_layout_ci.pSetLayouts = &ds_layout[2];
    VkPipelineLayout pipe_layout_five_samp;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipe_layout_five_samp);
    ASSERT_VK_SUCCESS(err);
    // Create pipelineLayout with UB type, but stageFlags for FS only
    pipeline_layout_ci.pSetLayouts = &ds_layout_fs_only;
    VkPipelineLayout pipe_layout_fs_only;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipe_layout_fs_only);
    ASSERT_VK_SUCCESS(err);
    // Create pipelineLayout w/ incompatible set0 layout, but set1 is fine
    VkDescriptorSetLayout pl_bad_s0[2] = {};
    pl_bad_s0[0] = ds_layout_fs_only;
    pl_bad_s0[1] = ds_layout[1];
    pipeline_layout_ci.setLayoutCount = 2;
    pipeline_layout_ci.pSetLayouts = pl_bad_s0;
    VkPipelineLayout pipe_layout_bad_set0;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipe_layout_bad_set0);
    ASSERT_VK_SUCCESS(err);

    // Create a buffer to update the descriptor with
    uint32_t qfi = 0;
    VkBufferCreateInfo buffCI = {};
    buffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffCI.size = 1024;
    buffCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffCI.queueFamilyIndexCount = 1;
    buffCI.pQueueFamilyIndices = &qfi;

    VkBuffer dyub;
    err = vkCreateBuffer(m_device->device(), &buffCI, NULL, &dyub);
    ASSERT_VK_SUCCESS(err);
    // Correctly update descriptor to avoid "NOT_UPDATED" error
    static const uint32_t NUM_BUFFS = 5;
    VkDescriptorBufferInfo buffInfo[NUM_BUFFS] = {};
    for (uint32_t i = 0; i < NUM_BUFFS; ++i) {
        buffInfo[i].buffer = dyub;
        buffInfo[i].offset = 0;
        buffInfo[i].range = 1024;
    }
    VkImage image;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = tiling;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    image_create_info.flags = 0;
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    VkMemoryRequirements memReqs;
    VkDeviceMemory imageMem;
    bool pass;
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &imageMem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, imageMem, 0);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = tex_format;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageView view;
    err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorImageInfo imageInfo[4] = {};
    imageInfo[0].imageView = view;
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo[1].imageView = view;
    imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo[2].imageView = view;
    imageInfo[2].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo[3].imageView = view;
    imageInfo[3].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    static const uint32_t NUM_SET_UPDATES = 3;
    VkWriteDescriptorSet descriptor_write[NUM_SET_UPDATES] = {};
    descriptor_write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write[0].dstSet = descriptorSet[0];
    descriptor_write[0].dstBinding = 0;
    descriptor_write[0].descriptorCount = 5;
    descriptor_write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write[0].pBufferInfo = buffInfo;
    descriptor_write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write[1].dstSet = descriptorSet[1];
    descriptor_write[1].dstBinding = 0;
    descriptor_write[1].descriptorCount = 2;
    descriptor_write[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptor_write[1].pImageInfo = imageInfo;
    descriptor_write[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write[2].dstSet = descriptorSet[1];
    descriptor_write[2].dstBinding = 1;
    descriptor_write[2].descriptorCount = 2;
    descriptor_write[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptor_write[2].pImageInfo = &imageInfo[2];

    vkUpdateDescriptorSets(m_device->device(), 3, descriptor_write, 0, NULL);

    // Create PSO to be used for draw-time errors below
    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 x;\n"
                           "layout(set=0) layout(binding=0) uniform foo { int x; int y; } bar;\n"
                           "void main(){\n"
                           "   x = vec4(bar.y);\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipe_layout_fs_only, renderPass());

    BeginCommandBuffer();

    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    // NOTE : I believe LunarG ilo driver has bug (LX#189) that requires binding
    // of PSO
    //  here before binding DSs. Otherwise we assert in cmd_copy_dset_data() of
    //  cmd_pipeline.c
    //  due to the fact that cmd_alloc_dset_data() has not been called in
    //  cmd_bind_graphics_pipeline()
    // TODO : Want to cause various binding incompatibility issues here to test
    // DrawState
    //  First cause various verify_layout_compatibility() fails
    //  Second disturb early and late sets and verify INFO msgs
    // verify_set_layout_compatibility fail cases:
    // 1. invalid VkPipelineLayout (layout) passed into vkCmdBindDescriptorSets
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Pipeline Layout Object ");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                            (VkPipelineLayout)((size_t)0xbaadb1be), 0, 1, &descriptorSet[0], 0, NULL);
    m_errorMonitor->VerifyFound();

    // 2. layoutIndex exceeds # of layouts in layout
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " attempting to bind set to index 1");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, single_pipe_layout, 0, 2,
                            &descriptorSet[0], 0, NULL);
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), single_pipe_layout, NULL);
    // 3. Pipeline setLayout[0] has 2 descriptors, but set being bound has 5
    // descriptors
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " has 2 descriptors, but DescriptorSetLayout ");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_one_desc, 0, 1,
                            &descriptorSet[0], 0, NULL);
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipe_layout_one_desc, NULL);
    // 4. same # of descriptors but mismatch in type
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is type 'VK_DESCRIPTOR_TYPE_SAMPLER' but binding ");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_five_samp, 0, 1,
                            &descriptorSet[0], 0, NULL);
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipe_layout_five_samp, NULL);
    // 5. same # of descriptors but mismatch in stageFlags
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " has stageFlags 16 but binding 0 for DescriptorSetLayout ");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_fs_only, 0, 1,
                            &descriptorSet[0], 0, NULL);
    m_errorMonitor->VerifyFound();

    // Cause INFO messages due to disturbing previously bound Sets
    // First bind sets 0 & 1
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2,
                            &descriptorSet[0], 0, NULL);
    // 1. Disturb bound set0 by re-binding set1 w/ updated pipelineLayout
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, " previously bound as set #0 was disturbed ");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_bad_set0, 1, 1,
                            &descriptorSet[1], 0, NULL);
    m_errorMonitor->VerifyFound();

    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2,
                            &descriptorSet[0], 0, NULL);
    // 2. Disturb set after last bound set
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, " newly bound as set #0 so set #1 and "
                                                                                      "any subsequent sets were disturbed ");
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_fs_only, 0, 1,
                            &ds0_fs_only, 0, NULL);
    m_errorMonitor->VerifyFound();

    // Now that we're done actively using the pipelineLayout that gfx pipeline
    //  was created with, we should be able to delete it. Do that now to verify
    //  that validation obeys pipelineLayout lifetime
    vkDestroyPipelineLayout(m_device->device(), pipe_layout_fs_only, NULL);

    // Cause draw-time errors due to PSO incompatibilities
    // 1. Error due to not binding required set (we actually use same code as
    // above to disturb set0)
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2,
                            &descriptorSet[0], 0, NULL);
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_bad_set0, 1, 1,
                            &descriptorSet[1], 0, NULL);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " uses set #0 but that set is not bound.");
    Draw(1, 0, 0, 0);
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipe_layout_bad_set0, NULL);
    // 2. Error due to bound set not being compatible with PSO's
    // VkPipelineLayout (diff stageFlags in this case)
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2,
                            &descriptorSet[0], 0, NULL);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " bound as set #0 is not compatible with ");
    Draw(1, 0, 0, 0);
    m_errorMonitor->VerifyFound();

    // Remaining clean-up
    for (uint32_t i = 0; i < NUM_LAYOUTS; ++i) {
        vkDestroyDescriptorSetLayout(m_device->device(), ds_layout[i], NULL);
    }
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout_fs_only, NULL);
    vkDestroyBuffer(m_device->device(), dyub, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
    vkFreeMemory(m_device->device(), imageMem, NULL);
    vkDestroyImage(m_device->device(), image, NULL);
    vkDestroyImageView(m_device->device(), view, NULL);
}

TEST_F(VkLayerTest, NoBeginCommandBuffer) {

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "You must call vkBeginCommandBuffer() before this call to ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkCommandBufferObj commandBuffer(m_device, m_commandPool);
    // Call EndCommandBuffer() w/o calling BeginCommandBuffer()
    vkEndCommandBuffer(commandBuffer.GetBufferHandle());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, SecondaryCommandBufferNullRenderpass) {
    VkResult err;
    VkCommandBuffer draw_cmd;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " must specify a valid renderpass parameter.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkCommandBufferAllocateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd.pNext = NULL;
    cmd.commandPool = m_commandPool;
    cmd.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    cmd.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(m_device->device(), &cmd, &draw_cmd);
    ASSERT_VK_SUCCESS(err);

    // Force the failure by not setting the Renderpass and Framebuffer fields
    VkCommandBufferBeginInfo cmd_buf_info = {};
    VkCommandBufferInheritanceInfo cmd_buf_hinfo = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmd_buf_info.pInheritanceInfo = &cmd_buf_hinfo;

    // The error should be caught by validation of the BeginCommandBuffer call
    vkBeginCommandBuffer(draw_cmd, &cmd_buf_info);

    m_errorMonitor->VerifyFound();
    vkFreeCommandBuffers(m_device->device(), m_commandPool, 1, &draw_cmd);
}

TEST_F(VkLayerTest, CommandBufferResetErrors) {
    // Cause error due to Begin while recording CB
    // Then cause 2 errors for attempting to reset CB w/o having
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT set for the pool from
    // which CBs were allocated. Note that this bit is off by default.
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot call Begin on command buffer");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Calls AllocateCommandBuffers
    VkCommandBufferObj commandBuffer(m_device, m_commandPool);

    // Force the failure by setting the Renderpass and Framebuffer fields with
    // (fake) data
    VkCommandBufferBeginInfo cmd_buf_info = {};
    VkCommandBufferInheritanceInfo cmd_buf_hinfo = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmd_buf_info.pInheritanceInfo = &cmd_buf_hinfo;

    // Begin CB to transition to recording state
    vkBeginCommandBuffer(commandBuffer.GetBufferHandle(), &cmd_buf_info);
    // Can't re-begin. This should trigger error
    vkBeginCommandBuffer(commandBuffer.GetBufferHandle(), &cmd_buf_info);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attempt to reset command buffer ");
    VkCommandBufferResetFlags flags = 0; // Don't care about flags for this test
    // Reset attempt will trigger error due to incorrect CommandPool state
    vkResetCommandBuffer(commandBuffer.GetBufferHandle(), flags);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " attempts to implicitly reset cmdBuffer created from ");
    // Transition CB to RECORDED state
    vkEndCommandBuffer(commandBuffer.GetBufferHandle());
    // Now attempting to Begin will implicitly reset, which triggers error
    vkBeginCommandBuffer(commandBuffer.GetBufferHandle(), &cmd_buf_info);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidPipelineCreateState) {
    // Attempt to Create Gfx Pipeline w/o a VS
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid Pipeline CreateInfo State: Vertex Shader required");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkViewport vp = {}; // Just need dummy vp to point to
    VkRect2D sc = {};   // dummy scissor to point to

    VkPipelineViewportStateCreateInfo vp_state_ci = {};
    vp_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_ci.scissorCount = 1;
    vp_state_ci.pScissors = &sc;
    vp_state_ci.viewportCount = 1;
    vp_state_ci.pViewports = &vp;

    VkPipelineRasterizationStateCreateInfo rs_state_ci = {};
    rs_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_state_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rs_state_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    rs_state_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs_state_ci.depthClampEnable = VK_FALSE;
    rs_state_ci.rasterizerDiscardEnable = VK_FALSE;
    rs_state_ci.depthBiasEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.pViewportState = &vp_state_ci;
    gp_ci.pRasterizationState = &rs_state_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pc_ci.initialDataSize = 0;
    pc_ci.pInitialData = 0;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}
/*// TODO : This test should be good, but needs Tess support in compiler to run
TEST_F(VkLayerTest, InvalidPatchControlPoints)
{
    // Attempt to Create Gfx Pipeline w/o a VS
    VkResult        err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH
primitive ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.poolSizeCount = 1;
        ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(),
VK_DESCRIPTOR_POOL_USAGE_NON_FREE, 1, &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.binding = 0;
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.descriptorCount = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType =
VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.bindingCount = 1;
        ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL,
&ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocateDescriptorSets(m_device->device(), ds_pool,
VK_DESCRIPTOR_SET_USAGE_NON_FREE, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType =
VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.setLayoutCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL,
&pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineShaderStageCreateInfo shaderStages[3];
    memset(&shaderStages, 0, 3 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device,bindStateVertShaderText,VK_SHADER_STAGE_VERTEX_BIT,
this);
    // Just using VS txt for Tess shaders as we don't care about functionality
    VkShaderObj
tc(m_device,bindStateVertShaderText,VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
this);
    VkShaderObj
te(m_device,bindStateVertShaderText,VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
this);

    shaderStages[0].sType  =
VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].shader = vs.handle();
    shaderStages[1].sType  =
VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    shaderStages[1].shader = tc.handle();
    shaderStages[2].sType  =
VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[2].stage  = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    shaderStages[2].shader = te.handle();

    VkPipelineInputAssemblyStateCreateInfo iaCI = {};
        iaCI.sType =
VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        iaCI.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

    VkPipelineTessellationStateCreateInfo tsCI = {};
        tsCI.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tsCI.patchControlPoints = 0; // This will cause an error

    VkGraphicsPipelineCreateInfo gp_ci = {};
        gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gp_ci.pNext = NULL;
        gp_ci.stageCount = 3;
        gp_ci.pStages = shaderStages;
        gp_ci.pVertexInputState = NULL;
        gp_ci.pInputAssemblyState = &iaCI;
        gp_ci.pTessellationState = &tsCI;
        gp_ci.pViewportState = NULL;
        gp_ci.pRasterizationState = NULL;
        gp_ci.pMultisampleState = NULL;
        gp_ci.pDepthStencilState = NULL;
        gp_ci.pColorBlendState = NULL;
        gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
        gp_ci.layout = pipeline_layout;
        gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
        pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pc_ci.pNext = NULL;
        pc_ci.initialSize = 0;
        pc_ci.initialData = 0;
        pc_ci.maxSize = 0;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL,
&pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1,
&gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}
*/
// Set scissor and viewport counts to different numbers
TEST_F(VkLayerTest, PSOViewportScissorCountMismatch) {
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Gfx Pipeline viewport count (1) must match scissor count (0).");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkViewport vp = {}; // Just need dummy vp to point to

    VkPipelineViewportStateCreateInfo vp_state_ci = {};
    vp_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_ci.scissorCount = 0;
    vp_state_ci.viewportCount = 1; // Count mismatch should cause error
    vp_state_ci.pViewports = &vp;

    VkPipelineRasterizationStateCreateInfo rs_state_ci = {};
    rs_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_state_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rs_state_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    rs_state_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs_state_ci.depthClampEnable = VK_FALSE;
    rs_state_ci.rasterizerDiscardEnable = VK_FALSE;
    rs_state_ci.depthBiasEnable = VK_FALSE;

    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    shaderStages[0] = vs.GetStageCreateInfo();
    shaderStages[1] = fs.GetStageCreateInfo();

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.stageCount = 2;
    gp_ci.pStages = shaderStages;
    gp_ci.pViewportState = &vp_state_ci;
    gp_ci.pRasterizationState = &rs_state_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}
// Don't set viewport state in PSO. This is an error b/c we always need this
// state
//  for the counts even if the data is going to be set dynamically.
TEST_F(VkLayerTest, PSOViewportStateNotSet) {
    // Attempt to Create Gfx Pipeline w/o a VS
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Gfx Pipeline pViewportState is null. Even if ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkDynamicState sc_state = VK_DYNAMIC_STATE_SCISSOR;
    // Set scissor as dynamic to avoid second error
    VkPipelineDynamicStateCreateInfo dyn_state_ci = {};
    dyn_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_state_ci.dynamicStateCount = 1;
    dyn_state_ci.pDynamicStates = &sc_state;

    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    shaderStages[0] = vs.GetStageCreateInfo();
    shaderStages[1] = fs.GetStageCreateInfo();

    VkPipelineRasterizationStateCreateInfo rs_state_ci = {};
    rs_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_state_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rs_state_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    rs_state_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs_state_ci.depthClampEnable = VK_FALSE;
    rs_state_ci.rasterizerDiscardEnable = VK_FALSE;
    rs_state_ci.depthBiasEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.stageCount = 2;
    gp_ci.pStages = shaderStages;
    gp_ci.pRasterizationState = &rs_state_ci;
    gp_ci.pViewportState = NULL; // Not setting VP state w/o dynamic vp state
                                 // should cause validation error
    gp_ci.pDynamicState = &dyn_state_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}
// Create PSO w/o non-zero viewportCount but no viewport data
// Then run second test where dynamic scissor count doesn't match PSO scissor
// count
TEST_F(VkLayerTest, PSOViewportCountWithoutDataAndDynScissorMismatch) {
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Gfx Pipeline viewportCount is 1, but pViewports is NULL. ");

    ASSERT_NO_FATAL_FAILURE(InitState());

    if (!m_device->phy().features().multiViewport) {
        printf("Device does not support multiple viewports/scissors; skipped.\n");
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineViewportStateCreateInfo vp_state_ci = {};
    vp_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_ci.viewportCount = 1;
    vp_state_ci.pViewports = NULL; // Null vp w/ count of 1 should cause error
    vp_state_ci.scissorCount = 1;
    vp_state_ci.pScissors = NULL; // Scissor is dynamic (below) so this won't cause error

    VkDynamicState sc_state = VK_DYNAMIC_STATE_SCISSOR;
    // Set scissor as dynamic to avoid that error
    VkPipelineDynamicStateCreateInfo dyn_state_ci = {};
    dyn_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_state_ci.dynamicStateCount = 1;
    dyn_state_ci.pDynamicStates = &sc_state;

    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    shaderStages[0] = vs.GetStageCreateInfo();
    shaderStages[1] = fs.GetStageCreateInfo();

    VkPipelineVertexInputStateCreateInfo vi_ci = {};
    vi_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_ci.pNext = nullptr;
    vi_ci.vertexBindingDescriptionCount = 0;
    vi_ci.pVertexBindingDescriptions = nullptr;
    vi_ci.vertexAttributeDescriptionCount = 0;
    vi_ci.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo ia_ci = {};
    ia_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.pNext = nullptr;

    VkPipelineColorBlendAttachmentState att = {};
    att.blendEnable = VK_FALSE;
    att.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo cb_ci = {};
    cb_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_ci.pNext = nullptr;
    cb_ci.attachmentCount = 1;
    cb_ci.pAttachments = &att;

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.stageCount = 2;
    gp_ci.pStages = shaderStages;
    gp_ci.pVertexInputState = &vi_ci;
    gp_ci.pInputAssemblyState = &ia_ci;
    gp_ci.pViewportState = &vp_state_ci;
    gp_ci.pRasterizationState = &rs_ci;
    gp_ci.pColorBlendState = &cb_ci;
    gp_ci.pDynamicState = &dyn_state_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();

    // Now hit second fail case where we set scissor w/ different count than PSO
    // First need to successfully create the PSO from above by setting
    // pViewports
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Dynamic scissor(s) 0 are used by pipeline state object, ");

    VkViewport vp = {}; // Just need dummy vp to point to
    vp_state_ci.pViewports = &vp;
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);
    ASSERT_VK_SUCCESS(err);
    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkRect2D scissors[1] = {}; // don't care about data
    // Count of 2 doesn't match PSO count of 1
    vkCmdSetScissor(m_commandBuffer->GetBufferHandle(), 1, 1, scissors);
    Draw(1, 0, 0, 0);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
    vkDestroyPipeline(m_device->device(), pipeline, NULL);
}
// Create PSO w/o non-zero scissorCount but no scissor data
// Then run second test where dynamic viewportCount doesn't match PSO
// viewportCount
TEST_F(VkLayerTest, PSOScissorCountWithoutDataAndDynViewportMismatch) {
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Gfx Pipeline scissorCount is 1, but pScissors is NULL. ");

    ASSERT_NO_FATAL_FAILURE(InitState());

    if (!m_device->phy().features().multiViewport) {
        printf("Device does not support multiple viewports/scissors; skipped.\n");
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineViewportStateCreateInfo vp_state_ci = {};
    vp_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_ci.scissorCount = 1;
    vp_state_ci.pScissors = NULL; // Null scissor w/ count of 1 should cause error
    vp_state_ci.viewportCount = 1;
    vp_state_ci.pViewports = NULL; // vp is dynamic (below) so this won't cause error

    VkDynamicState vp_state = VK_DYNAMIC_STATE_VIEWPORT;
    // Set scissor as dynamic to avoid that error
    VkPipelineDynamicStateCreateInfo dyn_state_ci = {};
    dyn_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_state_ci.dynamicStateCount = 1;
    dyn_state_ci.pDynamicStates = &vp_state;

    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    shaderStages[0] = vs.GetStageCreateInfo();
    shaderStages[1] = fs.GetStageCreateInfo();

    VkPipelineVertexInputStateCreateInfo vi_ci = {};
    vi_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_ci.pNext = nullptr;
    vi_ci.vertexBindingDescriptionCount = 0;
    vi_ci.pVertexBindingDescriptions = nullptr;
    vi_ci.vertexAttributeDescriptionCount = 0;
    vi_ci.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo ia_ci = {};
    ia_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.pNext = nullptr;

    VkPipelineColorBlendAttachmentState att = {};
    att.blendEnable = VK_FALSE;
    att.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo cb_ci = {};
    cb_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_ci.pNext = nullptr;
    cb_ci.attachmentCount = 1;
    cb_ci.pAttachments = &att;

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.stageCount = 2;
    gp_ci.pStages = shaderStages;
    gp_ci.pVertexInputState = &vi_ci;
    gp_ci.pInputAssemblyState = &ia_ci;
    gp_ci.pViewportState = &vp_state_ci;
    gp_ci.pRasterizationState = &rs_ci;
    gp_ci.pColorBlendState = &cb_ci;
    gp_ci.pDynamicState = &dyn_state_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();

    // Now hit second fail case where we set scissor w/ different count than PSO
    // First need to successfully create the PSO from above by setting
    // pViewports
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Dynamic viewport(s) 0 are used by pipeline state object, ");

    VkRect2D sc = {}; // Just need dummy vp to point to
    vp_state_ci.pScissors = &sc;
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);
    ASSERT_VK_SUCCESS(err);
    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkViewport viewports[1] = {}; // don't care about data
    // Count of 2 doesn't match PSO count of 1
    vkCmdSetViewport(m_commandBuffer->GetBufferHandle(), 1, 1, viewports);
    Draw(1, 0, 0, 0);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
    vkDestroyPipeline(m_device->device(), pipeline, NULL);
}

TEST_F(VkLayerTest, PSOLineWidthInvalid) {
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attempt to set lineWidth to -1");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineViewportStateCreateInfo vp_state_ci = {};
    vp_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_ci.scissorCount = 1;
    vp_state_ci.pScissors = NULL;
    vp_state_ci.viewportCount = 1;
    vp_state_ci.pViewports = NULL;

    VkDynamicState dynamic_states[3] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH};
    // Set scissor as dynamic to avoid that error
    VkPipelineDynamicStateCreateInfo dyn_state_ci = {};
    dyn_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_state_ci.dynamicStateCount = 2;
    dyn_state_ci.pDynamicStates = dynamic_states;

    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT,
                   this); // TODO - We shouldn't need a fragment shader
                          // but add it to be able to run on more devices
    shaderStages[0] = vs.GetStageCreateInfo();
    shaderStages[1] = fs.GetStageCreateInfo();

    VkPipelineVertexInputStateCreateInfo vi_ci = {};
    vi_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_ci.pNext = nullptr;
    vi_ci.vertexBindingDescriptionCount = 0;
    vi_ci.pVertexBindingDescriptions = nullptr;
    vi_ci.vertexAttributeDescriptionCount = 0;
    vi_ci.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo ia_ci = {};
    ia_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.pNext = nullptr;

    // Check too low (line width of -1.0f).
    rs_ci.lineWidth = -1.0f;

    VkPipelineColorBlendAttachmentState att = {};
    att.blendEnable = VK_FALSE;
    att.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo cb_ci = {};
    cb_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_ci.pNext = nullptr;
    cb_ci.attachmentCount = 1;
    cb_ci.pAttachments = &att;

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.stageCount = 2;
    gp_ci.pStages = shaderStages;
    gp_ci.pVertexInputState = &vi_ci;
    gp_ci.pInputAssemblyState = &ia_ci;
    gp_ci.pViewportState = &vp_state_ci;
    gp_ci.pRasterizationState = &rs_ci;
    gp_ci.pColorBlendState = &cb_ci;
    gp_ci.pDynamicState = &dyn_state_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();
    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attempt to set lineWidth to 65536");

    // Check too high (line width of 65536.0f).
    rs_ci.lineWidth = 65536.0f;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);

    m_errorMonitor->VerifyFound();
    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attempt to set lineWidth to -1");

    dyn_state_ci.dynamicStateCount = 3;

    rs_ci.lineWidth = 1.0f;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);
    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Check too low with dynamic setting.
    vkCmdSetLineWidth(m_commandBuffer->GetBufferHandle(), -1.0f);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attempt to set lineWidth to 65536");

    // Check too high with dynamic setting.
    vkCmdSetLineWidth(m_commandBuffer->GetBufferHandle(), 65536.0f);
    m_errorMonitor->VerifyFound();
    EndCommandBuffer();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
    vkDestroyPipeline(m_device->device(), pipeline, NULL);
}

TEST_F(VkLayerTest, NullRenderPass) {
    // Bind a NULL RenderPass
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();
    // Don't care about RenderPass handle b/c error should be flagged before
    // that
    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), NULL, VK_SUBPASS_CONTENTS_INLINE);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, RenderPassWithinRenderPass) {
    // Bind a BeginRenderPass within an active RenderPass
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "It is invalid to issue this call inside an active render pass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();
    // Just create a dummy Renderpass that's non-NULL so we can get to the
    // proper error
    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), &m_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, RenderPassClearOpMismatch) {
    TEST_DESCRIPTION("Begin a renderPass where clearValueCount is less than"
                     "the number of renderPass attachments that use loadOp"
                     "VK_ATTACHMENT_LOAD_OP_CLEAR.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Create a renderPass with a single attachment that uses loadOp CLEAR
    VkAttachmentReference attach = {};
    attach.layout = VK_IMAGE_LAYOUT_GENERAL;
    VkSubpassDescription subpass = {};
    subpass.inputAttachmentCount = 1;
    subpass.pInputAttachments = &attach;
    VkRenderPassCreateInfo rpci = {};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.attachmentCount = 1;
    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_UNDEFINED;
    // Set loadOp to CLEAR
    attach_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    rpci.pAttachments = &attach_desc;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkRenderPass rp;
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);

    VkCommandBufferInheritanceInfo hinfo = {};
    hinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    hinfo.renderPass = VK_NULL_HANDLE;
    hinfo.subpass = 0;
    hinfo.framebuffer = VK_NULL_HANDLE;
    hinfo.occlusionQueryEnable = VK_FALSE;
    hinfo.queryFlags = 0;
    hinfo.pipelineStatistics = 0;
    VkCommandBufferBeginInfo info = {};
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pInheritanceInfo = &hinfo;

    vkBeginCommandBuffer(m_commandBuffer->handle(), &info);
    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = renderPass();
    rp_begin.framebuffer = framebuffer();
    rp_begin.clearValueCount = 0; // Should be 1

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " has a clearValueCount of 0 but "
                                                                        "there must be at least 1 entries in "
                                                                        "pClearValues array to account for ");

    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    m_errorMonitor->VerifyFound();

    vkDestroyRenderPass(m_device->device(), rp, NULL);
}

TEST_F(VkLayerTest, EndCommandBufferWithinRenderPass) {

    TEST_DESCRIPTION("End a command buffer with an active render pass");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "It is invalid to issue this call inside an active render pass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // The framework's BeginCommandBuffer calls CreateRenderPass
    BeginCommandBuffer();

    // Call directly into vkEndCommandBuffer instead of the
    // the framework's EndCommandBuffer, which inserts a
    // vkEndRenderPass
    vkEndCommandBuffer(m_commandBuffer->GetBufferHandle());

    m_errorMonitor->VerifyFound();

    // TODO: Add test for VK_COMMAND_BUFFER_LEVEL_SECONDARY
    // TODO: Add test for VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
}

TEST_F(VkLayerTest, FillBufferWithinRenderPass) {
    // Call CmdFillBuffer within an active renderpass
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "It is invalid to issue this call inside an active render pass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Renderpass is started here
    BeginCommandBuffer();

    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_testing::Buffer dstBuffer;
    dstBuffer.init_as_dst(*m_device, (VkDeviceSize)1024, reqs);

    m_commandBuffer->FillBuffer(dstBuffer.handle(), 0, 4, 0x11111111);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, UpdateBufferWithinRenderPass) {
    // Call CmdUpdateBuffer within an active renderpass
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "It is invalid to issue this call inside an active render pass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Renderpass is started here
    BeginCommandBuffer();

    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_testing::Buffer dstBuffer;
    dstBuffer.init_as_dst(*m_device, (VkDeviceSize)1024, reqs);

    VkDeviceSize dstOffset = 0;
    VkDeviceSize dataSize = 1024;
    const void *pData = NULL;

    vkCmdUpdateBuffer(m_commandBuffer->GetBufferHandle(), dstBuffer.handle(), dstOffset, dataSize, pData);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ClearColorImageWithinRenderPass) {
    // Call CmdClearColorImage within an active RenderPass
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "It is invalid to issue this call inside an active render pass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Renderpass is started here
    BeginCommandBuffer();

    VkClearColorValue clear_color;
    memset(clear_color.uint32, 0, sizeof(uint32_t) * 4);
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    vk_testing::Image dstImage;
    dstImage.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);

    const VkImageSubresourceRange range = vk_testing::Image::subresource_range(image_create_info, VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdClearColorImage(m_commandBuffer->GetBufferHandle(), dstImage.handle(), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ClearDepthStencilImageWithinRenderPass) {
    // Call CmdClearDepthStencilImage within an active RenderPass
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "It is invalid to issue this call inside an active render pass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Renderpass is started here
    BeginCommandBuffer();

    VkClearDepthStencilValue clear_value = {0};
    VkMemoryPropertyFlags reqs = 0;
    VkImageCreateInfo image_create_info = vk_testing::Image::create_info();
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    image_create_info.extent.width = 64;
    image_create_info.extent.height = 64;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    vk_testing::Image dstImage;
    dstImage.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);

    const VkImageSubresourceRange range = vk_testing::Image::subresource_range(image_create_info, VK_IMAGE_ASPECT_DEPTH_BIT);

    vkCmdClearDepthStencilImage(m_commandBuffer->GetBufferHandle(), dstImage.handle(),
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &clear_value, 1, &range);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ClearColorAttachmentsOutsideRenderPass) {
    // Call CmdClearAttachmentss outside of an active RenderPass
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdClearAttachments(): This call "
                                                                        "must be issued inside an active "
                                                                        "render pass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Start no RenderPass
    err = m_commandBuffer->BeginCommandBuffer();
    ASSERT_VK_SUCCESS(err);

    VkClearAttachment color_attachment;
    color_attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_attachment.clearValue.color.float32[0] = 0;
    color_attachment.clearValue.color.float32[1] = 0;
    color_attachment.clearValue.color.float32[2] = 0;
    color_attachment.clearValue.color.float32[3] = 0;
    color_attachment.colorAttachment = 0;
    VkClearRect clear_rect = {{{0, 0}, {32, 32}}};
    vkCmdClearAttachments(m_commandBuffer->GetBufferHandle(), 1, &color_attachment, 1, &clear_rect);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, RenderPassExcessiveNextSubpass) {
    TEST_DESCRIPTION("Test that an error is produced when CmdNextSubpass is "
                     "called too many times in a renderpass instance");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdNextSubpass(): Attempted to advance "
                                                                        "beyond final subpass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();

    // error here.
    vkCmdNextSubpass(m_commandBuffer->GetBufferHandle(), VK_SUBPASS_CONTENTS_INLINE);
    m_errorMonitor->VerifyFound();

    EndCommandBuffer();
}

TEST_F(VkLayerTest, RenderPassEndedBeforeFinalSubpass) {
    TEST_DESCRIPTION("Test that an error is produced when CmdEndRenderPass is "
                     "called before the final subpass has been reached");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdEndRenderPass(): Called before reaching "
                                                                        "final subpass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkSubpassDescription sd[2] = {{0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 0, nullptr, nullptr, nullptr, 0, nullptr},
                                  {0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 0, nullptr, nullptr, nullptr, 0, nullptr}};

    VkRenderPassCreateInfo rcpi = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 0, nullptr, 2, sd, 0, nullptr};

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rcpi, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fbci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, rp, 0, nullptr, 16, 16, 1};

    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fbci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    m_commandBuffer->BeginCommandBuffer(); // no implicit RP begin

    VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, rp, fb, {{0, 0}, {16, 16}}, 0, nullptr};

    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    // Error here.
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle());
    m_errorMonitor->VerifyFound();

    // Clean up.
    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
}

TEST_F(VkLayerTest, BufferMemoryBarrierNoBuffer) {
    // Try to add a buffer memory barrier with no buffer.
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "required parameter pBufferMemoryBarriers[0].buffer specified as VK_NULL_HANDLE");

    ASSERT_NO_FATAL_FAILURE(InitState());
    BeginCommandBuffer();

    VkBufferMemoryBarrier buf_barrier = {};
    buf_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buf_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    buf_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    buf_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_barrier.buffer = VK_NULL_HANDLE;
    buf_barrier.offset = 0;
    buf_barrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 1, &buf_barrier, 0, nullptr);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidBarriers) {
    TEST_DESCRIPTION("A variety of ways to get VK_INVALID_BARRIER ");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Barriers cannot be set during subpass");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkMemoryBarrier mem_barrier = {};
    mem_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    mem_barrier.pNext = NULL;
    mem_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    mem_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    BeginCommandBuffer();
    // BeginCommandBuffer() starts a render pass
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1,
                         &mem_barrier, 0, nullptr, 0, nullptr);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Image Layout cannot be transitioned to UNDEFINED");
    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());
    VkImageMemoryBarrier img_barrier = {};
    img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.pNext = NULL;
    img_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    img_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // New layout can't be UNDEFINED
    img_barrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_barrier.image = image.handle();
    img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier.subresourceRange.baseArrayLayer = 0;
    img_barrier.subresourceRange.baseMipLevel = 0;
    img_barrier.subresourceRange.layerCount = 1;
    img_barrier.subresourceRange.levelCount = 1;
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &img_barrier);
    m_errorMonitor->VerifyFound();
    img_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Subresource must have the sum of the "
                                                                        "baseArrayLayer");
    // baseArrayLayer + layerCount must be <= image's arrayLayers
    img_barrier.subresourceRange.baseArrayLayer = 1;
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &img_barrier);
    m_errorMonitor->VerifyFound();
    img_barrier.subresourceRange.baseArrayLayer = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Subresource must have the sum of the baseMipLevel");
    // baseMipLevel + levelCount must be <= image's mipLevels
    img_barrier.subresourceRange.baseMipLevel = 1;
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &img_barrier);
    m_errorMonitor->VerifyFound();
    img_barrier.subresourceRange.baseMipLevel = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Buffer Barriers cannot be used during a render pass");
    vk_testing::Buffer buffer;
    buffer.init(*m_device, 256);
    VkBufferMemoryBarrier buf_barrier = {};
    buf_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buf_barrier.pNext = NULL;
    buf_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    buf_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    buf_barrier.buffer = buffer.handle();
    buf_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_barrier.offset = 0;
    buf_barrier.size = VK_WHOLE_SIZE;
    // Can't send buffer barrier during a render pass
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 1, &buf_barrier, 0, nullptr);
    m_errorMonitor->VerifyFound();
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "which is not less than total size");
    buf_barrier.offset = 257;
    // Offset greater than total size
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 1, &buf_barrier, 0, nullptr);
    m_errorMonitor->VerifyFound();
    buf_barrier.offset = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "whose sum is greater than total size");
    buf_barrier.size = 257;
    // Size greater than total size
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 1, &buf_barrier, 0, nullptr);
    m_errorMonitor->VerifyFound();

    // Now exercise barrier aspect bit errors, first DS
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Image is a depth and stencil format and thus must "
                                                                        "have either one or both of VK_IMAGE_ASPECT_DEPTH_BIT and "
                                                                        "VK_IMAGE_ASPECT_STENCIL_BIT set.");
    VkDepthStencilObj ds_image(m_device);
    ds_image.Init(m_device, 128, 128, VK_FORMAT_D24_UNORM_S8_UINT);
    ASSERT_TRUE(ds_image.initialized());
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_barrier.image = ds_image.handle();
    // Use of COLOR aspect on DS image is error
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &img_barrier);
    m_errorMonitor->VerifyFound();
    // Now test depth-only
    VkFormatProperties format_props;

    vkGetPhysicalDeviceFormatProperties(m_device->phy().handle(), VK_FORMAT_D16_UNORM, &format_props);
    if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Image is a depth-only format and thus must "
                                                                            "have VK_IMAGE_ASPECT_DEPTH_BIT set.");
        VkDepthStencilObj d_image(m_device);
        d_image.Init(m_device, 128, 128, VK_FORMAT_D16_UNORM);
        ASSERT_TRUE(d_image.initialized());
        img_barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        img_barrier.image = d_image.handle();
        // Use of COLOR aspect on depth image is error
        img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &img_barrier);
        m_errorMonitor->VerifyFound();
    }
    vkGetPhysicalDeviceFormatProperties(m_device->phy().handle(), VK_FORMAT_S8_UINT, &format_props);
    if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        // Now test stencil-only
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Image is a stencil-only format and thus must "
                                                                            "have VK_IMAGE_ASPECT_STENCIL_BIT set.");
        VkDepthStencilObj s_image(m_device);
        s_image.Init(m_device, 128, 128, VK_FORMAT_S8_UINT);
        ASSERT_TRUE(s_image.initialized());
        img_barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        img_barrier.image = s_image.handle();
        // Use of COLOR aspect on depth image is error
        img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &img_barrier);
        m_errorMonitor->VerifyFound();
    }
    // Finally test color
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Image is a color format and thus must "
                                                                        "have VK_IMAGE_ASPECT_COLOR_BIT set.");
    VkImageObj c_image(m_device);
    c_image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(c_image.initialized());
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_barrier.image = c_image.handle();
    // Set aspect to depth (non-color)
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &img_barrier);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, LayoutFromPresentWithoutAccessMemoryRead) {
    // Transition an image away from PRESENT_SRC_KHR without ACCESS_MEMORY_READ in srcAccessMask

    m_errorMonitor->SetDesiredFailureMsg(
        VK_DEBUG_REPORT_WARNING_BIT_EXT,
        "must have required access bit");
    ASSERT_NO_FATAL_FAILURE(InitState());
    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageMemoryBarrier barrier = {};
    VkImageSubresourceRange range;
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.image = image.handle();
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    barrier.subresourceRange = range;
    VkCommandBufferObj cmdbuf(m_device, m_commandPool);
    cmdbuf.BeginCommandBuffer();
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &barrier);
    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &barrier);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, IdxBufferAlignmentError) {
    // Bind a BeginRenderPass within an active RenderPass
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdBindIndexBuffer() offset (0x7) does not fall on ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    uint32_t qfi = 0;
    VkBufferCreateInfo buffCI = {};
    buffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffCI.size = 1024;
    buffCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffCI.queueFamilyIndexCount = 1;
    buffCI.pQueueFamilyIndices = &qfi;

    VkBuffer ib;
    err = vkCreateBuffer(m_device->device(), &buffCI, NULL, &ib);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    ASSERT_VK_SUCCESS(err);
    // vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(),
    // VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    // Should error before calling to driver so don't care about actual data
    vkCmdBindIndexBuffer(m_commandBuffer->GetBufferHandle(), ib, 7, VK_INDEX_TYPE_UINT16);

    m_errorMonitor->VerifyFound();

    vkDestroyBuffer(m_device->device(), ib, NULL);
}

TEST_F(VkLayerTest, InvalidQueueFamilyIndex) {
    // Create an out-of-range queueFamilyIndex
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCreateBuffer: pCreateInfo->pQueueFamilyIndices[0] (777) must be one "
                                         "of the indices specified when the device was created, via the "
                                         "VkDeviceQueueCreateInfo structure.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    VkBufferCreateInfo buffCI = {};
    buffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffCI.size = 1024;
    buffCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffCI.queueFamilyIndexCount = 1;
    // Introduce failure by specifying invalid queue_family_index
    uint32_t qfi = 777;
    buffCI.pQueueFamilyIndices = &qfi;
    buffCI.sharingMode = VK_SHARING_MODE_CONCURRENT; // qfi only matters in CONCURRENT mode

    VkBuffer ib;
    vkCreateBuffer(m_device->device(), &buffCI, NULL, &ib);

    m_errorMonitor->VerifyFound();
    vkDestroyBuffer(m_device->device(), ib, NULL);
}

TEST_F(VkLayerTest, ExecuteCommandsPrimaryCB) {
TEST_DESCRIPTION("Attempt vkCmdExecuteCommands with a primary command buffer"
                     " (should only be secondary)");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // An empty primary command buffer
    VkCommandBufferObj cb(m_device, m_commandPool);
    cb.BeginCommandBuffer();
    cb.EndCommandBuffer();

    m_commandBuffer->BeginCommandBuffer();
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &renderPassBeginInfo(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    VkCommandBuffer handle = cb.handle();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdExecuteCommands() called w/ Primary Cmd Buffer ");
    vkCmdExecuteCommands(m_commandBuffer->handle(), 1, &handle);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, DSUsageBitsErrors) {
    TEST_DESCRIPTION("Attempt to update descriptor sets for images and buffers "
                     "that do not have correct usage bits sets.");
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkDescriptorPoolSize ds_type_count[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {};
    for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i) {
        ds_type_count[i].type = VkDescriptorType(i);
        ds_type_count[i].descriptorCount = 1;
    }
    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = VK_DESCRIPTOR_TYPE_RANGE_SIZE;
    ds_pool_ci.poolSizeCount = VK_DESCRIPTOR_TYPE_RANGE_SIZE;
    ds_pool_ci.pPoolSizes = ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    // Create 10 layouts where each has a single descriptor of different type
    VkDescriptorSetLayoutBinding dsl_binding[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {};
    for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i) {
        dsl_binding[i].binding = 0;
        dsl_binding[i].descriptorType = VkDescriptorType(i);
        dsl_binding[i].descriptorCount = 1;
        dsl_binding[i].stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding[i].pImmutableSamplers = NULL;
    }

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    VkDescriptorSetLayout ds_layouts[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
    for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i) {
        ds_layout_ci.pBindings = dsl_binding + i;
        err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, ds_layouts + i);
        ASSERT_VK_SUCCESS(err);
    }
    VkDescriptorSet descriptor_sets[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {};
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = VK_DESCRIPTOR_TYPE_RANGE_SIZE;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = ds_layouts;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, descriptor_sets);
    ASSERT_VK_SUCCESS(err);

    // Create a buffer & bufferView to be used for invalid updates
    VkBufferCreateInfo buff_ci = {};
    buff_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // This usage is not valid for any descriptor type
    buff_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buff_ci.size = 256;
    buff_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer;
    err = vkCreateBuffer(m_device->device(), &buff_ci, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    VkBufferViewCreateInfo buff_view_ci = {};
    buff_view_ci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    buff_view_ci.buffer = buffer;
    buff_view_ci.format = VK_FORMAT_R8_UNORM;
    buff_view_ci.range = VK_WHOLE_SIZE;
    VkBufferView buff_view;
    err = vkCreateBufferView(m_device->device(), &buff_view_ci, NULL, &buff_view);
    ASSERT_VK_SUCCESS(err);

    // Create an image to be used for invalid updates
    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_ci.extent.width = 64;
    image_ci.extent.height = 64;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_LINEAR;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    // This usage is not valid for any descriptor type
    image_ci.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkImage image;
    err = vkCreateImage(m_device->device(), &image_ci, NULL, &image);
    ASSERT_VK_SUCCESS(err);
    // Bind memory to image
    VkMemoryRequirements mem_reqs;
    VkDeviceMemory image_mem;
    bool pass;
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &image_mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, image_mem, 0);
    ASSERT_VK_SUCCESS(err);
    // Now create view for image
    VkImageViewCreateInfo image_view_ci = {};
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.image = image;
    image_view_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_ci.subresourceRange.layerCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageView image_view;
    err = vkCreateImageView(m_device->device(), &image_view_ci, NULL, &image_view);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorBufferInfo buff_info = {};
    buff_info.buffer = buffer;
    VkDescriptorImageInfo img_info = {};
    img_info.imageView = image_view;
    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pTexelBufferView = &buff_view;
    descriptor_write.pBufferInfo = &buff_info;
    descriptor_write.pImageInfo = &img_info;

    // These error messages align with VkDescriptorType struct
    const char *error_msgs[] = {"", // placeholder, no error for SAMPLER descriptor
                                " does not have VK_IMAGE_USAGE_SAMPLED_BIT set.",
                                " does not have VK_IMAGE_USAGE_SAMPLED_BIT set.",
                                " does not have VK_IMAGE_USAGE_STORAGE_BIT set.",
                                " does not have VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT set.",
                                " does not have VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT set.",
                                " does not have VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT set.",
                                " does not have VK_BUFFER_USAGE_STORAGE_BUFFER_BIT set.",
                                " does not have VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT set.",
                                " does not have VK_BUFFER_USAGE_STORAGE_BUFFER_BIT set.",
                                " does not have VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT set."};
    // Start loop at 1 as SAMPLER desc type has no usage bit error
    for (uint32_t i = 1; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i) {
        descriptor_write.descriptorType = VkDescriptorType(i);
        descriptor_write.dstSet = descriptor_sets[i];
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, error_msgs[i]);

        vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

        m_errorMonitor->VerifyFound();
        vkDestroyDescriptorSetLayout(m_device->device(), ds_layouts[i], NULL);
    }
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layouts[0], NULL);
    vkDestroyImage(m_device->device(), image, NULL);
    vkFreeMemory(m_device->device(), image_mem, NULL);
    vkDestroyImageView(m_device->device(), image_view, NULL);
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkDestroyBufferView(m_device->device(), buff_view, NULL);
    vkFreeDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_TYPE_RANGE_SIZE, descriptor_sets);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, DSBufferInfoErrors) {
    TEST_DESCRIPTION("Attempt to update buffer descriptor set that has incorrect "
                     "parameters in VkDescriptorBufferInfo struct. This includes:\n"
                     "1. offset value greater than buffer size\n"
                     "2. range value of 0\n"
                     "3. range value greater than buffer (size - offset)");
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    // Create layout with single uniform buffer descriptor
    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptor_set = {};
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    // Create a buffer to be used for invalid updates
    VkBufferCreateInfo buff_ci = {};
    buff_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buff_ci.size = 256;
    buff_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer;
    err = vkCreateBuffer(m_device->device(), &buff_ci, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);
    // Have to bind memory to buffer before descriptor update
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 256;
    mem_alloc.memoryTypeIndex = 0;

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(m_device->device(), buffer, &mem_reqs);
    bool pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }

    VkDeviceMemory mem;
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorBufferInfo buff_info = {};
    buff_info.buffer = buffer;
    // First make offset 1 larger than buffer size
    buff_info.offset = 257;
    buff_info.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pTexelBufferView = nullptr;
    descriptor_write.pBufferInfo = &buff_info;
    descriptor_write.pImageInfo = nullptr;

    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.dstSet = descriptor_set;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " offset of 257 is greater than buffer ");

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();
    // Now cause error due to range of 0
    buff_info.offset = 0;
    buff_info.range = 0;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " range is not VK_WHOLE_SIZE and is zero, which is not allowed.");

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();
    // Now cause error due to range exceeding buffer size - offset
    buff_info.offset = 128;
    buff_info.range = 200;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " range is 200 which is greater than buffer size ");

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();
    vkFreeMemory(m_device->device(), mem, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkFreeDescriptorSets(m_device->device(), ds_pool, 1, &descriptor_set);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, DSAspectBitsErrors) {
    // TODO : Initially only catching case where DEPTH & STENCIL aspect bits
    //  are set, but could expand this test to hit more cases.
    TEST_DESCRIPTION("Attempt to update descriptor sets for images "
                     "that do not have correct aspect bits sets.");
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 5;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptor_set = {};
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    // Create an image to be used for invalid updates
    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_D24_UNORM_S8_UINT;
    image_ci.extent.width = 64;
    image_ci.extent.height = 64;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_LINEAR;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkImage image;
    err = vkCreateImage(m_device->device(), &image_ci, NULL, &image);
    ASSERT_VK_SUCCESS(err);
    // Bind memory to image
    VkMemoryRequirements mem_reqs;
    VkDeviceMemory image_mem;
    bool pass;
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;
    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &image_mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), image, image_mem, 0);
    ASSERT_VK_SUCCESS(err);
    // Now create view for image
    VkImageViewCreateInfo image_view_ci = {};
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.image = image;
    image_view_ci.format = VK_FORMAT_D24_UNORM_S8_UINT;
    image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_ci.subresourceRange.layerCount = 1;
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.levelCount = 1;
    // Setting both depth & stencil aspect bits is illegal for descriptor
    image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    VkImageView image_view;
    err = vkCreateImageView(m_device->device(), &image_view_ci, NULL, &image_view);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorImageInfo img_info = {};
    img_info.imageView = image_view;
    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pTexelBufferView = NULL;
    descriptor_write.pBufferInfo = NULL;
    descriptor_write.pImageInfo = &img_info;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptor_write.dstSet = descriptor_set;
    const char *error_msg = " please only set either VK_IMAGE_ASPECT_DEPTH_BIT "
                            "or VK_IMAGE_ASPECT_STENCIL_BIT ";
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, error_msg);

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyImage(m_device->device(), image, NULL);
    vkFreeMemory(m_device->device(), image_mem, NULL);
    vkDestroyImageView(m_device->device(), image_view, NULL);
    vkFreeDescriptorSets(m_device->device(), ds_pool, 1, &descriptor_set);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, DSTypeMismatch) {
    // Create DS w/ layout of one type and attempt Update w/ mis-matched type
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " binding #0 with type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER but update "
                                         "type is VK_DESCRIPTOR_TYPE_SAMPLER");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // VkDescriptorSetObj descriptorSet(m_device);
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorImageInfo info = {};
    info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.descriptorCount = 1;
    // This is a mismatched type for the layout which expects BUFFER
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pImageInfo = &info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, DSUpdateOutOfBounds) {
    // For overlapping Update, have arrayIndex exceed that of layout
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " binding #0 with 1 total descriptors but update of 1 descriptors "
                                         "starting at binding offset of 0 combined with update array element "
                                         "offset of 1 oversteps the size of this descriptor set.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // VkDescriptorSetObj descriptorSet(m_device);
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    // Correctly update descriptor to avoid "NOT_UPDATED" error
    VkDescriptorBufferInfo buff_info = {};
    buff_info.buffer = VkBuffer(0); // Don't care about buffer handle for this test
    buff_info.offset = 0;
    buff_info.range = 1024;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstArrayElement = 1; /* This index out of bounds for the update */
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.pBufferInfo = &buff_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidDSUpdateIndex) {
    // Create layout w/ count of 1 and attempt update to that layout w/ binding
    // index 2
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " does not have binding 2.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // VkDescriptorSetObj descriptorSet(m_device);
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorImageInfo info = {};
    info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 2;
    descriptor_write.descriptorCount = 1;
    // This is the wrong type, but out of bounds will be flagged first
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pImageInfo = &info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, InvalidDSUpdateStruct) {
    // Call UpdateDS w/ struct type other than valid VK_STRUCTUR_TYPE_UPDATE_*
    // types
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, ".sType must be VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorImageInfo info = {};
    info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = (VkStructureType)0x99999999; /* Intentionally broken struct type */
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.descriptorCount = 1;
    // This is the wrong type, but out of bounds will be flagged first
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pImageInfo = &info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, SampleDescriptorUpdateError) {
    // Create a single Sampler descriptor and send it an invalid Sampler
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Attempted write update to sampler descriptor with invalid sampler");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // TODO : Farm Descriptor setup code to helper function(s) to reduce copied
    // code
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSampler sampler = (VkSampler)((size_t)0xbaadbeef); // Sampler with invalid handle

    VkDescriptorImageInfo descriptor_info;
    memset(&descriptor_info, 0, sizeof(VkDescriptorImageInfo));
    descriptor_info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pImageInfo = &descriptor_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, ImageViewDescriptorUpdateError) {
    // Create a single combined Image/Sampler descriptor and send it an invalid
    // imageView
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attempted write update to combined "
                                                                        "image sampler descriptor failed due "
                                                                        "to: Invalid VkImageView:");

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkImageView view = (VkImageView)((size_t)0xbaadbeef); // invalid imageView object

    VkDescriptorImageInfo descriptor_info;
    memset(&descriptor_info, 0, sizeof(VkDescriptorImageInfo));
    descriptor_info.sampler = sampler;
    descriptor_info.imageView = view;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo = &descriptor_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyFound();

    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, CopyDescriptorUpdateErrors) {
    // Create DS w/ layout of 2 types, write update 1 and attempt to copy-update
    // into the other
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " binding #1 with type "
                                                                        "VK_DESCRIPTOR_TYPE_SAMPLER. Types do "
                                                                        "not match.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // VkDescriptorSetObj descriptorSet(m_device);
    VkDescriptorPoolSize ds_type_count[2] = {};
    ds_type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count[0].descriptorCount = 1;
    ds_type_count[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    ds_type_count[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 2;
    ds_pool_ci.pPoolSizes = ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorSetLayoutBinding dsl_binding[2] = {};
    dsl_binding[0].binding = 0;
    dsl_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding[0].descriptorCount = 1;
    dsl_binding[0].stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding[0].pImmutableSamplers = NULL;
    dsl_binding[1].binding = 1;
    dsl_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    dsl_binding[1].descriptorCount = 1;
    dsl_binding[1].stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding[1].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 2;
    ds_layout_ci.pBindings = dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorImageInfo info = {};
    info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(VkWriteDescriptorSet));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptorSet;
    descriptor_write.dstBinding = 1; // SAMPLER binding from layout above
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pImageInfo = &info;
    // This write update should succeed
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    // Now perform a copy update that fails due to type mismatch
    VkCopyDescriptorSet copy_ds_update;
    memset(&copy_ds_update, 0, sizeof(VkCopyDescriptorSet));
    copy_ds_update.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy_ds_update.srcSet = descriptorSet;
    copy_ds_update.srcBinding = 1; // Copy from SAMPLER binding
    copy_ds_update.dstSet = descriptorSet;
    copy_ds_update.dstBinding = 0;      // ERROR : copy to UNIFORM binding
    copy_ds_update.descriptorCount = 1; // copy 1 descriptor
    vkUpdateDescriptorSets(m_device->device(), 0, NULL, 1, &copy_ds_update);

    m_errorMonitor->VerifyFound();
    // Now perform a copy update that fails due to binding out of bounds
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " does not have copy update src binding of 3.");
    memset(&copy_ds_update, 0, sizeof(VkCopyDescriptorSet));
    copy_ds_update.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy_ds_update.srcSet = descriptorSet;
    copy_ds_update.srcBinding = 3; // ERROR : Invalid binding for matching layout
    copy_ds_update.dstSet = descriptorSet;
    copy_ds_update.dstBinding = 0;
    copy_ds_update.descriptorCount = 1; // Copy 1 descriptor
    vkUpdateDescriptorSets(m_device->device(), 0, NULL, 1, &copy_ds_update);

    m_errorMonitor->VerifyFound();

    // Now perform a copy update that fails due to binding out of bounds
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " binding#1 with offset index of 1 plus "
                                                                        "update array offset of 0 and update of "
                                                                        "5 descriptors oversteps total number "
                                                                        "of descriptors in set: 2.");

    memset(&copy_ds_update, 0, sizeof(VkCopyDescriptorSet));
    copy_ds_update.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy_ds_update.srcSet = descriptorSet;
    copy_ds_update.srcBinding = 1;
    copy_ds_update.dstSet = descriptorSet;
    copy_ds_update.dstBinding = 0;
    copy_ds_update.descriptorCount = 5; // ERROR copy 5 descriptors (out of bounds for layout)
    vkUpdateDescriptorSets(m_device->device(), 0, NULL, 1, &copy_ds_update);

    m_errorMonitor->VerifyFound();

    vkDestroySampler(m_device->device(), sampler, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, NumSamplesMismatch) {
    // Create CommandBuffer where MSAA samples doesn't match RenderPass
    // sampleCount
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Num samples mismatch! ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
    pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipe_ms_state_ci.pNext = NULL;
    pipe_ms_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
    pipe_ms_state_ci.sampleShadingEnable = 0;
    pipe_ms_state_ci.minSampleShading = 1.0;
    pipe_ms_state_ci.pSampleMask = NULL;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());

    // Render triangle (the error should trigger on the attempt to draw).
    Draw(3, 1, 0, 0);

    // Finalize recording of the command buffer
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, RenderPassIncompatible) {
    TEST_DESCRIPTION("Hit RenderPass incompatible cases. "
                     "Initial case is drawing with an active renderpass that's "
                     "not compatible with the bound pipeline state object's creation renderpass");
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    // Create a renderpass that will be incompatible with default renderpass
    VkAttachmentReference attach = {};
    attach.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference color_att = {};
    color_att.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.inputAttachmentCount = 1;
    subpass.pInputAttachments = &attach;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_att;
    VkRenderPassCreateInfo rpci = {};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.attachmentCount = 1;
    VkAttachmentDescription attach_desc = {};
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    // Format incompatible with PSO RP color attach format B8G8R8A8_UNORM
    attach_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    rpci.pAttachments = &attach_desc;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkRenderPass rp;
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    VkViewport view_port = {};
    m_viewports.push_back(view_port);
    pipe.SetViewport(m_viewports);
    VkRect2D rect = {};
    m_scissors.push_back(rect);
    pipe.SetScissor(m_scissors);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    VkCommandBufferInheritanceInfo cbii = {};
    cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    cbii.renderPass = rp;
    cbii.subpass = 0;
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.pInheritanceInfo = &cbii;
    vkBeginCommandBuffer(m_commandBuffer->handle(), &cbbi);
    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.framebuffer = m_framebuffer;
    rpbi.renderPass = rp;
    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " is incompatible w/ gfx pipeline ");
    // Render triangle (the error should trigger on the attempt to draw).
    Draw(3, 1, 0, 0);

    // Finalize recording of the command buffer
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyRenderPass(m_device->device(), rp, NULL);
}

TEST_F(VkLayerTest, NumBlendAttachMismatch) {
    // Create Pipeline where the number of blend attachments doesn't match the
    // number of color attachments.  In this case, we don't add any color
    // blend attachments even though we have a color attachment.
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Render pass subpass 0 mismatch with blending state defined and blend state attachment");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
    pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipe_ms_state_ci.pNext = NULL;
    pipe_ms_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipe_ms_state_ci.sampleShadingEnable = 0;
    pipe_ms_state_ci.minSampleShading = 1.0;
    pipe_ms_state_ci.pSampleMask = NULL;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());

    // Render triangle (the error should trigger on the attempt to draw).
    Draw(3, 1, 0, 0);

    // Finalize recording of the command buffer
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, MissingClearAttachment) {
    TEST_DESCRIPTION("Points to a wrong colorAttachment index in a VkClearAttachment "
                     "structure passed to vkCmdClearAttachments");
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdClearAttachments() color attachment index 1 out of range for active subpass 0; ignored");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailCmdClearAttachments);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ClearCmdNoDraw) {
    // Create CommandBuffer where we add ClearCmd for FB Color attachment prior
    // to issuing a Draw
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                         "vkCmdClearAttachments() issued on command buffer object ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
    pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipe_ms_state_ci.pNext = NULL;
    pipe_ms_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
    pipe_ms_state_ci.sampleShadingEnable = 0;
    pipe_ms_state_ci.minSampleShading = 1.0;
    pipe_ms_state_ci.pSampleMask = NULL;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    //  We shouldn't need a fragment shader but add it to be able to run
    //  on more devices
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();

    // Main thing we care about for this test is that the VkImage obj we're
    // clearing matches Color Attachment of FB
    //  Also pass down other dummy params to keep driver and paramchecker happy
    VkClearAttachment color_attachment;
    color_attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_attachment.clearValue.color.float32[0] = 1.0;
    color_attachment.clearValue.color.float32[1] = 1.0;
    color_attachment.clearValue.color.float32[2] = 1.0;
    color_attachment.clearValue.color.float32[3] = 1.0;
    color_attachment.colorAttachment = 0;
    VkClearRect clear_rect = {{{0, 0}, {(uint32_t)m_width, (uint32_t)m_height}}};

    vkCmdClearAttachments(m_commandBuffer->GetBufferHandle(), 1, &color_attachment, 1, &clear_rect);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, VtxBufferBadIndex) {
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                         "but no vertex buffers are attached to this Pipeline State Object");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
    pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipe_ms_state_ci.pNext = NULL;
    pipe_ms_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipe_ms_state_ci.sampleShadingEnable = 0;
    pipe_ms_state_ci.minSampleShading = 1.0;
    pipe_ms_state_ci.pSampleMask = NULL;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;
    VkPipelineLayout pipeline_layout;

    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this); // We shouldn't need a fragment shader
    // but add it to be able to run on more devices
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.SetViewport(m_viewports);
    pipe.SetScissor(m_scissors);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    // Don't care about actual data, just need to get to draw to flag error
    static const float vbo_data[3] = {1.f, 0.f, 1.f};
    VkConstantBufferObj vbo(m_device, sizeof(vbo_data), sizeof(float), (const void *)&vbo_data);
    BindVertexBuffer(&vbo, (VkDeviceSize)0, 1); // VBO idx 1, but no VBO in PSO
    Draw(1, 0, 0, 0);

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, MismatchCountQueueCreateRequestedFeature) {
    TEST_DESCRIPTION("Use an invalid count in a vkEnumeratePhysicalDevices call."
                     "Use invalid Queue Family Index in vkCreateDevice");
    ASSERT_NO_FATAL_FAILURE(InitState());

    const char *mismatch_count_message = "Call to vkEnumeratePhysicalDevices() "
                                         "w/ pPhysicalDeviceCount value ";

    const char *invalid_queueFamilyIndex_message = "Invalid queue create request in vkCreateDevice(). Invalid "
                                                   "queueFamilyIndex ";

    const char *unavailable_feature_message = "While calling vkCreateDevice(), requesting feature #";

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, mismatch_count_message);
    // The following test fails with recent NVidia drivers.
    // By the time core_validation is reached, the NVidia
    // driver has sanitized the invalid condition and core_validation
    // is not introduced to the failure condition. This is not the case
    // with AMD and Mesa drivers. Futher investigation is required
    //    uint32_t count = static_cast<uint32_t>(~0);
    //    VkPhysicalDevice physical_device;
    //    vkEnumeratePhysicalDevices(instance(), &count, &physical_device);
    //    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_queueFamilyIndex_message);
    float queue_priority = 0.0;

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_info.queueFamilyIndex = static_cast<uint32_t>(~0);

    VkPhysicalDeviceFeatures features = m_device->phy().features();
    VkDevice testDevice;
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.pEnabledFeatures = &features;
    vkCreateDevice(gpu(), &device_create_info, nullptr, &testDevice);
    m_errorMonitor->VerifyFound();

    queue_create_info.queueFamilyIndex = 1;

    unsigned feature_count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    VkBool32 *feature_array = reinterpret_cast<VkBool32 *>(&features);
    for (unsigned i = 0; i < feature_count; i++) {
        if (VK_FALSE == feature_array[i]) {
            feature_array[i] = VK_TRUE;
            m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, unavailable_feature_message);
            device_create_info.pEnabledFeatures = &features;
            vkCreateDevice(gpu(), &device_create_info, nullptr, &testDevice);
            m_errorMonitor->VerifyFound();
            break;
        }
    }
}

TEST_F(VkLayerTest, InvalidQueueIndexInvalidQuery) {
    TEST_DESCRIPTION("Use an invalid queue index in a vkCmdWaitEvents call."
                     "End a command buffer with a query still in progress.");

    const char *invalid_queue_index = "was created with sharingMode of VK_SHARING_MODE_EXCLUSIVE. If one "
                                      "of src- or dstQueueFamilyIndex is VK_QUEUE_FAMILY_IGNORED, both "
                                      "must be.";

    const char *invalid_query = "Ending command buffer with in progress query: queryPool 0x";

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_queue_index);

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkEvent event;
    VkEventCreateInfo event_create_info{};
    event_create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    vkCreateEvent(m_device->device(), &event_create_info, nullptr, &event);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 0, &queue);

    BeginCommandBuffer();

    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());
    VkImageMemoryBarrier img_barrier = {};
    img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.pNext = NULL;
    img_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    img_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_barrier.image = image.handle();
    img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    // QueueFamilyIndex must be VK_QUEUE_FAMILY_IGNORED, this verifies
    // that layer validation catches the case when it is not.
    img_barrier.dstQueueFamilyIndex = 0;
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier.subresourceRange.baseArrayLayer = 0;
    img_barrier.subresourceRange.baseMipLevel = 0;
    img_barrier.subresourceRange.layerCount = 1;
    img_barrier.subresourceRange.levelCount = 1;
    vkCmdWaitEvents(m_commandBuffer->handle(), 1, &event, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                    nullptr, 0, nullptr, 1, &img_barrier);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_query);

    VkQueryPool query_pool;
    VkQueryPoolCreateInfo query_pool_create_info = {};
    query_pool_create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_create_info.queryType = VK_QUERY_TYPE_OCCLUSION;
    query_pool_create_info.queryCount = 1;
    vkCreateQueryPool(m_device->device(), &query_pool_create_info, nullptr, &query_pool);

    vkCmdResetQueryPool(m_commandBuffer->handle(), query_pool, 0 /*startQuery*/, 1 /*queryCount*/);
    vkCmdBeginQuery(m_commandBuffer->handle(), query_pool, 0, 0);

    vkEndCommandBuffer(m_commandBuffer->handle());
    m_errorMonitor->VerifyFound();

    vkDestroyQueryPool(m_device->device(), query_pool, nullptr);
    vkDestroyEvent(m_device->device(), event, nullptr);
}

TEST_F(VkLayerTest, VertexBufferInvalid) {
    TEST_DESCRIPTION("Submit a command buffer using deleted vertex buffer, "
                     "delete a buffer twice, use an invalid offset for each "
                     "buffer type, and attempt to bind a null buffer");

    const char *deleted_buffer_in_command_buffer = "Cannot submit cmd buffer "
                                                   "using deleted buffer ";
    const char *double_destroy_message = "Cannot free buffer 0x";
    const char *invalid_offset_message = "vkBindBufferMemory(): "
                                         "memoryOffset is 0x";
    const char *invalid_storage_buffer_offset_message = "vkBindBufferMemory(): "
                                                        "storage memoryOffset "
                                                        "is 0x";
    const char *invalid_texel_buffer_offset_message = "vkBindBufferMemory(): "
                                                      "texel memoryOffset "
                                                      "is 0x";
    const char *invalid_uniform_buffer_offset_message = "vkBindBufferMemory(): "
                                                        "uniform memoryOffset "
                                                        "is 0x";
    const char *bind_null_buffer_message = "In vkBindBufferMemory, attempting"
                                           " to Bind Obj(0x";
    const char *free_invalid_buffer_message = "Invalid Device Memory Object 0x";

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
    pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipe_ms_state_ci.pNext = NULL;
    pipe_ms_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipe_ms_state_ci.sampleShadingEnable = 0;
    pipe_ms_state_ci.minSampleShading = 1.0;
    pipe_ms_state_ci.pSampleMask = nullptr;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout pipeline_layout;

    VkResult err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, nullptr, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.SetViewport(m_viewports);
    pipe.SetScissor(m_scissors);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());

    {
        // Create and bind a vertex buffer in a reduced scope, which will cause
        // it to be deleted upon leaving this scope
        const float vbo_data[3] = {1.f, 0.f, 1.f};
        VkVerticesObj draw_verticies(m_device, 1, 1, sizeof(vbo_data), 3, vbo_data);
        draw_verticies.BindVertexBuffers(m_commandBuffer->handle());
        draw_verticies.AddVertexInputToPipe(pipe);
    }

    Draw(1, 0, 0, 0);

    EndCommandBuffer();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, deleted_buffer_in_command_buffer);
    QueueCommandBuffer(false);
    m_errorMonitor->VerifyFound();

    {
        // Create and bind a vertex buffer in a reduced scope, and delete it
        // twice, the second through the destructor
        VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VkBufferTest::eDoubleDelete);
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, double_destroy_message);
        buffer_test.TestDoubleDestroy();
    }
    m_errorMonitor->VerifyFound();

    if (VkBufferTest::GetTestConditionValid(m_device, VkBufferTest::eInvalidMemoryOffset)) {
        // Create and bind a memory buffer with an invalid offset.
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_offset_message);
        VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VkBufferTest::eInvalidMemoryOffset);
        (void)buffer_test;
        m_errorMonitor->VerifyFound();
    }

    if (VkBufferTest::GetTestConditionValid(m_device, VkBufferTest::eInvalidDeviceOffset,
                                            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)) {
        // Create and bind a memory buffer with an invalid offset again,
        // but look for a texel buffer message.
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_texel_buffer_offset_message);
        VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VkBufferTest::eInvalidDeviceOffset);
        (void)buffer_test;
        m_errorMonitor->VerifyFound();
    }

    if (VkBufferTest::GetTestConditionValid(m_device, VkBufferTest::eInvalidDeviceOffset, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
        // Create and bind a memory buffer with an invalid offset again, but
        // look for a uniform buffer message.
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_uniform_buffer_offset_message);
        VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VkBufferTest::eInvalidDeviceOffset);
        (void)buffer_test;
        m_errorMonitor->VerifyFound();
    }

    if (VkBufferTest::GetTestConditionValid(m_device, VkBufferTest::eInvalidDeviceOffset, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        // Create and bind a memory buffer with an invalid offset again, but
        // look for a storage buffer message.
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, invalid_storage_buffer_offset_message);
        VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VkBufferTest::eInvalidDeviceOffset);
        (void)buffer_test;
        m_errorMonitor->VerifyFound();
    }

    {
        // Attempt to bind a null buffer.
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, bind_null_buffer_message);
        VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VkBufferTest::eBindNullBuffer);
        (void)buffer_test;
        m_errorMonitor->VerifyFound();
    }

    {
        // Attempt to use an invalid handle to delete a buffer.
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, free_invalid_buffer_message);
        VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VkBufferTest::eFreeInvalidHandle);
        (void)buffer_test;
    }
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
}

// INVALID_IMAGE_LAYOUT tests (one other case is hit by MapMemWithoutHostVisibleBit and not here)
TEST_F(VkLayerTest, InvalidImageLayout) {
    TEST_DESCRIPTION("Hit all possible validation checks associated with the "
                     "DRAWSTATE_INVALID_IMAGE_LAYOUT enum. Generally these involve having"
                     "images in the wrong layout when they're copied or transitioned.");
    // 3 in ValidateCmdBufImageLayouts
    // *  -1 Attempt to submit cmd buf w/ deleted image
    // *  -2 Cmd buf submit of image w/ layout not matching first use w/ subresource
    // *  -3 Cmd buf submit of image w/ layout not matching first use w/o subresource
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                         "Layout for input image should be TRANSFER_SRC_OPTIMAL instead of GENERAL.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Create src & dst images to use for copy operations
    VkImage src_image;
    VkImage dst_image;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 4;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.flags = 0;

    VkResult err = vkCreateImage(m_device->device(), &image_create_info, NULL, &src_image);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dst_image);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    VkImageCopy copyRegion;
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset.x = 0;
    copyRegion.srcOffset.y = 0;
    copyRegion.srcOffset.z = 0;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset.x = 0;
    copyRegion.dstOffset.y = 0;
    copyRegion.dstOffset.z = 0;
    copyRegion.extent.width = 1;
    copyRegion.extent.height = 1;
    copyRegion.extent.depth = 1;
    m_commandBuffer->CopyImage(src_image, VK_IMAGE_LAYOUT_GENERAL, dst_image, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    m_errorMonitor->VerifyFound();
    // Now cause error due to src image layout changing
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot copy from an image whose source layout is "
                                                                        "VK_IMAGE_LAYOUT_UNDEFINED and doesn't match the current "
                                                                        "layout VK_IMAGE_LAYOUT_GENERAL.");
    m_commandBuffer->CopyImage(src_image, VK_IMAGE_LAYOUT_UNDEFINED, dst_image, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    m_errorMonitor->VerifyFound();
    // Final src error is due to bad layout type
    m_errorMonitor->SetDesiredFailureMsg(
        VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "Layout for input image is VK_IMAGE_LAYOUT_UNDEFINED but can only be TRANSFER_SRC_OPTIMAL or GENERAL.");
    m_commandBuffer->CopyImage(src_image, VK_IMAGE_LAYOUT_UNDEFINED, dst_image, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    m_errorMonitor->VerifyFound();
    // Now verify same checks for dst
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                         "Layout for output image should be TRANSFER_DST_OPTIMAL instead of GENERAL.");
    m_commandBuffer->CopyImage(src_image, VK_IMAGE_LAYOUT_GENERAL, dst_image, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    m_errorMonitor->VerifyFound();
    // Now cause error due to src image layout changing
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot copy from an image whose dest layout is "
                                                                        "VK_IMAGE_LAYOUT_UNDEFINED and doesn't match the current "
                                                                        "layout VK_IMAGE_LAYOUT_GENERAL.");
    m_commandBuffer->CopyImage(src_image, VK_IMAGE_LAYOUT_GENERAL, dst_image, VK_IMAGE_LAYOUT_UNDEFINED, 1, &copyRegion);
    m_errorMonitor->VerifyFound();
    m_errorMonitor->SetDesiredFailureMsg(
        VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "Layout for output image is VK_IMAGE_LAYOUT_UNDEFINED but can only be TRANSFER_DST_OPTIMAL or GENERAL.");
    m_commandBuffer->CopyImage(src_image, VK_IMAGE_LAYOUT_GENERAL, dst_image, VK_IMAGE_LAYOUT_UNDEFINED, 1, &copyRegion);
    m_errorMonitor->VerifyFound();
    // Now cause error due to bad image layout transition in PipelineBarrier
    VkImageMemoryBarrier image_barrier[1] = {};
    image_barrier[0].oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    image_barrier[0].image = src_image;
    image_barrier[0].subresourceRange.layerCount = 2;
    image_barrier[0].subresourceRange.levelCount = 2;
    image_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "You cannot transition the layout from "
                                                                        "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL when "
                                                                        "current layout is VK_IMAGE_LAYOUT_GENERAL.");
    vkCmdPipelineBarrier(m_commandBuffer->handle(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                         NULL, 0, NULL, 1, image_barrier);
    m_errorMonitor->VerifyFound();

    // Finally some layout errors at RenderPass create time
    // Just hacking in specific state to get to the errors we want so don't copy this unless you know what you're doing.
    VkAttachmentReference attach = {};
    // perf warning for GENERAL layout w/ non-DS input attachment
    attach.layout = VK_IMAGE_LAYOUT_GENERAL;
    VkSubpassDescription subpass = {};
    subpass.inputAttachmentCount = 1;
    subpass.pInputAttachments = &attach;
    VkRenderPassCreateInfo rpci = {};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.attachmentCount = 1;
    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_UNDEFINED;
    rpci.pAttachments = &attach_desc;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkRenderPass rp;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                         "Layout for input attachment is GENERAL but should be READ_ONLY_OPTIMAL.");
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();
    // error w/ non-general layout
    attach.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    m_errorMonitor->SetDesiredFailureMsg(
        VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "Layout for input attachment is VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL but can only be READ_ONLY_OPTIMAL or GENERAL.");
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();
    subpass.inputAttachmentCount = 0;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attach;
    attach.layout = VK_IMAGE_LAYOUT_GENERAL;
    // perf warning for GENERAL layout on color attachment
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                         "Layout for color attachment is GENERAL but should be COLOR_ATTACHMENT_OPTIMAL.");
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();
    // error w/ non-color opt or GENERAL layout for color attachment
    attach.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    m_errorMonitor->SetDesiredFailureMsg(
        VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "Layout for color attachment is VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL but can only be COLOR_ATTACHMENT_OPTIMAL or GENERAL.");
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &attach;
    attach.layout = VK_IMAGE_LAYOUT_GENERAL;
    // perf warning for GENERAL layout on DS attachment
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                         "GENERAL layout for depth attachment may not give optimal performance.");
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();
    // error w/ non-ds opt or GENERAL layout for color attachment
    attach.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Layout for depth attachment is VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL but can only be "
                                         "DEPTH_STENCIL_ATTACHMENT_OPTIMAL, DEPTH_STENCIL_READ_ONLY_OPTIMAL or GENERAL.");
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();
    // For this error we need a valid renderpass so create default one
    attach.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    attach.attachment = 0;
    attach_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attach_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attach_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attach_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Can't do a CLEAR load on READ_ONLY initialLayout
    attach_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    attach_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, " with invalid first layout "
                                                                        "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_"
                                                                        "ONLY_OPTIMAL");
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), src_image, NULL);
    vkDestroyImage(m_device->device(), dst_image, NULL);
}

TEST_F(VkLayerTest, InvalidStorageImageLayout) {
    TEST_DESCRIPTION("Attempt to update a STORAGE_IMAGE descriptor w/o GENERAL layout.");
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());

    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageTiling tiling;
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(gpu(), tex_format, &format_properties);
    if (format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
        tiling = VK_IMAGE_TILING_LINEAR;
    } else if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
        tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        printf("Device does not support VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT; "
               "skipped.\n");
        return;
    }

    VkDescriptorPoolSize ds_type = {};
    ds_type.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    ds_type.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type;
    ds_pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    VkDescriptorSet descriptor_set;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;
    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkImageObj image(m_device);
    image.init(32, 32, tex_format, VK_IMAGE_USAGE_STORAGE_BIT, tiling, 0);
    ASSERT_TRUE(image.initialized());
    VkImageView view = image.targetView(tex_format);

    VkDescriptorImageInfo image_info = {};
    image_info.imageView = view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptor_write.pImageInfo = &image_info;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " of VK_DESCRIPTOR_TYPE_STORAGE_IMAGE type is being updated with layout "
                                         "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL but according to spec ");
    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkFreeDescriptorSets(m_device->device(), ds_pool, 1, &descriptor_set);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, SimultaneousUse) {
    TEST_DESCRIPTION("Use vkCmdExecuteCommands with invalid state "
                     "in primary and secondary command buffers.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *simultaneous_use_message1 = "without VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set!";
    const char *simultaneous_use_message2 = "does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set and "
                                            "will cause primary command buffer";

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = m_commandPool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer secondary_command_buffer;
    ASSERT_VK_SUCCESS(vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, &secondary_command_buffer));
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = {};
    command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    command_buffer_inheritance_info.renderPass = m_renderPass;
    command_buffer_inheritance_info.framebuffer = m_framebuffer;
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags =
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    command_buffer_begin_info.pInheritanceInfo = &command_buffer_inheritance_info;

    vkBeginCommandBuffer(secondary_command_buffer, &command_buffer_begin_info);
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &renderPassBeginInfo(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdExecuteCommands(m_commandBuffer->handle(), 1, &secondary_command_buffer);
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle());
    vkEndCommandBuffer(secondary_command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    vkBeginCommandBuffer(m_commandBuffer->handle(), &command_buffer_begin_info);
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &renderPassBeginInfo(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, simultaneous_use_message1);
    vkCmdExecuteCommands(m_commandBuffer->handle(), 1, &secondary_command_buffer);
    m_errorMonitor->VerifyFound();
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle());
    vkEndCommandBuffer(m_commandBuffer->handle());

    m_errorMonitor->SetDesiredFailureMsg(0, "");
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(m_commandBuffer->handle(), &command_buffer_begin_info);
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &renderPassBeginInfo(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, simultaneous_use_message2);
    vkCmdExecuteCommands(m_commandBuffer->handle(), 1, &secondary_command_buffer);
    m_errorMonitor->VerifyFound();
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle());
    vkEndCommandBuffer(m_commandBuffer->handle());
}

TEST_F(VkLayerTest, InUseDestroyedSignaled) {
    TEST_DESCRIPTION("Use vkCmdExecuteCommands with invalid state "
                     "in primary and secondary command buffers. "
                     "Delete objects that are inuse. Call VkQueueSubmit "
                     "with an event that has been deleted.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *submit_with_deleted_event_message = "Cannot submit cmd buffer using deleted event 0x";
    const char *cannot_delete_event_message = "Cannot delete event 0x";
    const char *cannot_delete_semaphore_message = "Cannot delete semaphore 0x";
    const char *cannot_destroy_fence_message = "Fence 0x";

    BeginCommandBuffer();

    VkEvent event;
    VkEventCreateInfo event_create_info = {};
    event_create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    vkCreateEvent(m_device->device(), &event_create_info, nullptr, &event);
    vkCmdSetEvent(m_commandBuffer->handle(), event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    EndCommandBuffer();
    vkDestroyEvent(m_device->device(), event, nullptr);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, submit_with_deleted_event_message);
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(0, "");
    vkResetCommandBuffer(m_commandBuffer->handle(), 0);

    vkCreateEvent(m_device->device(), &event_create_info, nullptr, &event);

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore semaphore;
    ASSERT_VK_SUCCESS(vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore));
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    ASSERT_VK_SUCCESS(vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence));

    VkDescriptorPoolSize descriptor_pool_type_count = {};
    descriptor_pool_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &descriptor_pool_type_count;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool descriptorset_pool;
    ASSERT_VK_SUCCESS(vkCreateDescriptorPool(m_device->device(), &descriptor_pool_create_info, nullptr, &descriptorset_pool));

    VkDescriptorSetLayoutBinding descriptorset_layout_binding = {};
    descriptorset_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorset_layout_binding.descriptorCount = 1;
    descriptorset_layout_binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info = {};
    descriptorset_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorset_layout_create_info.bindingCount = 1;
    descriptorset_layout_create_info.pBindings = &descriptorset_layout_binding;

    VkDescriptorSetLayout descriptorset_layout;
    ASSERT_VK_SUCCESS(
        vkCreateDescriptorSetLayout(m_device->device(), &descriptorset_layout_create_info, nullptr, &descriptorset_layout));

    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo descriptorset_allocate_info = {};
    descriptorset_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorset_allocate_info.descriptorSetCount = 1;
    descriptorset_allocate_info.descriptorPool = descriptorset_pool;
    descriptorset_allocate_info.pSetLayouts = &descriptorset_layout;
    ASSERT_VK_SUCCESS(vkAllocateDescriptorSets(m_device->device(), &descriptorset_allocate_info, &descriptorset));

    VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = buffer_test.GetBuffer();
    buffer_info.offset = 0;
    buffer_info.range = 1024;

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptorset;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &write_descriptor_set, 0, nullptr);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptorset_layout;

    VkPipelineLayout pipeline_layout;
    ASSERT_VK_SUCCESS(vkCreatePipelineLayout(m_device->device(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

    pipe.CreateVKPipeline(pipeline_layout, m_renderPass);

    BeginCommandBuffer();
    vkCmdSetEvent(m_commandBuffer->handle(), event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vkCmdBindPipeline(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptorset, 0, NULL);

    EndCommandBuffer();

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &semaphore;
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, fence);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, cannot_delete_event_message);
    vkDestroyEvent(m_device->device(), event, nullptr);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, cannot_delete_semaphore_message);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, cannot_destroy_fence_message);
    vkDestroyFence(m_device->device(), fence, nullptr);
    m_errorMonitor->VerifyFound();

    vkQueueWaitIdle(m_device->m_queue);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
    vkDestroyFence(m_device->device(), fence, nullptr);
    vkDestroyEvent(m_device->device(), event, nullptr);
    vkDestroyDescriptorPool(m_device->device(), descriptorset_pool, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), descriptorset_layout, nullptr);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, nullptr);
}

TEST_F(VkLayerTest, QueryPoolInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete in-use query pool.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkQueryPool query_pool;
    VkQueryPoolCreateInfo query_pool_ci{};
    query_pool_ci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_ci.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_ci.queryCount = 1;
    vkCreateQueryPool(m_device->device(), &query_pool_ci, nullptr, &query_pool);
    BeginCommandBuffer();
    // Reset query pool to create binding with cmd buffer
    vkCmdResetQueryPool(m_commandBuffer->handle(), query_pool, 0, 1);

    EndCommandBuffer();

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    // Submit cmd buffer and then destroy query pool while in-flight
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete query pool 0x");
    vkDestroyQueryPool(m_device->handle(), query_pool, NULL);
    m_errorMonitor->VerifyFound();

    vkQueueWaitIdle(m_device->m_queue);
    // Now that cmd buffer done we can safely destroy query_pool
    vkDestroyQueryPool(m_device->handle(), query_pool, NULL);
}

TEST_F(VkLayerTest, PipelineInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete in-use pipeline.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Empty pipeline layout used for binding PSO
    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 0;
    pipeline_layout_ci.pSetLayouts = NULL;

    VkPipelineLayout pipeline_layout;
    VkResult err = vkCreatePipelineLayout(m_device->handle(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete pipeline 0x");
    // Create PSO to be used for draw-time errors below
    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    // Store pipeline handle so we can actually delete it before test finishes
    VkPipeline delete_this_pipeline;
    { // Scope pipeline so it will be auto-deleted
        VkPipelineObj pipe(m_device);
        pipe.AddShader(&vs);
        pipe.AddShader(&fs);
        pipe.AddColorAttachment();
        pipe.CreateVKPipeline(pipeline_layout, renderPass());
        delete_this_pipeline = pipe.handle();

        BeginCommandBuffer();
        // Bind pipeline to cmd buffer
        vkCmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());

        EndCommandBuffer();

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_commandBuffer->handle();
        // Submit cmd buffer and then pipeline destroyed while in-flight
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    } // Pipeline deletion triggered here
    m_errorMonitor->VerifyFound();
    // Make sure queue finished and then actually delete pipeline
    vkQueueWaitIdle(m_device->m_queue);
    vkDestroyPipeline(m_device->handle(), delete_this_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device->handle(), pipeline_layout, nullptr);
}

TEST_F(VkLayerTest, ImageViewInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete in-use imageView.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count;
    ds_type_count.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;

    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding layout_binding;
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &layout_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    VkDescriptorSet descriptor_set;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageView view;
    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = image.handle();
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R8G8B8A8_UNORM;
    ivci.subresourceRange.layerCount = 1;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    err = vkCreateImageView(m_device->device(), &ivci, NULL, &view);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = view;
    image_info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    // Create PSO to use the sampler
    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0) uniform sampler2D s;\n"
                           "layout(location=0) out vec4 x;\n"
                           "void main(){\n"
                           "   x = texture(s, vec2(1));\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete image view 0x");

    BeginCommandBuffer();
    // Bind pipeline to cmd buffer
    vkCmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptor_set, 0, nullptr);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();
    // Submit cmd buffer then destroy sampler
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    // Submit cmd buffer and then destroy imageView while in-flight
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    vkDestroyImageView(m_device->device(), view, nullptr);
    m_errorMonitor->VerifyFound();
    vkQueueWaitIdle(m_device->m_queue);
    // Now we can actually destroy imageView
    vkDestroyImageView(m_device->device(), view, NULL);
    vkDestroySampler(m_device->device(), sampler, nullptr);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, BufferViewInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete in-use bufferView.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count;
    ds_type_count.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding layout_binding;
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &layout_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    VkDescriptorSet descriptor_set;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkBuffer buffer;
    uint32_t queue_family_index = 0;
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = 1024;
    buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    buffer_create_info.queueFamilyIndexCount = 1;
    buffer_create_info.pQueueFamilyIndices = &queue_family_index;

    err = vkCreateBuffer(m_device->device(), &buffer_create_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    VkMemoryRequirements memory_reqs;
    VkDeviceMemory buffer_memory;

    VkMemoryAllocateInfo memory_info = {};
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.allocationSize = 0;
    memory_info.memoryTypeIndex = 0;

    vkGetBufferMemoryRequirements(m_device->device(), buffer, &memory_reqs);
    memory_info.allocationSize = memory_reqs.size;
    bool pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
    ASSERT_TRUE(pass);

    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &buffer_memory);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, buffer_memory, 0);
    ASSERT_VK_SUCCESS(err);

    VkBufferView view;
    VkBufferViewCreateInfo bvci = {};
    bvci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bvci.buffer = buffer;
    bvci.format = VK_FORMAT_R8_UNORM;
    bvci.range = VK_WHOLE_SIZE;

    err = vkCreateBufferView(m_device->device(), &bvci, NULL, &view);
    ASSERT_VK_SUCCESS(err);

    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    descriptor_write.pTexelBufferView = &view;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0, r8) uniform imageBuffer s;\n"
                           "layout(location=0) out vec4 x;\n"
                           "void main(){\n"
                           "   x = imageLoad(s, 0);\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete buffer view 0x");

    BeginCommandBuffer();
    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    vkCmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    VkRect2D scissor = {{0, 0}, {16, 16}};
    vkCmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    // Bind pipeline to cmd buffer
    vkCmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptor_set, 0, nullptr);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    // Submit cmd buffer and then destroy bufferView while in-flight
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    vkDestroyBufferView(m_device->device(), view, nullptr);
    m_errorMonitor->VerifyFound();
    vkQueueWaitIdle(m_device->m_queue);
    // Now we can actually destroy bufferView
    vkDestroyBufferView(m_device->device(), view, NULL);
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkFreeMemory(m_device->device(), buffer_memory, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, SamplerInUseDestroyedSignaled) {
    TEST_DESCRIPTION("Delete in-use sampler.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorPoolSize ds_type_count;
    ds_type_count.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    VkResult err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext = NULL;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias = 1.0;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
    sampler_ci.minLod = 1.0;
    sampler_ci.maxLod = 1.0;
    sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;

    err = vkCreateSampler(m_device->device(), &sampler_ci, NULL, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding layout_binding;
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &layout_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    VkDescriptorSet descriptor_set;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = NULL;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageView view;
    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = image.handle();
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R8G8B8A8_UNORM;
    ivci.subresourceRange.layerCount = 1;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    err = vkCreateImageView(m_device->device(), &ivci, NULL, &view);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = view;
    image_info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    // Create PSO to use the sampler
    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { \n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0) uniform sampler2D s;\n"
                           "layout(location=0) out vec4 x;\n"
                           "void main(){\n"
                           "   x = texture(s, vec2(1));\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Cannot delete sampler 0x");

    BeginCommandBuffer();
    // Bind pipeline to cmd buffer
    vkCmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &descriptor_set, 0, nullptr);
    Draw(1, 0, 0, 0);
    EndCommandBuffer();
    // Submit cmd buffer then destroy sampler
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    // Submit cmd buffer and then destroy sampler while in-flight
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);

    vkDestroySampler(m_device->device(), sampler, nullptr);
    m_errorMonitor->VerifyFound();
    vkQueueWaitIdle(m_device->m_queue);
    // Now we can actually destroy sampler
    vkDestroySampler(m_device->device(), sampler, nullptr);
    vkDestroyImageView(m_device->device(), view, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, QueueForwardProgressFenceWait) {
    TEST_DESCRIPTION("Call VkQueueSubmit with a semaphore that is already "
                     "signaled but not waited on by the queue. Wait on a "
                     "fence that has not yet been submitted to a queue.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *queue_forward_progress_message = " that has already been signaled but not waited on by queue 0x";
    const char *invalid_fence_wait_message = " which has not been submitted on a Queue or during "
                                             "acquire next image.";

    BeginCommandBuffer();
    EndCommandBuffer();

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore semaphore;
    ASSERT_VK_SUCCESS(vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore));
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffer->handle();
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &semaphore;
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->SetDesiredFailureMsg(0, "");
    vkResetCommandBuffer(m_commandBuffer->handle(), 0);
    BeginCommandBuffer();
    EndCommandBuffer();
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, queue_forward_progress_message);
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    ASSERT_VK_SUCCESS(vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence));

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, invalid_fence_wait_message);
    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    m_errorMonitor->VerifyFound();

    vkDeviceWaitIdle(m_device->device());
    vkDestroyFence(m_device->device(), fence, nullptr);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
}

TEST_F(VkLayerTest, FramebufferIncompatible) {
    TEST_DESCRIPTION("Bind a secondary command buffer with with a framebuffer "
                     "that does not match the framebuffer for the active "
                     "renderpass.");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // A renderpass with one color attachment.
    VkAttachmentDescription attachment = {0,
                                          VK_FORMAT_B8G8R8A8_UNORM,
                                          VK_SAMPLE_COUNT_1_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                          VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                          VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference att_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &att_ref, nullptr, nullptr, 0, nullptr};

    VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &attachment, 1, &subpass, 0, nullptr};

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    // A compatible framebuffer.
    VkImageObj image(m_device);
    image.init(32, 32, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageViewCreateInfo ivci = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        image.handle(),
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_B8G8R8A8_UNORM,
        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
         VK_COMPONENT_SWIZZLE_IDENTITY},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    VkImageView view;
    err = vkCreateImageView(m_device->device(), &ivci, nullptr, &view);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, rp, 1, &view, 32, 32, 1};
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = m_commandPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    cbai.commandBufferCount = 1;

    VkCommandBuffer sec_cb;
    err = vkAllocateCommandBuffers(m_device->device(), &cbai, &sec_cb);
    ASSERT_VK_SUCCESS(err);
    VkCommandBufferBeginInfo cbbi = {};
    VkCommandBufferInheritanceInfo cbii = {};
    cbii.renderPass = renderPass();
    cbii.framebuffer = fb;
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.pNext = NULL;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cbbi.pInheritanceInfo = &cbii;
    vkBeginCommandBuffer(sec_cb, &cbbi);
    vkEndCommandBuffer(sec_cb);

    VkCommandBufferBeginInfo cbbi2 = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        0, nullptr
    };
    vkBeginCommandBuffer(m_commandBuffer->GetBufferHandle(), &cbbi2);
    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), &m_renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " that is not the same as the primary command buffer's current active framebuffer ");
    vkCmdExecuteCommands(m_commandBuffer->GetBufferHandle(), 1, &sec_cb);
    m_errorMonitor->VerifyFound();
    // Cleanup
    vkDestroyImageView(m_device->device(), view, NULL);
    vkDestroyRenderPass(m_device->device(), rp, NULL);
    vkDestroyFramebuffer(m_device->device(), fb, NULL);
}

TEST_F(VkLayerTest, ColorBlendLogicOpTests) {
    TEST_DESCRIPTION("If logicOp is available on the device, set it to an "
                     "invalid value. If logicOp is not available, attempt to "
                     "use it and verify that we see the correct error.");
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    auto features = m_device->phy().features();
    // Set the expected error depending on whether or not logicOp available
    if (VK_FALSE == features.logicOp) {
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "If logic operations feature not "
                                                                            "enabled, logicOpEnable must be "
                                                                            "VK_FALSE");
    } else {
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "pColorBlendState->logicOp (16)");
    }
    // Create a pipeline using logicOp
    VkResult err;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineViewportStateCreateInfo vp_state_ci = {};
    vp_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_ci.viewportCount = 1;
    VkViewport vp = {}; // Just need dummy vp to point to
    vp_state_ci.pViewports = &vp;
    vp_state_ci.scissorCount = 1;
    VkRect2D scissors = {}; // Dummy scissors to point to
    vp_state_ci.pScissors = &scissors;

    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);
    shaderStages[0] = vs.GetStageCreateInfo();
    shaderStages[1] = fs.GetStageCreateInfo();

    VkPipelineVertexInputStateCreateInfo vi_ci = {};
    vi_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia_ci = {};
    ia_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState att = {};
    att.blendEnable = VK_FALSE;
    att.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo cb_ci = {};
    cb_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // Enable logicOp & set logicOp to value 1 beyond allowed entries
    cb_ci.logicOpEnable = VK_TRUE;
    cb_ci.logicOp = VK_LOGIC_OP_RANGE_SIZE; // This should cause an error
    cb_ci.attachmentCount = 1;
    cb_ci.pAttachments = &att;

    VkPipelineMultisampleStateCreateInfo ms_ci = {};
    ms_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkGraphicsPipelineCreateInfo gp_ci = {};
    gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_ci.stageCount = 2;
    gp_ci.pStages = shaderStages;
    gp_ci.pVertexInputState = &vi_ci;
    gp_ci.pInputAssemblyState = &ia_ci;
    gp_ci.pViewportState = &vp_state_ci;
    gp_ci.pRasterizationState = &rs_ci;
    gp_ci.pColorBlendState = &cb_ci;
    gp_ci.pMultisampleState = &ms_ci;
    gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    gp_ci.layout = pipeline_layout;
    gp_ci.renderPass = renderPass();

    VkPipelineCacheCreateInfo pc_ci = {};
    pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;
    err = vkCreatePipelineCache(m_device->device(), &pc_ci, NULL, &pipelineCache);
    ASSERT_VK_SUCCESS(err);

    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, NULL, &pipeline);
    m_errorMonitor->VerifyFound();
    if (VK_SUCCESS == err) {
        vkDestroyPipeline(m_device->device(), pipeline, NULL);
    }
    vkDestroyPipelineCache(m_device->device(), pipelineCache, NULL);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
}
#endif // DRAW_STATE_TESTS

#if THREADING_TESTS
#if GTEST_IS_THREADSAFE
struct thread_data_struct {
    VkCommandBuffer commandBuffer;
    VkEvent event;
    bool bailout;
};

extern "C" void *AddToCommandBuffer(void *arg) {
    struct thread_data_struct *data = (struct thread_data_struct *)arg;

    for (int i = 0; i < 80000; i++) {
        vkCmdSetEvent(data->commandBuffer, data->event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        if (data->bailout) {
            break;
        }
    }
    return NULL;
}

TEST_F(VkLayerTest, ThreadCommandBufferCollision) {
    test_platform_thread thread;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "THREADING ERROR");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Calls AllocateCommandBuffers
    VkCommandBufferObj commandBuffer(m_device, m_commandPool);

    // Avoid creating RenderPass
    commandBuffer.BeginCommandBuffer();

    VkEventCreateInfo event_info;
    VkEvent event;
    VkResult err;

    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = vkCreateEvent(device(), &event_info, NULL, &event);
    ASSERT_VK_SUCCESS(err);

    err = vkResetEvent(device(), event);
    ASSERT_VK_SUCCESS(err);

    struct thread_data_struct data;
    data.commandBuffer = commandBuffer.GetBufferHandle();
    data.event = event;
    data.bailout = false;
    m_errorMonitor->SetBailout(&data.bailout);

    // First do some correct operations using multiple threads.
    // Add many entries to command buffer from another thread.
    test_platform_thread_create(&thread, AddToCommandBuffer, (void *)&data);
    // Make non-conflicting calls from this thread at the same time.
    for (int i = 0; i < 80000; i++) {
        uint32_t count;
        vkEnumeratePhysicalDevices(instance(), &count, NULL);
    }
    test_platform_thread_join(thread, NULL);

    // Then do some incorrect operations using multiple threads.
    // Add many entries to command buffer from another thread.
    test_platform_thread_create(&thread, AddToCommandBuffer, (void *)&data);
    // Add many entries to command buffer from this thread at the same time.
    AddToCommandBuffer(&data);

    test_platform_thread_join(thread, NULL);
    commandBuffer.EndCommandBuffer();

    m_errorMonitor->SetBailout(NULL);

    m_errorMonitor->VerifyFound();

    vkDestroyEvent(device(), event, NULL);
}
#endif // GTEST_IS_THREADSAFE
#endif // THREADING_TESTS

#if SHADER_CHECKER_TESTS
TEST_F(VkLayerTest, InvalidSPIRVCodeSize) {
    TEST_DESCRIPTION("Test that an error is produced for a spirv module "
                     "with an impossible code size");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid SPIR-V header");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    struct icd_spv_header spv;

    spv.magic = ICD_SPV_MAGIC;
    spv.version = ICD_SPV_VERSION;
    spv.gen_magic = 0;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.pCode = (const uint32_t *)&spv;
    moduleCreateInfo.codeSize = 4;
    moduleCreateInfo.flags = 0;
    vkCreateShaderModule(m_device->device(), &moduleCreateInfo, NULL, &module);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidSPIRVMagic) {
    TEST_DESCRIPTION("Test that an error is produced for a spirv module "
                     "with a bad magic number");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Invalid SPIR-V magic number");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    struct icd_spv_header spv;

    spv.magic = ~ICD_SPV_MAGIC;
    spv.version = ICD_SPV_VERSION;
    spv.gen_magic = 0;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.pCode = (const uint32_t *)&spv;
    moduleCreateInfo.codeSize = sizeof(spv) + 10;
    moduleCreateInfo.flags = 0;
    vkCreateShaderModule(m_device->device(), &moduleCreateInfo, NULL, &module);

    m_errorMonitor->VerifyFound();
}

#if 0
// Not currently covered by SPIRV-Tools validator
TEST_F(VkLayerTest, InvalidSPIRVVersion) {
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Invalid SPIR-V header");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    struct icd_spv_header spv;

    spv.magic = ICD_SPV_MAGIC;
    spv.version = ~ICD_SPV_VERSION;
    spv.gen_magic = 0;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;

    moduleCreateInfo.pCode = (const uint32_t *)&spv;
    moduleCreateInfo.codeSize = sizeof(spv) + 10;
    moduleCreateInfo.flags = 0;
    vkCreateShaderModule(m_device->device(), &moduleCreateInfo, NULL, &module);

    m_errorMonitor->VerifyFound();
}
#endif

TEST_F(VkLayerTest, CreatePipelineVertexOutputNotConsumed) {
    TEST_DESCRIPTION("Test that a warning is produced for a vertex output that "
                     "is not consumed by the fragment stage");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, "not consumed by fragment shader");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out float x;\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "   x = 0;\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineCheckShaderBadSpecialization) {
    TEST_DESCRIPTION("Challenge core_validation with shader validation issues related to vkCreateGraphicsPipelines.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *bad_specialization_message =
            "Specialization entry 0 (for constant id 0) references memory outside provided specialization data ";

    char const *vsSource =
            "#version 450\n"
            "\n"
            "out gl_PerVertex {\n"
            "    vec4 gl_Position;\n"
            "};\n"
            "void main(){\n"
            "   gl_Position = vec4(1);\n"
            "}\n";

    char const *fsSource =
            "#version 450\n"
            "\n"
            "layout (constant_id = 0) const float r = 0.0f;\n"
            "layout(location = 0) out vec4 uFragColor;\n"
            "void main(){\n"
            "   uFragColor = vec4(r,1,0,1);\n"
            "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPipelineLayout pipeline_layout;
    ASSERT_VK_SUCCESS(vkCreatePipelineLayout(m_device->device(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

    VkPipelineViewportStateCreateInfo vp_state_create_info = {};
    vp_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_create_info.viewportCount = 1;
    VkViewport viewport = {};
    vp_state_create_info.pViewports = &viewport;
    vp_state_create_info.scissorCount = 1;
    VkRect2D scissors = {};
    vp_state_create_info.pScissors = &scissors;

    VkDynamicState scissor_state = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {};
    pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline_dynamic_state_create_info.dynamicStateCount = 1;
    pipeline_dynamic_state_create_info.pDynamicStates = &scissor_state;

    VkPipelineShaderStageCreateInfo shader_stage_create_info[2] = {
        vs.GetStageCreateInfo(),
        fs.GetStageCreateInfo()
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.pNext = nullptr;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.rasterizerDiscardEnable = true;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

    VkGraphicsPipelineCreateInfo graphicspipe_create_info = {};
    graphicspipe_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicspipe_create_info.stageCount = 2;
    graphicspipe_create_info.pStages = shader_stage_create_info;
    graphicspipe_create_info.pVertexInputState = &vertex_input_create_info;
    graphicspipe_create_info.pInputAssemblyState = &input_assembly_create_info;
    graphicspipe_create_info.pViewportState = &vp_state_create_info;
    graphicspipe_create_info.pRasterizationState = &rasterization_state_create_info;
    graphicspipe_create_info.pColorBlendState = &color_blend_state_create_info;
    graphicspipe_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
    graphicspipe_create_info.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    graphicspipe_create_info.layout = pipeline_layout;
    graphicspipe_create_info.renderPass = renderPass();

    VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
    pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipelineCache pipelineCache;
    ASSERT_VK_SUCCESS(vkCreatePipelineCache(m_device->device(), &pipeline_cache_create_info, nullptr, &pipelineCache));

    // This structure maps constant ids to data locations.
    const VkSpecializationMapEntry entry =
        // id,  offset,                size
        {0, 4, sizeof(uint32_t)};  // Challenge core validation by using a bogus offset.

    uint32_t data = 1;

    // Set up the info describing spec map and data
    const VkSpecializationInfo specialization_info = {
        1,
        &entry,
        1 * sizeof(float),
        &data,
    };
    shader_stage_create_info[0].pSpecializationInfo = &specialization_info;

    VkPipeline pipeline;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, bad_specialization_message);
    vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &graphicspipe_create_info, nullptr, &pipeline);
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineCache(m_device->device(), pipelineCache, nullptr);
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineCheckShaderDescriptorTypeMismatch) {
    TEST_DESCRIPTION("Challenge core_validation with shader validation issues related to vkCreateGraphicsPipelines.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *descriptor_type_mismatch_message = "Type mismatch on descriptor slot 0.0 (used as type ";

    VkDescriptorPoolSize descriptor_pool_type_count[2] = {};
    descriptor_pool_type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_type_count[0].descriptorCount = 1;
    descriptor_pool_type_count[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_pool_type_count[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 2;
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_type_count;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool descriptorset_pool;
    ASSERT_VK_SUCCESS(vkCreateDescriptorPool(m_device->device(), &descriptor_pool_create_info, nullptr, &descriptorset_pool));

    VkDescriptorSetLayoutBinding descriptorset_layout_binding = {};
    descriptorset_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorset_layout_binding.descriptorCount = 1;
    descriptorset_layout_binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info = {};
    descriptorset_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorset_layout_create_info.bindingCount = 1;
    descriptorset_layout_create_info.pBindings = &descriptorset_layout_binding;

    VkDescriptorSetLayout descriptorset_layout;
    ASSERT_VK_SUCCESS(vkCreateDescriptorSetLayout(m_device->device(), &descriptorset_layout_create_info, nullptr, &descriptorset_layout));

    VkDescriptorSetAllocateInfo descriptorset_allocate_info = {};
    descriptorset_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorset_allocate_info.descriptorSetCount = 1;
    descriptorset_allocate_info.descriptorPool = descriptorset_pool;
    descriptorset_allocate_info.pSetLayouts = &descriptorset_layout;
    VkDescriptorSet descriptorset;
    ASSERT_VK_SUCCESS(vkAllocateDescriptorSets(m_device->device(), &descriptorset_allocate_info, &descriptorset));

    // Challenge core_validation with a non uniform buffer type.
    VkBufferTest storage_buffer_test(m_device, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    char const *vsSource =
            "#version 450\n"
            "\n"
            "layout (std140, set = 0, binding = 0) uniform buf {\n"
            "    mat4 mvp;\n"
            "} ubuf;\n"
            "out gl_PerVertex {\n"
            "    vec4 gl_Position;\n"
            "};\n"
            "void main(){\n"
            "   gl_Position = ubuf.mvp * vec4(1);\n"
            "}\n";

    char const *fsSource =
            "#version 450\n"
            "\n"
            "layout(location = 0) out vec4 uFragColor;\n"
            "void main(){\n"
            "   uFragColor = vec4(0,1,0,1);\n"
            "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptorset_layout;

    VkPipelineLayout pipeline_layout;
    ASSERT_VK_SUCCESS(vkCreatePipelineLayout(m_device->device(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, descriptor_type_mismatch_message);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, nullptr);
    vkDestroyDescriptorPool(m_device->device(), descriptorset_pool, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), descriptorset_layout, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineCheckShaderDescriptorNotAccessible) {
    TEST_DESCRIPTION(
        "Create a pipeline in which a descriptor used by a shader stage does not include that stage in its stageFlags.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *descriptor_not_accessible_message = "Shader uses descriptor slot 0.0 (used as type ";

    VkDescriptorPoolSize descriptor_pool_type_count = {};
    descriptor_pool_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &descriptor_pool_type_count;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool descriptorset_pool;
    ASSERT_VK_SUCCESS(vkCreateDescriptorPool(m_device->device(), &descriptor_pool_create_info, nullptr, &descriptorset_pool));

    VkDescriptorSetLayoutBinding descriptorset_layout_binding = {};
    descriptorset_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorset_layout_binding.descriptorCount = 1;
    // Intentionally make the uniform buffer inaccessible to the vertex shader to challenge core_validation
    descriptorset_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info = {};
    descriptorset_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorset_layout_create_info.bindingCount = 1;
    descriptorset_layout_create_info.pBindings = &descriptorset_layout_binding;

    VkDescriptorSetLayout descriptorset_layout;
    ASSERT_VK_SUCCESS(vkCreateDescriptorSetLayout(m_device->device(), &descriptorset_layout_create_info,
                                                  nullptr, &descriptorset_layout));

    VkDescriptorSetAllocateInfo descriptorset_allocate_info = {};
    descriptorset_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorset_allocate_info.descriptorSetCount = 1;
    descriptorset_allocate_info.descriptorPool = descriptorset_pool;
    descriptorset_allocate_info.pSetLayouts = &descriptorset_layout;
    VkDescriptorSet descriptorset;
    ASSERT_VK_SUCCESS(vkAllocateDescriptorSets(m_device->device(), &descriptorset_allocate_info, &descriptorset));

    VkBufferTest buffer_test(m_device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    char const *vsSource =
            "#version 450\n"
            "\n"
            "layout (std140, set = 0, binding = 0) uniform buf {\n"
            "    mat4 mvp;\n"
            "} ubuf;\n"
            "out gl_PerVertex {\n"
            "    vec4 gl_Position;\n"
            "};\n"
            "void main(){\n"
            "   gl_Position = ubuf.mvp * vec4(1);\n"
            "}\n";

    char const *fsSource =
            "#version 450\n"
            "\n"
            "layout(location = 0) out vec4 uFragColor;\n"
            "void main(){\n"
            "   uFragColor = vec4(0,1,0,1);\n"
            "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptorset_layout;

    VkPipelineLayout pipeline_layout;
    ASSERT_VK_SUCCESS(vkCreatePipelineLayout(m_device->device(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, descriptor_not_accessible_message);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, nullptr);
    vkDestroyDescriptorPool(m_device->device(), descriptorset_pool, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), descriptorset_layout, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineCheckShaderPushConstantNotAccessible) {
    TEST_DESCRIPTION("Create a graphics pipleine in which a push constant range containing a push constant block member is not "
                     "accessible from the current shader stage.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *push_constant_not_accessible_message =
            "Push constant range covering variable starting at offset 0 not accessible from stage VK_SHADER_STAGE_VERTEX_BIT";

    char const *vsSource =
            "#version 450\n"
            "\n"
            "layout(push_constant, std430) uniform foo { float x; } consts;\n"
            "out gl_PerVertex {\n"
            "    vec4 gl_Position;\n"
            "};\n"
            "void main(){\n"
            "   gl_Position = vec4(consts.x);\n"
            "}\n";

    char const *fsSource =
            "#version 450\n"
            "\n"
            "layout(location = 0) out vec4 uFragColor;\n"
            "void main(){\n"
            "   uFragColor = vec4(0,1,0,1);\n"
            "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // Set up a push constant range
    VkPushConstantRange push_constant_ranges = {};
    // Set to the wrong stage to challenge core_validation
    push_constant_ranges.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_constant_ranges.size = 4;

    pipeline_layout_create_info.pPushConstantRanges = &push_constant_ranges;
    pipeline_layout_create_info.pushConstantRangeCount = 1;

    VkPipelineLayout pipeline_layout;
    ASSERT_VK_SUCCESS(vkCreatePipelineLayout(m_device->device(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, push_constant_not_accessible_message);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineCheckShaderNotEnabled) {
    TEST_DESCRIPTION(
        "Create a graphics pipeline in which a capability declared by the shader requires a feature not enabled on the device.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *feature_not_enabled_message =
            "Shader requires VkPhysicalDeviceFeatures::shaderFloat64 but is not enabled on the device";

    // Some awkward steps are required to test with custom device features.
    std::vector<const char *> device_extension_names;
    auto features = m_device->phy().features();
    // Disable support for 64 bit floats
    features.shaderFloat64 = false;
    // The sacrificial device object
    VkDeviceObj test_device(0, gpu(), device_extension_names, &features);

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   dvec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
                           "   color = vec4(green);\n"
                           "}\n";

    VkShaderObj vs(&test_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(&test_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkRenderpassObj render_pass(&test_device);

    VkPipelineObj pipe(&test_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout pipeline_layout;
    ASSERT_VK_SUCCESS(vkCreatePipelineLayout(test_device.device(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, feature_not_enabled_message);
    pipe.CreateVKPipeline(pipeline_layout, render_pass.handle());
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(test_device.device(), pipeline_layout, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineCheckShaderBadCapability) {
    TEST_DESCRIPTION("Create a graphics pipeline in which a capability declared by the shader is not supported by Vulkan shaders.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    const char *bad_capability_message = "Shader declares capability 53, not supported in Vulkan.";

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "layout(xfb_buffer = 1) out;"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   dvec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
                           "   color = vec4(green);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout pipeline_layout;
    ASSERT_VK_SUCCESS(vkCreatePipelineLayout(m_device->device(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, bad_capability_message);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());
    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineFragmentInputNotProvided) {
    TEST_DESCRIPTION("Test that an error is produced for a fragment shader input "
                     "which is not present in the outputs of the previous stage");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "not written by vertex shader");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) in float x;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineFragmentInputNotProvidedInBlock) {
    TEST_DESCRIPTION("Test that an error is produced for a fragment shader input "
                     "within an interace block, which is not present in the outputs "
                     "of the previous stage.");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "not written by vertex shader");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "in block { layout(location=0) float x; } ins;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(ins.x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineVsFsTypeMismatchArraySize) {
    TEST_DESCRIPTION("Test that an error is produced for mismatched array sizes "
                     "across the vertex->fragment shader interface");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Type mismatch on location 0.0: 'ptr to "
                                                                        "output arr[2] of float32' vs 'ptr to "
                                                                        "input arr[3] of float32'");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out float x[2];\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   x[0] = 0; x[1] = 0;\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) in float x[3];\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(x[0] + x[1] + x[2]);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}


TEST_F(VkLayerTest, CreatePipelineVsFsTypeMismatch) {
    TEST_DESCRIPTION("Test that an error is produced for mismatched types across "
                     "the vertex->fragment shader interface");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Type mismatch on location 0");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out int x;\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   x = 0;\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) in float x;\n" /* VS writes int */
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineVsFsTypeMismatchInBlock) {
    TEST_DESCRIPTION("Test that an error is produced for mismatched types across "
                     "the vertex->fragment shader interface, when the variable is contained within "
                     "an interface block");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Type mismatch on location 0");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out block { layout(location=0) int x; } outs;\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   outs.x = 0;\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "in block { layout(location=0) float x; } ins;\n" /* VS writes int */
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(ins.x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineVsFsMismatchByLocation) {
    TEST_DESCRIPTION("Test that an error is produced for location mismatches across "
                     "the vertex->fragment shader interface; This should manifest as a not-written/not-consumed "
                     "pair, but flushes out broken walking of the interfaces");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "location 0.0 which is not written by vertex shader");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out block { layout(location=1) float x; } outs;\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   outs.x = 0;\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "in block { layout(location=0) float x; } ins;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(ins.x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineVsFsMismatchByComponent) {
    TEST_DESCRIPTION("Test that an error is produced for component mismatches across the "
                     "vertex->fragment shader interface. It's not enough to have the same set of locations in "
                     "use; matching is defined in terms of spirv variables.");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "location 0.1 which is not written by vertex shader");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out block { layout(location=0, component=0) float x; } outs;\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   outs.x = 0;\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "in block { layout(location=0, component=1) float x; } ins;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(ins.x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineAttribNotConsumed) {
    TEST_DESCRIPTION("Test that a warning is produced for a vertex attribute which is "
                     "not consumed by the vertex shader");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, "location 0 not consumed by vertex shader");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attrib;
    memset(&input_attrib, 0, sizeof(input_attrib));
    input_attrib.format = VK_FORMAT_R32_SFLOAT;

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(&input_attrib, 1);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineAttribLocationMismatch) {
    TEST_DESCRIPTION("Test that a warning is produced for a location mismatch on "
                     "vertex attributes. This flushes out bad behavior in the interface walker");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, "location 0 not consumed by vertex shader");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attrib;
    memset(&input_attrib, 0, sizeof(input_attrib));
    input_attrib.format = VK_FORMAT_R32_SFLOAT;

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(location=1) in float x;\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(x);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(&input_attrib, 1);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineAttribNotProvided) {
    TEST_DESCRIPTION("Test that an error is produced for a vertex shader input which is not "
                     "provided by a vertex attribute");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Vertex shader consumes input at location 0 but not provided");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) in vec4 x;\n" /* not provided */
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = x;\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineAttribTypeMismatch) {
    TEST_DESCRIPTION("Test that an error is produced for a mismatch between the "
                     "fundamental type (float/int/uint) of an attribute and the "
                     "vertex shader input that consumes it");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "location 0 does not match vertex shader input type");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attrib;
    memset(&input_attrib, 0, sizeof(input_attrib));
    input_attrib.format = VK_FORMAT_R32_SFLOAT;

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) in int x;\n" /* attrib provided float */
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(x);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(&input_attrib, 1);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineDuplicateStage) {
    TEST_DESCRIPTION("Test that an error is produced for a pipeline containing multiple "
                     "shaders for the same stage");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Multiple shaders provided for stage VK_SHADER_STAGE_VERTEX_BIT");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&vs);        // intentionally duplicate vertex shader attachment
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineMissingEntrypoint) {
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "No entrypoint found named `foo`");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(0);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this, "foo");

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineDepthStencilRequired) {
    m_errorMonitor->SetDesiredFailureMsg(
        VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "pDepthStencilState is NULL when rasterization is enabled and subpass "
        "uses a depth/stencil attachment");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "void main(){ gl_Position = vec4(0); }\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    VkAttachmentDescription attachments[] = {
        { 0, VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        { 0, VK_FORMAT_D16_UNORM, VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };
    VkAttachmentReference refs[] = {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
    };
    VkSubpassDescription subpass = {
        0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr,
        1, &refs[0], nullptr, &refs[1],
        0, nullptr
    };
    VkRenderPassCreateInfo rpci = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr,
        0, 2, attachments, 1, &subpass, 0, nullptr
    };
    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), rp);

    m_errorMonitor->VerifyFound();

    vkDestroyRenderPass(m_device->device(), rp, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineTessPatchDecorationMismatch) {
    TEST_DESCRIPTION("Test that an error is produced for a variable output from "
                     "the TCS without the patch decoration, but consumed in the TES "
                     "with the decoration.");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "is per-vertex in tessellation control shader stage "
                                                                        "but per-patch in tessellation evaluation shader stage");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    if (!m_device->phy().features().tessellationShader) {
        printf("Device does not support tessellation shaders; skipped.\n");
        return;
    }

    char const *vsSource = "#version 450\n"
                           "void main(){}\n";
    char const *tcsSource = "#version 450\n"
                            "layout(location=0) out int x[];\n"
                            "layout(vertices=3) out;\n"
                            "void main(){\n"
                            "   gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = 1;\n"
                            "   gl_TessLevelInner[0] = 1;\n"
                            "   x[gl_InvocationID] = gl_InvocationID;\n"
                            "}\n";
    char const *tesSource = "#version 450\n"
                            "layout(triangles, equal_spacing, cw) in;\n"
                            "layout(location=0) patch in int x;\n"
                            "out gl_PerVertex { vec4 gl_Position; };\n"
                            "void main(){\n"
                            "   gl_Position.xyz = gl_TessCoord;\n"
                            "   gl_Position.w = x;\n"
                            "}\n";
    char const *fsSource = "#version 450\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj tcs(m_device, tcsSource, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, this);
    VkShaderObj tes(m_device, tesSource, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineInputAssemblyStateCreateInfo iasci{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
                                                 VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, VK_FALSE};

    VkPipelineTessellationStateCreateInfo tsci{VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, nullptr, 0, 3};

    VkPipelineObj pipe(m_device);
    pipe.SetInputAssembly(&iasci);
    pipe.SetTessellation(&tsci);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&tcs);
    pipe.AddShader(&tes);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineAttribBindingConflict) {
    TEST_DESCRIPTION("Test that an error is produced for a vertex attribute setup where multiple "
                     "bindings provide the same location");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Duplicate vertex input binding descriptions for binding 0");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    /* Two binding descriptions for binding 0 */
    VkVertexInputBindingDescription input_bindings[2];
    memset(input_bindings, 0, sizeof(input_bindings));

    VkVertexInputAttributeDescription input_attrib;
    memset(&input_attrib, 0, sizeof(input_attrib));
    input_attrib.format = VK_FORMAT_R32_SFLOAT;

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) in float x;\n" /* attrib provided float */
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(x);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main(){\n"
                           "   color = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(input_bindings, 2);
    pipe.AddVertexInputAttribs(&input_attrib, 1);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineFragmentOutputNotWritten) {
    TEST_DESCRIPTION("Test that an error is produced for a fragment shader which does not "
                     "provide an output for one of the pipeline's color attachments");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Attachment 0 not written by fragment shader");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "void main(){\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0, not written */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineFragmentOutputNotConsumed) {
    TEST_DESCRIPTION("Test that a warning is produced for a fragment shader which provides a spurious "
                     "output with no matching attachment");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                         "fragment shader writes to output location 1 with no matching attachment");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 x;\n"
                           "layout(location=1) out vec4 y;\n" /* no matching attachment for this */
                           "void main(){\n"
                           "   x = vec4(1);\n"
                           "   y = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0, not written */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    /* FS writes CB 1, but we don't configure it */

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineFragmentOutputTypeMismatch) {
    TEST_DESCRIPTION("Test that an error is produced for a mismatch between the fundamental "
                     "type of an fragment shader output variable, and the format of the corresponding attachment");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "does not match fragment shader output type");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out ivec4 x;\n" /* not UNORM */
                           "void main(){\n"
                           "   x = ivec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0; type is UNORM by default */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineUniformBlockNotProvided) {
    TEST_DESCRIPTION("Test that an error is produced for a shader consuming a uniform "
                     "block which has no corresponding binding in the pipeline layout");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "not declared in pipeline layout");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 x;\n"
                           "layout(set=0) layout(binding=0) uniform foo { int x; int y; } bar;\n"
                           "void main(){\n"
                           "   x = vec4(bar.y);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0; type is UNORM by default */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelinePushConstantsNotInLayout) {
    TEST_DESCRIPTION("Test that an error is produced for a shader consuming push constants "
                     "which are not provided in the pipeline layout");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "not declared in layout");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "layout(push_constant, std430) uniform foo { float x; } consts;\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "   gl_Position = vec4(consts.x);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(location=0) out vec4 x;\n"
                           "void main(){\n"
                           "   x = vec4(1);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0; type is UNORM by default */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    /* should have generated an error -- no push constant ranges provided! */
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreatePipelineInputAttachmentMissing) {
    TEST_DESCRIPTION("Test that an error is produced for a shader consuming an input attachment "
                     "which is not included in the subpass description");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "consumes input attachment index 0 but not provided in subpass");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "    gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput x;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main() {\n"
                           "   color = subpassLoad(x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo dslci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dslb};
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr};
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    // error here.
    pipe.CreateVKPipeline(pl, renderPass());

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineInputAttachmentTypeMismatch) {
    TEST_DESCRIPTION("Test that an error is produced for a shader consuming an input attachment "
                     "with a format having a different fundamental type");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "input attachment 0 format of VK_FORMAT_R8G8B8A8_UINT does not match");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "    gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput x;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main() {\n"
                           "   color = subpassLoad(x);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo dslci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dslb};
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr};
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    VkAttachmentDescription descs[2] = {
        {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
         VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {0, VK_FORMAT_R8G8B8A8_UINT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
         VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL},
    };
    VkAttachmentReference color = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference input = {
        1, VK_IMAGE_LAYOUT_GENERAL,
    };

    VkSubpassDescription sd = {0, VK_PIPELINE_BIND_POINT_GRAPHICS, 1, &input, 1, &color, nullptr, nullptr, 0, nullptr};

    VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 2, descs, 1, &sd, 0, nullptr};
    VkRenderPass rp;
    err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    // error here.
    pipe.CreateVKPipeline(pl, rp);

    m_errorMonitor->VerifyFound();

    vkDestroyRenderPass(m_device->device(), rp, nullptr);
    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkLayerTest, CreatePipelineInputAttachmentMissingArray) {
    TEST_DESCRIPTION("Test that an error is produced for a shader consuming an input attachment "
                     "which is not included in the subpass description -- array case");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "consumes input attachment index 1 but not provided in subpass");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex {\n"
                           "    vec4 gl_Position;\n"
                           "};\n"
                           "void main(){\n"
                           "    gl_Position = vec4(1);\n"
                           "}\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput xs[2];\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main() {\n"
                           "   color = subpassLoad(xs[1]);\n"
                           "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo dslci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dslb};
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr};
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    // error here.
    pipe.CreateVKPipeline(pl, renderPass());

    m_errorMonitor->VerifyFound();

    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkLayerTest, CreateComputePipelineMissingDescriptor) {
    TEST_DESCRIPTION("Test that an error is produced for a compute pipeline consuming a "
                     "descriptor which is not provided in the pipeline layout");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Shader uses descriptor slot 0.0");

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *csSource = "#version 450\n"
                           "\n"
                           "layout(local_size_x=1) in;\n"
                           "layout(set=0, binding=0) buffer block { vec4 x; };\n"
                           "void main(){\n"
                           "   x = vec4(1);\n"
                           "}\n";

    VkShaderObj cs(m_device, csSource, VK_SHADER_STAGE_COMPUTE_BIT, this);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    VkComputePipelineCreateInfo cpci = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                                        nullptr,
                                        0,
                                        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                         VK_SHADER_STAGE_COMPUTE_BIT, cs.handle(), "main", nullptr},
                                        descriptorSet.GetPipelineLayout(),
                                        VK_NULL_HANDLE,
                                        -1};

    VkPipeline pipe;
    VkResult err = vkCreateComputePipelines(m_device->device(), VK_NULL_HANDLE, 1, &cpci, nullptr, &pipe);

    m_errorMonitor->VerifyFound();

    if (err == VK_SUCCESS) {
        vkDestroyPipeline(m_device->device(), pipe, nullptr);
    }
}

TEST_F(VkLayerTest, CreateComputePipelineDescriptorTypeMismatch) {
    TEST_DESCRIPTION("Test that an error is produced for a pipeline consuming a "
                     "descriptor-backed resource of a mismatched type");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "but descriptor of type VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDescriptorSetLayoutBinding binding = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo dslci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1, &binding};
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr};
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    char const *csSource = "#version 450\n"
                           "\n"
                           "layout(local_size_x=1) in;\n"
                           "layout(set=0, binding=0) buffer block { vec4 x; };\n"
                           "void main() {\n"
                           "   x.x = 1.0f;\n"
                           "}\n";
    VkShaderObj cs(m_device, csSource, VK_SHADER_STAGE_COMPUTE_BIT, this);

    VkComputePipelineCreateInfo cpci = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                                        nullptr,
                                        0,
                                        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                         VK_SHADER_STAGE_COMPUTE_BIT, cs.handle(), "main", nullptr},
                                        pl,
                                        VK_NULL_HANDLE,
                                        -1};

    VkPipeline pipe;
    err = vkCreateComputePipelines(m_device->device(), VK_NULL_HANDLE, 1, &cpci, nullptr, &pipe);

    m_errorMonitor->VerifyFound();

    if (err == VK_SUCCESS) {
        vkDestroyPipeline(m_device->device(), pipe, nullptr);
    }

    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkLayerTest, DrawTimeImageViewTypeMismatchWithPipeline) {
    TEST_DESCRIPTION("Test that an error is produced when an image view type "
                     "does not match the dimensionality declared in the shader");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "requires an image view of type VK_IMAGE_VIEW_TYPE_3D");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { vec4 gl_Position; };\n"
                           "void main() { gl_Position = vec4(0); }\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0) uniform sampler3D s;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main() {\n"
                           "   color = texture(s, vec3(0));\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();

    VkTextureObj texture(m_device, nullptr);
    VkSamplerObj sampler(m_device);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler, &texture);
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    VkResult err = pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();

    m_commandBuffer->BindPipeline(pipe);
    m_commandBuffer->BindDescriptorSet(descriptorSet);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    vkCmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    VkRect2D scissor = {{0, 0}, {16, 16}};
    vkCmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);

    // error produced here.
    vkCmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);

    m_errorMonitor->VerifyFound();

    EndCommandBuffer();
}

TEST_F(VkLayerTest, DrawTimeImageMultisampleMismatchWithPipeline) {
    TEST_DESCRIPTION("Test that an error is produced when a multisampled images "
                     "are consumed via singlesample images types in the shader, or vice versa.");

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "requires bound image to have multiple samples");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
                           "\n"
                           "out gl_PerVertex { vec4 gl_Position; };\n"
                           "void main() { gl_Position = vec4(0); }\n";
    char const *fsSource = "#version 450\n"
                           "\n"
                           "layout(set=0, binding=0) uniform sampler2DMS s;\n"
                           "layout(location=0) out vec4 color;\n"
                           "void main() {\n"
                           "   color = texelFetch(s, ivec2(0), 0);\n"
                           "}\n";
    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();

    VkTextureObj texture(m_device, nullptr);
    VkSamplerObj sampler(m_device);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendSamplerTexture(&sampler, &texture);
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    VkResult err = pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();

    m_commandBuffer->BindPipeline(pipe);
    m_commandBuffer->BindDescriptorSet(descriptorSet);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    vkCmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    VkRect2D scissor = {{0, 0}, {16, 16}};
    vkCmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);

    // error produced here.
    vkCmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);

    m_errorMonitor->VerifyFound();

    EndCommandBuffer();
}

#endif // SHADER_CHECKER_TESTS

#if DEVICE_LIMITS_TESTS
TEST_F(VkLayerTest, CreateImageLimitsViolationMaxWidth) {
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "CreateImage extents exceed allowable limits for format");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image
    VkImage image;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    // Introduce error by sending down a bogus width extent
    image_create_info.extent.width = 65536;
    vkCreateImage(m_device->device(), &image_create_info, NULL, &image);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CreateImageLimitsViolationMinWidth) {
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "CreateImage extents is 0 for at least one required dimension");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image
    VkImage image;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    // Introduce error by sending down a bogus width extent
    image_create_info.extent.width = 0;
    vkCreateImage(m_device->device(), &image_create_info, NULL, &image);

    m_errorMonitor->VerifyFound();
}
#endif // DEVICE_LIMITS_TESTS

#if IMAGE_TESTS
TEST_F(VkLayerTest, AttachmentDescriptionUndefinedFormat) {
    TEST_DESCRIPTION("Create a render pass with an attachment description "
                     "format set to VK_FORMAT_UNDEFINED");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "format is VK_FORMAT_UNDEFINED");

    VkAttachmentReference color_attach = {};
    color_attach.layout = VK_IMAGE_LAYOUT_GENERAL;
    color_attach.attachment = 0;
    VkSubpassDescription subpass = {};
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attach;

    VkRenderPassCreateInfo rpci = {};
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.attachmentCount = 1;
    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_UNDEFINED;
    rpci.pAttachments = &attach_desc;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkRenderPass rp;
    VkResult result = vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);

    m_errorMonitor->VerifyFound();

    if (result == VK_SUCCESS) {
        vkDestroyRenderPass(m_device->device(), rp, NULL);
    }
}

TEST_F(VkLayerTest, InvalidImageView) {
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCreateImageView called with baseMipLevel 10 ");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image and try to create a view with bad baseMipLevel
    VkImage image;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = tex_format;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.subresourceRange.baseMipLevel = 10; // cause an error
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageView view;
    err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);

    m_errorMonitor->VerifyFound();
    vkDestroyImage(m_device->device(), image, NULL);
}

TEST_F(VkLayerTest, CreateImageViewNoMemoryBoundToImage) {
    VkResult err;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         " used with no memory bound. Memory should be bound by calling vkBindImageMemory().");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create an image and try to create a view with no memory backing the image
    VkImage image;

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = tex_format;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageView view;
    err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);

    m_errorMonitor->VerifyFound();
    vkDestroyImage(m_device->device(), image, NULL);
    // If last error is success, it still created the view, so delete it.
    if (err == VK_SUCCESS) {
        vkDestroyImageView(m_device->device(), view, NULL);
    }
}

TEST_F(VkLayerTest, InvalidImageViewAspect) {
    TEST_DESCRIPTION("Create an image and try to create a view with an invalid aspectMask");
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCreateImageView: Color image "
                                                                        "formats must have ONLY the "
                                                                        "VK_IMAGE_ASPECT_COLOR_BIT set");

    ASSERT_NO_FATAL_FAILURE(InitState());

    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkImageObj image(m_device);
    image.init(32, 32, tex_format, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_LINEAR, 0);
    ASSERT_TRUE(image.initialized());

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_view_create_info.image = image.handle();
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = tex_format;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    // Cause an error by setting an invalid image aspect
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_METADATA_BIT;

    VkImageView view;
    vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CopyImageLayerCountMismatch) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdCopyImage: number of layers in source and destination subresources for pRegions");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create two images of different types and try to copy between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 4;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    VkImageCopy copyRegion;
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset.x = 0;
    copyRegion.srcOffset.y = 0;
    copyRegion.srcOffset.z = 0;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    // Introduce failure by forcing the dst layerCount to differ from src
    copyRegion.dstSubresource.layerCount = 3;
    copyRegion.dstOffset.x = 0;
    copyRegion.dstOffset.y = 0;
    copyRegion.dstOffset.z = 0;
    copyRegion.extent.width = 1;
    copyRegion.extent.height = 1;
    copyRegion.extent.depth = 1;
    m_commandBuffer->CopyImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, ImageLayerUnsupportedFormat) {

    TEST_DESCRIPTION("Creating images with unsuported formats ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    // Create image with unsupported format - Expect FORMAT_UNSUPPORTED
    VkImageCreateInfo image_create_info;
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_UNDEFINED;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.flags = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCreateImage: VkFormat for image must not be VK_FORMAT_UNDEFINED");

    VkImage localImage;
    vkCreateImage(m_device->handle(), &image_create_info, NULL, &localImage);
    m_errorMonitor->VerifyFound();

    VkFormat unsupported = VK_FORMAT_UNDEFINED;
    // Look for a format that is COMPLETELY unsupported with this hardware
    for (int f = VK_FORMAT_BEGIN_RANGE; f <= VK_FORMAT_END_RANGE; f++) {
        VkFormat format = static_cast<VkFormat>(f);
        VkFormatProperties fProps = m_device->format_properties(format);
        if (format != VK_FORMAT_UNDEFINED && fProps.linearTilingFeatures == 0 && fProps.optimalTilingFeatures == 0) {
            unsupported = format;
            break;
        }
    }

    if (unsupported != VK_FORMAT_UNDEFINED) {
        image_create_info.format = unsupported;
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "is an unsupported format");

        vkCreateImage(m_device->handle(), &image_create_info, NULL, &localImage);
        m_errorMonitor->VerifyFound();
    }
}

TEST_F(VkLayerTest, ImageLayerViewTests) {
    VkResult ret;
    TEST_DESCRIPTION("Passing bad parameters to CreateImageView");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageView imgView;
    VkImageViewCreateInfo imgViewInfo = {};
    imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imgViewInfo.image = image.handle();
    imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imgViewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    imgViewInfo.subresourceRange.layerCount = 1;
    imgViewInfo.subresourceRange.baseMipLevel = 0;
    imgViewInfo.subresourceRange.levelCount = 1;
    imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCreateImageView called with baseMipLevel");
    // View can't have baseMipLevel >= image's mipLevels - Expect
    // VIEW_CREATE_ERROR
    imgViewInfo.subresourceRange.baseMipLevel = 1;
    vkCreateImageView(m_device->handle(), &imgViewInfo, NULL, &imgView);
    m_errorMonitor->VerifyFound();
    imgViewInfo.subresourceRange.baseMipLevel = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCreateImageView called with baseArrayLayer");
    // View can't have baseArrayLayer >= image's arraySize - Expect
    // VIEW_CREATE_ERROR
    imgViewInfo.subresourceRange.baseArrayLayer = 1;
    vkCreateImageView(m_device->handle(), &imgViewInfo, NULL, &imgView);
    m_errorMonitor->VerifyFound();
    imgViewInfo.subresourceRange.baseArrayLayer = 0;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCreateImageView called with 0 in "
                                                                        "pCreateInfo->subresourceRange."
                                                                        "levelCount");
    // View's levelCount can't be 0 - Expect VIEW_CREATE_ERROR
    imgViewInfo.subresourceRange.levelCount = 0;
    vkCreateImageView(m_device->handle(), &imgViewInfo, NULL, &imgView);
    m_errorMonitor->VerifyFound();
    imgViewInfo.subresourceRange.levelCount = 1;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCreateImageView called with 0 in "
                                                                        "pCreateInfo->subresourceRange."
                                                                        "layerCount");
    // View's layerCount can't be 0 - Expect VIEW_CREATE_ERROR
    imgViewInfo.subresourceRange.layerCount = 0;
    vkCreateImageView(m_device->handle(), &imgViewInfo, NULL, &imgView);
    m_errorMonitor->VerifyFound();
    imgViewInfo.subresourceRange.layerCount = 1;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "but both must be color formats");
    // Can't use depth format for view into color image - Expect INVALID_FORMAT
    imgViewInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    vkCreateImageView(m_device->handle(), &imgViewInfo, NULL, &imgView);
    m_errorMonitor->VerifyFound();
    imgViewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "Formats MUST be IDENTICAL unless "
                                                                        "VK_IMAGE_CREATE_MUTABLE_FORMAT BIT "
                                                                        "was set on image creation.");
    // Same compatibility class but no MUTABLE_FORMAT bit - Expect
    // VIEW_CREATE_ERROR
    imgViewInfo.format = VK_FORMAT_B8G8R8A8_UINT;
    vkCreateImageView(m_device->handle(), &imgViewInfo, NULL, &imgView);
    m_errorMonitor->VerifyFound();
    imgViewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "can support ImageViews with "
                                                                        "differing formats but they must be "
                                                                        "in the same compatibility class.");
    // Have MUTABLE_FORMAT bit but not in same compatibility class - Expect
    // VIEW_CREATE_ERROR
    VkImageCreateInfo mutImgInfo = image.create_info();
    VkImage mutImage;
    mutImgInfo.format = VK_FORMAT_R8_UINT;
    assert(m_device->format_properties(VK_FORMAT_R8_UINT).optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    mutImgInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    mutImgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ret = vkCreateImage(m_device->handle(), &mutImgInfo, NULL, &mutImage);
    ASSERT_VK_SUCCESS(ret);
    imgViewInfo.image = mutImage;
    vkCreateImageView(m_device->handle(), &imgViewInfo, NULL, &imgView);
    m_errorMonitor->VerifyFound();
    imgViewInfo.image = image.handle();
    vkDestroyImage(m_device->handle(), mutImage, NULL);
}

TEST_F(VkLayerTest, MiscImageLayerTests) {

    TEST_DESCRIPTION("Image layer tests that don't belong elsewhare");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "number of layers in image subresource is zero");
    vk_testing::Buffer buffer;
    VkMemoryPropertyFlags reqs = 0;
    buffer.init_as_src(*m_device, 128 * 128 * 4, reqs);
    VkBufferImageCopy region = {};
    region.bufferRowLength = 128;
    region.bufferImageHeight = 128;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // layerCount can't be 0 - Expect MISMATCHED_IMAGE_ASPECT
    region.imageSubresource.layerCount = 0;
    region.imageExtent.height = 4;
    region.imageExtent.width = 4;
    region.imageExtent.depth = 1;
    m_commandBuffer->BeginCommandBuffer();
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_errorMonitor->VerifyFound();
    region.imageSubresource.layerCount = 1;

    // BufferOffset must be a multiple of the calling command's VkImage parameter's texel size
    // Introduce failure by setting bufferOffset to 1 and 1/2 texels
    region.bufferOffset = 6;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "must be a multiple of this format's texel size");
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_errorMonitor->VerifyFound();

    // BufferOffset must be a multiple of 4
    // Introduce failure by setting bufferOffset to a value not divisible by 4
    region.bufferOffset = 6;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "must be a multiple of 4");
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_errorMonitor->VerifyFound();

    // BufferRowLength must be 0, or greater than or equal to the width member of imageExtent
    region.bufferOffset = 0;
    region.imageExtent.height = 128;
    region.imageExtent.width = 128;
    // Introduce failure by setting bufferRowLength > 0 but less than width
    region.bufferRowLength = 64;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "must be zero or greater-than-or-equal-to imageExtent.width");
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_errorMonitor->VerifyFound();

    // BufferImageHeight must be 0, or greater than or equal to the height member of imageExtent
    region.bufferRowLength = 128;
    // Introduce failure by setting bufferRowHeight > 0 but less than height
    region.bufferImageHeight = 64;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "must be zero or greater-than-or-equal-to imageExtent.height");
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_errorMonitor->VerifyFound();

    region.bufferImageHeight = 128;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "aspectMasks for each region must "
                                                                        "specify only COLOR or DEPTH or "
                                                                        "STENCIL");
    // Expect MISMATCHED_IMAGE_ASPECT
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_METADATA_BIT;
    vkCmdCopyBufferToImage(m_commandBuffer->GetBufferHandle(), buffer.handle(), image.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_errorMonitor->VerifyFound();
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "If the format of srcImage is a depth, stencil, depth stencil or "
                                         "integer-based format then filter must be VK_FILTER_NEAREST");
    // Expect INVALID_FILTER
    VkImageObj intImage1(m_device);
    intImage1.init(128, 128, VK_FORMAT_R8_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    VkImageObj intImage2(m_device);
    intImage2.init(128, 128, VK_FORMAT_R8_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    VkImageBlit blitRegion = {};
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    vkCmdBlitImage(m_commandBuffer->GetBufferHandle(), intImage1.handle(), intImage1.layout(), intImage2.handle(),
                   intImage2.layout(), 16, &blitRegion, VK_FILTER_LINEAR);
    m_errorMonitor->VerifyFound();

    // Look for NULL-blit warning
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, "Offsets specify a zero-volume area.");
    vkCmdBlitImage(m_commandBuffer->GetBufferHandle(), intImage1.handle(), intImage1.layout(), intImage2.handle(),
                   intImage2.layout(), 1, &blitRegion, VK_FILTER_LINEAR);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "called with 0 in ppMemoryBarriers");
    VkImageMemoryBarrier img_barrier;
    img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.pNext = NULL;
    img_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    img_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_barrier.image = image.handle();
    img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier.subresourceRange.baseArrayLayer = 0;
    img_barrier.subresourceRange.baseMipLevel = 0;
    // layerCount should not be 0 - Expect INVALID_IMAGE_RESOURCE
    img_barrier.subresourceRange.layerCount = 0;
    img_barrier.subresourceRange.levelCount = 1;
    vkCmdPipelineBarrier(m_commandBuffer->GetBufferHandle(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &img_barrier);
    m_errorMonitor->VerifyFound();
    img_barrier.subresourceRange.layerCount = 1;
}

TEST_F(VkLayerTest, ImageFormatLimits) {

    TEST_DESCRIPTION("Exceed the limits of image format ");

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "CreateImage extents exceed allowable limits for format");
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.flags = 0;

    VkImage nullImg;
    VkImageFormatProperties imgFmtProps;
    vkGetPhysicalDeviceImageFormatProperties(gpu(), image_create_info.format, image_create_info.imageType, image_create_info.tiling,
                                             image_create_info.usage, image_create_info.flags, &imgFmtProps);
    image_create_info.extent.depth = imgFmtProps.maxExtent.depth + 1;
    // Expect INVALID_FORMAT_LIMITS_VIOLATION
    vkCreateImage(m_device->handle(), &image_create_info, NULL, &nullImg);
    m_errorMonitor->VerifyFound();
    image_create_info.extent.depth = 1;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "exceeds allowable maximum supported by format of");
    image_create_info.mipLevels = imgFmtProps.maxMipLevels + 1;
    // Expect INVALID_FORMAT_LIMITS_VIOLATION
    vkCreateImage(m_device->handle(), &image_create_info, NULL, &nullImg);
    m_errorMonitor->VerifyFound();
    image_create_info.mipLevels = 1;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "exceeds allowable maximum supported by format of");
    image_create_info.arrayLayers = imgFmtProps.maxArrayLayers + 1;
    // Expect INVALID_FORMAT_LIMITS_VIOLATION
    vkCreateImage(m_device->handle(), &image_create_info, NULL, &nullImg);
    m_errorMonitor->VerifyFound();
    image_create_info.arrayLayers = 1;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "is not supported by format");
    int samples = imgFmtProps.sampleCounts >> 1;
    image_create_info.samples = (VkSampleCountFlagBits)samples;
    // Expect INVALID_FORMAT_LIMITS_VIOLATION
    vkCreateImage(m_device->handle(), &image_create_info, NULL, &nullImg);
    m_errorMonitor->VerifyFound();
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "pCreateInfo->initialLayout, must be "
                                                                        "VK_IMAGE_LAYOUT_UNDEFINED or "
                                                                        "VK_IMAGE_LAYOUT_PREINITIALIZED");
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Expect INVALID_LAYOUT
    vkCreateImage(m_device->handle(), &image_create_info, NULL, &nullImg);
    m_errorMonitor->VerifyFound();
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

TEST_F(VkLayerTest, CopyImageSrcSizeExceeded) {

    // Image copy with source region specified greater than src image size
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, VALIDATION_ERROR_01175);

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImageObj src_image(m_device);
    src_image.init(32, 32, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_LINEAR, 0);
    VkImageObj dst_image(m_device);
    dst_image.init(64, 64, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_LINEAR, 0);

    BeginCommandBuffer();
    VkImageCopy copy_region;
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.mipLevel = 0;
    copy_region.srcSubresource.baseArrayLayer = 0;
    copy_region.srcSubresource.layerCount = 0;
    copy_region.srcOffset.x = 0;
    copy_region.srcOffset.y = 0;
    copy_region.srcOffset.z = 0;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.mipLevel = 0;
    copy_region.dstSubresource.baseArrayLayer = 0;
    copy_region.dstSubresource.layerCount = 0;
    copy_region.dstOffset.x = 0;
    copy_region.dstOffset.y = 0;
    copy_region.dstOffset.z = 0;
    copy_region.extent.width = 64;
    copy_region.extent.height = 64;
    copy_region.extent.depth = 1;
    m_commandBuffer->CopyImage(src_image.image(), VK_IMAGE_LAYOUT_GENERAL, dst_image.image(), VK_IMAGE_LAYOUT_GENERAL, 1,
                               &copy_region);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CopyImageDstSizeExceeded) {

    // Image copy with dest region specified greater than dest image size
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, VALIDATION_ERROR_01176);

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkImageObj src_image(m_device);
    src_image.init(64, 64, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TILING_LINEAR, 0);
    VkImageObj dst_image(m_device);
    dst_image.init(32, 32, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_LINEAR, 0);

    BeginCommandBuffer();
    VkImageCopy copy_region;
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.mipLevel = 0;
    copy_region.srcSubresource.baseArrayLayer = 0;
    copy_region.srcSubresource.layerCount = 0;
    copy_region.srcOffset.x = 0;
    copy_region.srcOffset.y = 0;
    copy_region.srcOffset.z = 0;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.mipLevel = 0;
    copy_region.dstSubresource.baseArrayLayer = 0;
    copy_region.dstSubresource.layerCount = 0;
    copy_region.dstOffset.x = 0;
    copy_region.dstOffset.y = 0;
    copy_region.dstOffset.z = 0;
    copy_region.extent.width = 64;
    copy_region.extent.height = 64;
    copy_region.extent.depth = 1;
    m_commandBuffer->CopyImage(src_image.image(), VK_IMAGE_LAYOUT_GENERAL, dst_image.image(), VK_IMAGE_LAYOUT_GENERAL, 1,
                               &copy_region);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, CopyImageFormatSizeMismatch) {
    VkResult err;
    bool pass;

    // Create color images with different format sizes and try to copy between them
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdCopyImage called with unmatched source and dest image format sizes");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create two images of different types and try to copy between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // Introduce failure by creating second image with a different-sized format.
    image_create_info.format = VK_FORMAT_R5G5B5A1_UNORM_PACK16;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    VkImageCopy copyRegion;
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 0;
    copyRegion.srcOffset.x = 0;
    copyRegion.srcOffset.y = 0;
    copyRegion.srcOffset.z = 0;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 0;
    copyRegion.dstOffset.x = 0;
    copyRegion.dstOffset.y = 0;
    copyRegion.dstOffset.z = 0;
    copyRegion.extent.width = 1;
    copyRegion.extent.height = 1;
    copyRegion.extent.depth = 1;
    m_commandBuffer->CopyImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, CopyImageDepthStencilFormatMismatch) {
    VkResult err;
    bool pass;

    // Create a color image and a depth/stencil image and try to copy between them
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdCopyImage called with unmatched source and dest image depth");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create two images of different types and try to copy between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // Introduce failure by creating second image with a depth/stencil format
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    VkImageCopy copyRegion;
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 0;
    copyRegion.srcOffset.x = 0;
    copyRegion.srcOffset.y = 0;
    copyRegion.srcOffset.z = 0;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 0;
    copyRegion.dstOffset.x = 0;
    copyRegion.dstOffset.y = 0;
    copyRegion.dstOffset.z = 0;
    copyRegion.extent.width = 1;
    copyRegion.extent.height = 1;
    copyRegion.extent.depth = 1;
    m_commandBuffer->CopyImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, ResolveImageLowSampleCount) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdResolveImage called with source sample count less than 2.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create two images of sample count 1 and try to Resolve between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 1;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    // Need memory barrier to VK_IMAGE_LAYOUT_GENERAL for source and dest?
    // VK_IMAGE_LAYOUT_UNDEFINED = 0,
    // VK_IMAGE_LAYOUT_GENERAL = 1,
    VkImageResolve resolveRegion;
    resolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.srcSubresource.mipLevel = 0;
    resolveRegion.srcSubresource.baseArrayLayer = 0;
    resolveRegion.srcSubresource.layerCount = 1;
    resolveRegion.srcOffset.x = 0;
    resolveRegion.srcOffset.y = 0;
    resolveRegion.srcOffset.z = 0;
    resolveRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.dstSubresource.mipLevel = 0;
    resolveRegion.dstSubresource.baseArrayLayer = 0;
    resolveRegion.dstSubresource.layerCount = 1;
    resolveRegion.dstOffset.x = 0;
    resolveRegion.dstOffset.y = 0;
    resolveRegion.dstOffset.z = 0;
    resolveRegion.extent.width = 1;
    resolveRegion.extent.height = 1;
    resolveRegion.extent.depth = 1;
    m_commandBuffer->ResolveImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &resolveRegion);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, ResolveImageHighSampleCount) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdResolveImage called with dest sample count greater than 1.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create two images of sample count 4 and try to Resolve between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 1;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_4_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    // Note: Some implementations expect color attachment usage for any
    // multisample surface
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    // Note: Some implementations expect color attachment usage for any
    // multisample surface
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    // Need memory barrier to VK_IMAGE_LAYOUT_GENERAL for source and dest?
    // VK_IMAGE_LAYOUT_UNDEFINED = 0,
    // VK_IMAGE_LAYOUT_GENERAL = 1,
    VkImageResolve resolveRegion;
    resolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.srcSubresource.mipLevel = 0;
    resolveRegion.srcSubresource.baseArrayLayer = 0;
    resolveRegion.srcSubresource.layerCount = 1;
    resolveRegion.srcOffset.x = 0;
    resolveRegion.srcOffset.y = 0;
    resolveRegion.srcOffset.z = 0;
    resolveRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.dstSubresource.mipLevel = 0;
    resolveRegion.dstSubresource.baseArrayLayer = 0;
    resolveRegion.dstSubresource.layerCount = 1;
    resolveRegion.dstOffset.x = 0;
    resolveRegion.dstOffset.y = 0;
    resolveRegion.dstOffset.z = 0;
    resolveRegion.extent.width = 1;
    resolveRegion.extent.height = 1;
    resolveRegion.extent.depth = 1;
    m_commandBuffer->ResolveImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &resolveRegion);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, ResolveImageFormatMismatch) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdResolveImage called with unmatched source and dest formats.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create two images of different types and try to copy between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 1;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_2_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    // Note: Some implementations expect color attachment usage for any
    // multisample surface
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    // Set format to something other than source image
    image_create_info.format = VK_FORMAT_R32_SFLOAT;
    // Note: Some implementations expect color attachment usage for any
    // multisample surface
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    // Need memory barrier to VK_IMAGE_LAYOUT_GENERAL for source and dest?
    // VK_IMAGE_LAYOUT_UNDEFINED = 0,
    // VK_IMAGE_LAYOUT_GENERAL = 1,
    VkImageResolve resolveRegion;
    resolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.srcSubresource.mipLevel = 0;
    resolveRegion.srcSubresource.baseArrayLayer = 0;
    resolveRegion.srcSubresource.layerCount = 1;
    resolveRegion.srcOffset.x = 0;
    resolveRegion.srcOffset.y = 0;
    resolveRegion.srcOffset.z = 0;
    resolveRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.dstSubresource.mipLevel = 0;
    resolveRegion.dstSubresource.baseArrayLayer = 0;
    resolveRegion.dstSubresource.layerCount = 1;
    resolveRegion.dstOffset.x = 0;
    resolveRegion.dstOffset.y = 0;
    resolveRegion.dstOffset.z = 0;
    resolveRegion.extent.width = 1;
    resolveRegion.extent.height = 1;
    resolveRegion.extent.depth = 1;
    m_commandBuffer->ResolveImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &resolveRegion);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, ResolveImageTypeMismatch) {
    VkResult err;
    bool pass;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdResolveImage called with unmatched source and dest image types.");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create two images of different types and try to copy between them
    VkImage srcImage;
    VkImage dstImage;
    VkDeviceMemory srcMem;
    VkDeviceMemory destMem;
    VkMemoryRequirements memReqs;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 1;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_2_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    // Note: Some implementations expect color attachment usage for any
    // multisample surface
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &srcImage);
    ASSERT_VK_SUCCESS(err);

    image_create_info.imageType = VK_IMAGE_TYPE_1D;
    // Note: Some implementations expect color attachment usage for any
    // multisample surface
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &dstImage);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    vkGetImageMemoryRequirements(m_device->device(), srcImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &srcMem);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), dstImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &memAlloc, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memAlloc, NULL, &destMem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindImageMemory(m_device->device(), srcImage, srcMem, 0);
    ASSERT_VK_SUCCESS(err);
    err = vkBindImageMemory(m_device->device(), dstImage, destMem, 0);
    ASSERT_VK_SUCCESS(err);

    BeginCommandBuffer();
    // Need memory barrier to VK_IMAGE_LAYOUT_GENERAL for source and dest?
    // VK_IMAGE_LAYOUT_UNDEFINED = 0,
    // VK_IMAGE_LAYOUT_GENERAL = 1,
    VkImageResolve resolveRegion;
    resolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.srcSubresource.mipLevel = 0;
    resolveRegion.srcSubresource.baseArrayLayer = 0;
    resolveRegion.srcSubresource.layerCount = 1;
    resolveRegion.srcOffset.x = 0;
    resolveRegion.srcOffset.y = 0;
    resolveRegion.srcOffset.z = 0;
    resolveRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.dstSubresource.mipLevel = 0;
    resolveRegion.dstSubresource.baseArrayLayer = 0;
    resolveRegion.dstSubresource.layerCount = 1;
    resolveRegion.dstOffset.x = 0;
    resolveRegion.dstOffset.y = 0;
    resolveRegion.dstOffset.z = 0;
    resolveRegion.extent.width = 1;
    resolveRegion.extent.height = 1;
    resolveRegion.extent.depth = 1;
    m_commandBuffer->ResolveImage(srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &resolveRegion);
    EndCommandBuffer();

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), srcImage, NULL);
    vkDestroyImage(m_device->device(), dstImage, NULL);
    vkFreeMemory(m_device->device(), srcMem, NULL);
    vkFreeMemory(m_device->device(), destMem, NULL);
}

TEST_F(VkLayerTest, DepthStencilImageViewWithColorAspectBitError) {
    // Create a single Image descriptor and cause it to first hit an error due
    //  to using a DS format, then cause it to hit error due to COLOR_BIT not
    //  set in aspect
    // The image format check comes 2nd in validation so we trigger it first,
    //  then when we cause aspect fail next, bad format check will be preempted
    VkResult err;

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "Combination depth/stencil image formats can have only the ");

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    ds_type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding = 0;
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    dsl_binding.descriptorCount = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = 1;
    ds_layout_ci.pBindings = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkImage image_bad;
    VkImage image_good;
    // One bad format and one good format for Color attachment
    const VkFormat tex_format_bad = VK_FORMAT_D24_UNORM_S8_UINT;
    const VkFormat tex_format_good = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 32;
    const int32_t tex_height = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = tex_format_bad;
    image_create_info.extent.width = tex_width;
    image_create_info.extent.height = tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.flags = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image_bad);
    ASSERT_VK_SUCCESS(err);
    image_create_info.format = tex_format_good;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image_good);
    ASSERT_VK_SUCCESS(err);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_view_create_info.image = image_bad;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = tex_format_bad;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageView view;
    err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);

    m_errorMonitor->VerifyFound();

    vkDestroyImage(m_device->device(), image_bad, NULL);
    vkDestroyImage(m_device->device(), image_good, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

TEST_F(VkLayerTest, ClearImageErrors) {
    TEST_DESCRIPTION("Call ClearColorImage w/ a depth|stencil image and "
                     "ClearDepthStencilImage with a color image.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // Renderpass is started here so end it as Clear cmds can't be in renderpass
    BeginCommandBuffer();
    m_commandBuffer->EndRenderPass();

    // Color image
    VkClearColorValue clear_color;
    memset(clear_color.uint32, 0, sizeof(uint32_t) * 4);
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t img_width = 32;
    const int32_t img_height = 32;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = color_format;
    image_create_info.extent.width = img_width;
    image_create_info.extent.height = img_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    vk_testing::Image color_image;
    color_image.init(*m_device, (const VkImageCreateInfo &)image_create_info, reqs);

    const VkImageSubresourceRange color_range = vk_testing::Image::subresource_range(image_create_info, VK_IMAGE_ASPECT_COLOR_BIT);

    // Depth/Stencil image
    VkClearDepthStencilValue clear_value = {0};
    reqs = 0; // don't need HOST_VISIBLE DS image
    VkImageCreateInfo ds_image_create_info = vk_testing::Image::create_info();
    ds_image_create_info.imageType = VK_IMAGE_TYPE_2D;
    ds_image_create_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    ds_image_create_info.extent.width = 64;
    ds_image_create_info.extent.height = 64;
    ds_image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    ds_image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    vk_testing::Image ds_image;
    ds_image.init(*m_device, (const VkImageCreateInfo &)ds_image_create_info, reqs);

    const VkImageSubresourceRange ds_range = vk_testing::Image::subresource_range(ds_image_create_info, VK_IMAGE_ASPECT_DEPTH_BIT);

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdClearColorImage called with depth/stencil image.");

    vkCmdClearColorImage(m_commandBuffer->GetBufferHandle(), ds_image.handle(), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1,
                         &color_range);

    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "vkCmdClearColorImage called with "
                                                                        "image created without "
                                                                        "VK_IMAGE_USAGE_TRANSFER_DST_BIT");

    vkCmdClearColorImage(m_commandBuffer->GetBufferHandle(), ds_image.handle(), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1,
                         &color_range);

    m_errorMonitor->VerifyFound();

    // Call CmdClearDepthStencilImage with color image
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         "vkCmdClearDepthStencilImage called without a depth/stencil image.");

    vkCmdClearDepthStencilImage(m_commandBuffer->GetBufferHandle(), color_image.handle(),
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &clear_value, 1, &ds_range);

    m_errorMonitor->VerifyFound();
}
#endif // IMAGE_TESTS


// WSI Enabled Tests
//
TEST_F(VkWsiEnabledLayerTest, TestEnabledWsi) {

#if defined(VK_USE_PLATFORM_XCB_KHR)
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkResult err;
    bool pass;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    //    uint32_t swapchain_image_count = 0;
    //    VkImage swapchain_images[1] = {VK_NULL_HANDLE};
    //    uint32_t image_index = 0;
    //    VkPresentInfoKHR present_info = {};

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Use the create function from one of the VK_KHR_*_surface extension in
    // order to create a surface, testing all known errors in the process,
    // before successfully creating a surface:
    // First, try to create a surface without a VkXcbSurfaceCreateInfoKHR:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pCreateInfo specified as NULL");
    err = vkCreateXcbSurfaceKHR(instance(), NULL, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, try to create a surface with the wrong
    // VkXcbSurfaceCreateInfoKHR::sType:
    VkXcbSurfaceCreateInfoKHR xcb_create_info = {};
    xcb_create_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "parameter pCreateInfo->sType must be");
    err = vkCreateXcbSurfaceKHR(instance(), &xcb_create_info, NULL, &surface);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Create a native window, and then correctly create a surface:
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t xcb_window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;

    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;
    uint32_t value_mask, value_list[32];
    int width = 1;
    int height = 1;

    connection = xcb_connect(NULL, &scr);
    ASSERT_TRUE(connection != NULL);
    setup = xcb_get_setup(connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);
    screen = iter.data;

    xcb_window = xcb_generate_id(connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(connection, XCB_COPY_FROM_PARENT, xcb_window, screen->root, 0, 0, width, height, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
    atom_wm_delete_window = xcb_intern_atom_reply(connection, cookie2, 0);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, xcb_window, (*reply).atom, 4, 32, 1, &(*atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(connection, xcb_window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = { 100, 100 };
    xcb_configure_window(connection, xcb_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);

    // Finally, try to correctly create a surface:
    xcb_create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_create_info.pNext = NULL;
    xcb_create_info.flags = 0;
    xcb_create_info.connection = connection;
    xcb_create_info.window = xcb_window;
    err = vkCreateXcbSurfaceKHR(instance(), &xcb_create_info, NULL, &surface);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);

    // Check if surface supports presentation:

    // 1st, do so without having queried the queue families:
    VkBool32 supported = false;
    // TODO: Get the following error to come out:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "called before calling the vkGetPhysicalDeviceQueueFamilyProperties "
        "function");
    err = vkGetPhysicalDeviceSurfaceSupportKHR(gpu(), 0, surface, &supported);
    pass = (err != VK_SUCCESS);
    //    ASSERT_TRUE(pass);
    //    m_errorMonitor->VerifyFound();

    // Next, query a queue family index that's too large:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "called with a queueFamilyIndex that is too large");
    err = vkGetPhysicalDeviceSurfaceSupportKHR(gpu(), 100000, surface, &supported);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Finally, do so correctly:
    // FIXME: THIS ISN'T CORRECT--MUST QUERY UNTIL WE FIND A QUEUE FAMILY THAT'S
    // SUPPORTED
    err = vkGetPhysicalDeviceSurfaceSupportKHR(gpu(), 0, surface, &supported);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);

    // Before proceeding, try to create a swapchain without having called
    // vkGetPhysicalDeviceSurfaceCapabilitiesKHR():
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.surface = surface;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "called before calling vkGetPhysicalDeviceSurfaceCapabilitiesKHR().");
    err = vkCreateSwapchainKHR(m_device->device(), &swapchain_create_info, NULL, &swapchain);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Get the surface capabilities:
    VkSurfaceCapabilitiesKHR surface_capabilities;

    // Do so correctly (only error logged by this entrypoint is if the
    // extension isn't enabled):
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu(), surface, &surface_capabilities);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);

    // Get the surface formats:
    uint32_t surface_format_count;

    // First, try without a pointer to surface_format_count:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pSurfaceFormatCount "
        "specified as NULL");
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, NULL, NULL);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, call with a non-NULL pSurfaceFormats, even though we haven't
    // correctly done a 1st try (to get the count):
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, "but no prior positive value has been seen for");
    surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, &surface_format_count, (VkSurfaceFormatKHR *)&surface_format_count);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, correctly do a 1st try (with a NULL pointer to surface_formats):
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, &surface_format_count, NULL);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);

    // Allocate memory for the correct number of VkSurfaceFormatKHR's:
    VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(surface_format_count * sizeof(VkSurfaceFormatKHR));

    // Next, do a 2nd try with surface_format_count being set too high:
    surface_format_count += 5;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "that is greater than the value");
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, &surface_format_count, surface_formats);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Finally, do a correct 1st and 2nd try:
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, &surface_format_count, NULL);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu(), surface, &surface_format_count, surface_formats);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);

    // Get the surface present modes:
    uint32_t surface_present_mode_count;

    // First, try without a pointer to surface_format_count:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pPresentModeCount "
        "specified as NULL");

    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu(), surface, NULL, NULL);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, call with a non-NULL VkPresentModeKHR, even though we haven't
    // correctly done a 1st try (to get the count):
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_WARNING_BIT_EXT, "but no prior positive value has been seen for");
    surface_present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu(), surface, &surface_present_mode_count,
        (VkPresentModeKHR *)&surface_present_mode_count);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, correctly do a 1st try (with a NULL pointer to surface_formats):
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu(), surface, &surface_present_mode_count, NULL);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);

    // Allocate memory for the correct number of VkSurfaceFormatKHR's:
    VkPresentModeKHR *surface_present_modes = (VkPresentModeKHR *)malloc(surface_present_mode_count * sizeof(VkPresentModeKHR));

    // Next, do a 2nd try with surface_format_count being set too high:
    surface_present_mode_count += 5;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "that is greater than the value");
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu(), surface, &surface_present_mode_count, surface_present_modes);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Finally, do a correct 1st and 2nd try:
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu(), surface, &surface_present_mode_count, NULL);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu(), surface, &surface_present_mode_count, surface_present_modes);
    pass = (err == VK_SUCCESS);
    ASSERT_TRUE(pass);

    // Create a swapchain:

    // First, try without a pointer to swapchain_create_info:
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pCreateInfo "
        "specified as NULL");

    err = vkCreateSwapchainKHR(m_device->device(), NULL, NULL, &swapchain);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, call with a non-NULL swapchain_create_info, that has the wrong
    // sType:
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "parameter pCreateInfo->sType must be");

    err = vkCreateSwapchainKHR(m_device->device(), &swapchain_create_info, NULL, &swapchain);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, call with a NULL swapchain pointer:
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "required parameter pSwapchain "
        "specified as NULL");

    err = vkCreateSwapchainKHR(m_device->device(), &swapchain_create_info, NULL, NULL);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // TODO: Enhance swapchain layer so that
    // swapchain_create_info.queueFamilyIndexCount is checked against something?

    // Next, call with a queue family index that's too large:
    uint32_t queueFamilyIndex[2] = { 100000, 0 };
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queueFamilyIndex;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "called with a queueFamilyIndex that is too large");
    err = vkCreateSwapchainKHR(m_device->device(), &swapchain_create_info, NULL, &swapchain);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, call a queueFamilyIndexCount that's too small for CONCURRENT:
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 1;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "but with a bad value(s) for pCreateInfo->queueFamilyIndexCount or "
        "pCreateInfo->pQueueFamilyIndices).");
    err = vkCreateSwapchainKHR(m_device->device(), &swapchain_create_info, NULL, &swapchain);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();

    // Next, call with an invalid imageSharingMode:
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_MAX_ENUM;
    swapchain_create_info.queueFamilyIndexCount = 1;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT,
        "called with a non-supported pCreateInfo->imageSharingMode (i.e.");
    err = vkCreateSwapchainKHR(m_device->device(), &swapchain_create_info, NULL, &swapchain);
    pass = (err != VK_SUCCESS);
    ASSERT_TRUE(pass);
    m_errorMonitor->VerifyFound();
    // Fix for the future:
    // FIXME: THIS ISN'T CORRECT--MUST QUERY UNTIL WE FIND A QUEUE FAMILY THAT'S
    // SUPPORTED
    swapchain_create_info.queueFamilyIndexCount = 0;
    queueFamilyIndex[0] = 0;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // TODO: CONTINUE TESTING VALIDATION OF vkCreateSwapchainKHR() ...
    // Get the images from a swapchain:
    // Acquire an image from a swapchain:
    // Present an image to a swapchain:
    // Destroy the swapchain:

    // TODOs:
    //
    // - Try destroying the device without first destroying the swapchain
    //
    // - Try destroying the device without first destroying the surface
    //
    // - Try destroying the surface without first destroying the swapchain

    // Destroy the surface:
    vkDestroySurfaceKHR(instance(), surface, NULL);

    // Tear down the window:
    xcb_destroy_window(connection, xcb_window);
    xcb_disconnect(connection);

#else  // VK_USE_PLATFORM_XCB_KHR
    return;
#endif // VK_USE_PLATFORM_XCB_KHR
}

//
// POSITIVE VALIDATION TESTS
//
// These tests do not expect to encounter ANY validation errors pass only if this is true

// This is a positive test. No failures are expected.
TEST_F(VkPositiveLayerTest, IgnoreUnrelatedDescriptor) {
    TEST_DESCRIPTION("Ensure that the vkUpdateDescriptorSets validation code "
        "is ignoring VkWriteDescriptorSet members that are not "
        "related to the descriptor type specified by "
        "VkWriteDescriptorSet::descriptorType.  Correct "
        "validation behavior will result in the test running to "
        "completion without validation errors.");

    const uintptr_t invalid_ptr = 0xcdcdcdcd;

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Image Case
    {
        m_errorMonitor->ExpectSuccess();

        VkImage image;
        const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
        const int32_t tex_width = 32;
        const int32_t tex_height = 32;
        VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = NULL;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = tex_format;
        image_create_info.extent.width = tex_width;
        image_create_info.extent.height = tex_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.flags = 0;
        VkResult err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
        ASSERT_VK_SUCCESS(err);

        VkMemoryRequirements memory_reqs;
        VkDeviceMemory image_memory;
        bool pass;
        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.pNext = NULL;
        memory_info.allocationSize = 0;
        memory_info.memoryTypeIndex = 0;
        vkGetImageMemoryRequirements(m_device->device(), image, &memory_reqs);
        memory_info.allocationSize = memory_reqs.size;
        pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
        ASSERT_TRUE(pass);
        err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &image_memory);
        ASSERT_VK_SUCCESS(err);
        err = vkBindImageMemory(m_device->device(), image, image_memory, 0);
        ASSERT_VK_SUCCESS(err);

        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = image;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = tex_format;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageView view;
        err = vkCreateImageView(m_device->device(), &image_view_create_info, NULL, &view);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorPoolSize ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        ds_type_count.descriptorCount = 1;

        VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.maxSets = 1;
        ds_pool_ci.poolSizeCount = 1;
        ds_pool_ci.pPoolSizes = &ds_type_count;

        VkDescriptorPool ds_pool;
        err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.binding = 0;
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        dsl_binding.descriptorCount = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.bindingCount = 1;
        ds_layout_ci.pBindings = &dsl_binding;
        VkDescriptorSetLayout ds_layout;
        err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorSet descriptor_set;
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorSetCount = 1;
        alloc_info.descriptorPool = ds_pool;
        alloc_info.pSetLayouts = &ds_layout;
        err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorImageInfo image_info = {};
        image_info.imageView = view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet descriptor_write;
        memset(&descriptor_write, 0, sizeof(descriptor_write));
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_set;
        descriptor_write.dstBinding = 0;
        descriptor_write.descriptorCount = 1;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptor_write.pImageInfo = &image_info;

        // Set pBufferInfo and pTexelBufferView to invalid values, which should
        // be
        //  ignored for descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE.
        // This will most likely produce a crash if the parameter_validation
        // layer
        // does not correctly ignore pBufferInfo.
        descriptor_write.pBufferInfo = reinterpret_cast<const VkDescriptorBufferInfo *>(invalid_ptr);
        descriptor_write.pTexelBufferView = reinterpret_cast<const VkBufferView *>(invalid_ptr);

        vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

        m_errorMonitor->VerifyNotFound();

        vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
        vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
        vkDestroyImageView(m_device->device(), view, NULL);
        vkDestroyImage(m_device->device(), image, NULL);
        vkFreeMemory(m_device->device(), image_memory, NULL);
    }

    // Buffer Case
    {
        m_errorMonitor->ExpectSuccess();

        VkBuffer buffer;
        uint32_t queue_family_index = 0;
        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.size = 1024;
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.queueFamilyIndexCount = 1;
        buffer_create_info.pQueueFamilyIndices = &queue_family_index;

        VkResult err = vkCreateBuffer(m_device->device(), &buffer_create_info, NULL, &buffer);
        ASSERT_VK_SUCCESS(err);

        VkMemoryRequirements memory_reqs;
        VkDeviceMemory buffer_memory;
        bool pass;
        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.pNext = NULL;
        memory_info.allocationSize = 0;
        memory_info.memoryTypeIndex = 0;

        vkGetBufferMemoryRequirements(m_device->device(), buffer, &memory_reqs);
        memory_info.allocationSize = memory_reqs.size;
        pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
        ASSERT_TRUE(pass);

        err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &buffer_memory);
        ASSERT_VK_SUCCESS(err);
        err = vkBindBufferMemory(m_device->device(), buffer, buffer_memory, 0);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorPoolSize ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.descriptorCount = 1;

        VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.maxSets = 1;
        ds_pool_ci.poolSizeCount = 1;
        ds_pool_ci.pPoolSizes = &ds_type_count;

        VkDescriptorPool ds_pool;
        err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.binding = 0;
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.descriptorCount = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.bindingCount = 1;
        ds_layout_ci.pBindings = &dsl_binding;
        VkDescriptorSetLayout ds_layout;
        err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorSet descriptor_set;
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorSetCount = 1;
        alloc_info.descriptorPool = ds_pool;
        alloc_info.pSetLayouts = &ds_layout;
        err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = 1024;

        VkWriteDescriptorSet descriptor_write;
        memset(&descriptor_write, 0, sizeof(descriptor_write));
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_set;
        descriptor_write.dstBinding = 0;
        descriptor_write.descriptorCount = 1;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.pBufferInfo = &buffer_info;

        // Set pImageInfo and pTexelBufferView to invalid values, which should
        // be
        //  ignored for descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER.
        // This will most likely produce a crash if the parameter_validation
        // layer
        // does not correctly ignore pImageInfo.
        descriptor_write.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo *>(invalid_ptr);
        descriptor_write.pTexelBufferView = reinterpret_cast<const VkBufferView *>(invalid_ptr);

        vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

        m_errorMonitor->VerifyNotFound();

        vkFreeDescriptorSets(m_device->device(), ds_pool, 1, &descriptor_set);
        vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
        vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        vkFreeMemory(m_device->device(), buffer_memory, NULL);
    }

    // Texel Buffer Case
    {
        m_errorMonitor->ExpectSuccess();

        VkBuffer buffer;
        uint32_t queue_family_index = 0;
        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.size = 1024;
        buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        buffer_create_info.queueFamilyIndexCount = 1;
        buffer_create_info.pQueueFamilyIndices = &queue_family_index;

        VkResult err = vkCreateBuffer(m_device->device(), &buffer_create_info, NULL, &buffer);
        ASSERT_VK_SUCCESS(err);

        VkMemoryRequirements memory_reqs;
        VkDeviceMemory buffer_memory;
        bool pass;
        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.pNext = NULL;
        memory_info.allocationSize = 0;
        memory_info.memoryTypeIndex = 0;

        vkGetBufferMemoryRequirements(m_device->device(), buffer, &memory_reqs);
        memory_info.allocationSize = memory_reqs.size;
        pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
        ASSERT_TRUE(pass);

        err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &buffer_memory);
        ASSERT_VK_SUCCESS(err);
        err = vkBindBufferMemory(m_device->device(), buffer, buffer_memory, 0);
        ASSERT_VK_SUCCESS(err);

        VkBufferViewCreateInfo buff_view_ci = {};
        buff_view_ci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        buff_view_ci.buffer = buffer;
        buff_view_ci.format = VK_FORMAT_R8_UNORM;
        buff_view_ci.range = VK_WHOLE_SIZE;
        VkBufferView buffer_view;
        err = vkCreateBufferView(m_device->device(), &buff_view_ci, NULL, &buffer_view);

        VkDescriptorPoolSize ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        ds_type_count.descriptorCount = 1;

        VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.maxSets = 1;
        ds_pool_ci.poolSizeCount = 1;
        ds_pool_ci.pPoolSizes = &ds_type_count;

        VkDescriptorPool ds_pool;
        err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.binding = 0;
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        dsl_binding.descriptorCount = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.bindingCount = 1;
        ds_layout_ci.pBindings = &dsl_binding;
        VkDescriptorSetLayout ds_layout;
        err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
        ASSERT_VK_SUCCESS(err);

        VkDescriptorSet descriptor_set;
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorSetCount = 1;
        alloc_info.descriptorPool = ds_pool;
        alloc_info.pSetLayouts = &ds_layout;
        err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
        ASSERT_VK_SUCCESS(err);

        VkWriteDescriptorSet descriptor_write;
        memset(&descriptor_write, 0, sizeof(descriptor_write));
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_set;
        descriptor_write.dstBinding = 0;
        descriptor_write.descriptorCount = 1;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        descriptor_write.pTexelBufferView = &buffer_view;

        // Set pImageInfo and pBufferInfo to invalid values, which should be
        //  ignored for descriptorType ==
        //  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER.
        // This will most likely produce a crash if the parameter_validation
        // layer
        // does not correctly ignore pImageInfo and pBufferInfo.
        descriptor_write.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo *>(invalid_ptr);
        descriptor_write.pBufferInfo = reinterpret_cast<const VkDescriptorBufferInfo *>(invalid_ptr);

        vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

        m_errorMonitor->VerifyNotFound();

        vkFreeDescriptorSets(m_device->device(), ds_pool, 1, &descriptor_set);
        vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
        vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
        vkDestroyBufferView(m_device->device(), buffer_view, NULL);
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        vkFreeMemory(m_device->device(), buffer_memory, NULL);
    }
}

TEST_F(VkLayerTest, DuplicateDescriptorBinding) {
    TEST_DESCRIPTION("Create a descriptor set layout with a duplicate binding number.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    // Create layout where two binding #s are "1"
    static const uint32_t NUM_BINDINGS = 3;
    VkDescriptorSetLayoutBinding dsl_binding[NUM_BINDINGS] = {};
    dsl_binding[0].binding = 1;
    dsl_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding[0].descriptorCount = 1;
    dsl_binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    dsl_binding[0].pImmutableSamplers = NULL;
    dsl_binding[1].binding = 0;
    dsl_binding[1].descriptorCount = 1;
    dsl_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding[1].descriptorCount = 1;
    dsl_binding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    dsl_binding[1].pImmutableSamplers = NULL;
    dsl_binding[2].binding = 1; // Duplicate binding should cause error
    dsl_binding[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding[2].descriptorCount = 1;
    dsl_binding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    dsl_binding[2].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = NUM_BINDINGS;
    ds_layout_ci.pBindings = dsl_binding;
    VkDescriptorSetLayout ds_layout;
    m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, VALIDATION_ERROR_02345);
    vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    m_errorMonitor->VerifyFound();
}

// This is a positive test. No failures are expected.
TEST_F(VkPositiveLayerTest, EmptyDescriptorUpdateTest) {
    TEST_DESCRIPTION("Update last descriptor in a set that includes an empty binding");
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ExpectSuccess();
    VkDescriptorPoolSize ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.descriptorCount = 2;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.maxSets = 1;
    ds_pool_ci.poolSizeCount = 1;
    ds_pool_ci.pPoolSizes = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), &ds_pool_ci, NULL, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    // Create layout with two uniform buffer descriptors w/ empty binding between them
    static const uint32_t NUM_BINDINGS = 3;
    VkDescriptorSetLayoutBinding dsl_binding[NUM_BINDINGS] = {};
    dsl_binding[0].binding = 0;
    dsl_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding[0].descriptorCount = 1;
    dsl_binding[0].stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding[0].pImmutableSamplers = NULL;
    dsl_binding[1].binding = 1;
    dsl_binding[1].descriptorCount = 0; // empty binding
    dsl_binding[2].binding = 2;
    dsl_binding[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding[2].descriptorCount = 1;
    dsl_binding[2].stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding[2].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.bindingCount = NUM_BINDINGS;
    ds_layout_ci.pBindings = dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, NULL, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptor_set = {};
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorSetCount = 1;
    alloc_info.descriptorPool = ds_pool;
    alloc_info.pSetLayouts = &ds_layout;
    err = vkAllocateDescriptorSets(m_device->device(), &alloc_info, &descriptor_set);
    ASSERT_VK_SUCCESS(err);

    // Create a buffer to be used for update
    VkBufferCreateInfo buff_ci = {};
    buff_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buff_ci.size = 256;
    buff_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer;
    err = vkCreateBuffer(m_device->device(), &buff_ci, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);
    // Have to bind memory to buffer before descriptor update
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 512; // one allocation for both buffers
    mem_alloc.memoryTypeIndex = 0;

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(m_device->device(), buffer, &mem_reqs);
    bool pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }

    VkDeviceMemory mem;
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);

    // Only update the descriptor at binding 2
    VkDescriptorBufferInfo buff_info = {};
    buff_info.buffer = buffer;
    buff_info.offset = 0;
    buff_info.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstBinding = 2;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pTexelBufferView = nullptr;
    descriptor_write.pBufferInfo = &buff_info;
    descriptor_write.pImageInfo = nullptr;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.dstSet = descriptor_set;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    m_errorMonitor->VerifyNotFound();
    // Cleanup
    vkFreeMemory(m_device->device(), mem, NULL);
    vkDestroyDescriptorSetLayout(m_device->device(), ds_layout, NULL);
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkDestroyDescriptorPool(m_device->device(), ds_pool, NULL);
}

// This is a positive test. No failures are expected.
TEST_F(VkPositiveLayerTest, TestAliasedMemoryTracking) {
    VkResult err;
    bool pass;

    TEST_DESCRIPTION("Create a buffer, allocate memory, bind memory, destroy "
        "the buffer, create an image, and bind the same memory to "
        "it");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkBuffer buffer;
    VkImage image;
    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = 256;
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    err = vkCreateBuffer(m_device->device(), &buf_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);

    vkGetBufferMemoryRequirements(m_device->device(), buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    // Ensure memory is big enough for both bindings
    alloc_info.allocationSize = 0x10000;

    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }

    err = vkAllocateMemory(m_device->device(), &alloc_info, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    uint8_t *pData;
    err = vkMapMemory(m_device->device(), mem, 0, mem_reqs.size, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);

    memset(pData, 0xCADECADE, static_cast<size_t>(mem_reqs.size));

    vkUnmapMemory(m_device->device(), mem);

    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);

    // NOW, destroy the buffer. Obviously, the resource no longer occupies this
    // memory. In fact, it was never used by the GPU.
    // Just be be sure, wait for idle.
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkDeviceWaitIdle(m_device->device());

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_create_info.extent.width = 64;
    image_create_info.extent.height = 64;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.flags = 0;

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    /* Create a mappable image.  It will be the texture if linear images are ok
    * to be textures or it will be the staging image if they are not.
    */
    err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    vkGetImageMemoryRequirements(m_device->device(), image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;

    pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (!pass) {
        vkDestroyImage(m_device->device(), image, NULL);
        return;
    }

    // VALIDATION FAILURE:
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    ASSERT_VK_SUCCESS(err);

    m_errorMonitor->VerifyNotFound();

    vkFreeMemory(m_device->device(), mem, NULL);
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkDestroyImage(m_device->device(), image, NULL);
}

TEST_F(VkPositiveLayerTest, NonCoherentMemoryMapping) {

    TEST_DESCRIPTION("Ensure that validations handling of non-coherent memory "
        "mapping while using VK_WHOLE_SIZE does not cause access "
        "violations");
    VkResult err;
    uint8_t *pData;
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDeviceMemory mem;
    VkMemoryRequirements mem_reqs;
    mem_reqs.memoryTypeBits = 0xFFFFFFFF;
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    static const VkDeviceSize allocation_size = 0x1000;
    alloc_info.allocationSize = allocation_size;

    // Find a memory configurations WITHOUT a COHERENT bit, otherwise exit
    bool pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!pass) {
        pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!pass) {
            pass = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &alloc_info,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (!pass) {
                return;
            }
        }
    }

    err = vkAllocateMemory(m_device->device(), &alloc_info, NULL, &mem);
    ASSERT_VK_SUCCESS(err);

    // Map/Flush/Invalidate using WHOLE_SIZE and zero offsets and entire
    // mapped range
    m_errorMonitor->ExpectSuccess();
    err = vkMapMemory(m_device->device(), mem, 0, VK_WHOLE_SIZE, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);
    VkMappedMemoryRange mmr = {};
    mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mmr.memory = mem;
    mmr.offset = 0;
    mmr.size = VK_WHOLE_SIZE;
    err = vkFlushMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    err = vkInvalidateMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->VerifyNotFound();
    vkUnmapMemory(m_device->device(), mem);

    // Map/Flush/Invalidate using WHOLE_SIZE and a prime offset and entire
    // mapped range
    m_errorMonitor->ExpectSuccess();
    err = vkMapMemory(m_device->device(), mem, 13, VK_WHOLE_SIZE, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);
    mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mmr.memory = mem;
    mmr.offset = 13;
    mmr.size = VK_WHOLE_SIZE;
    err = vkFlushMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    err = vkInvalidateMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->VerifyNotFound();
    vkUnmapMemory(m_device->device(), mem);

    // Map with prime offset and size
    // Flush/Invalidate subrange of mapped area with prime offset and size
    m_errorMonitor->ExpectSuccess();
    err = vkMapMemory(m_device->device(), mem, allocation_size - 137, 109, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);
    mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mmr.memory = mem;
    mmr.offset = allocation_size - 107;
    mmr.size = 61;
    err = vkFlushMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    err = vkInvalidateMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->VerifyNotFound();
    vkUnmapMemory(m_device->device(), mem);

    // Map without offset and flush WHOLE_SIZE with two separate offsets
    m_errorMonitor->ExpectSuccess();
    err = vkMapMemory(m_device->device(), mem, 0, VK_WHOLE_SIZE, 0, (void **)&pData);
    ASSERT_VK_SUCCESS(err);
    mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mmr.memory = mem;
    mmr.offset = allocation_size - 100;
    mmr.size = VK_WHOLE_SIZE;
    err = vkFlushMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    mmr.offset = allocation_size - 200;
    mmr.size = VK_WHOLE_SIZE;
    err = vkFlushMappedMemoryRanges(m_device->device(), 1, &mmr);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->VerifyNotFound();
    vkUnmapMemory(m_device->device(), mem);

    vkFreeMemory(m_device->device(), mem, NULL);
}

// This is a positive test. We used to expect error in this case but spec now allows it
TEST_F(VkPositiveLayerTest, ResetUnsignaledFence) {
    m_errorMonitor->ExpectSuccess();
    vk_testing::Fence testFence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;

    ASSERT_NO_FATAL_FAILURE(InitState());
    testFence.init(*m_device, fenceInfo);
    VkFence fences[1] = { testFence.handle() };
    VkResult result = vkResetFences(m_device->device(), 1, fences);
    ASSERT_VK_SUCCESS(result);

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CommandBufferSimultaneousUseSync) {
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkResult err;

    // Record (empty!) command buffer that can be submitted multiple times
    // simultaneously.
    VkCommandBufferBeginInfo cbbi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr };
    m_commandBuffer->BeginCommandBuffer(&cbbi);
    m_commandBuffer->EndCommandBuffer();

    VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
    VkFence fence;
    err = vkCreateFence(m_device->device(), &fci, nullptr, &fence);
    ASSERT_VK_SUCCESS(err);

    VkSemaphoreCreateInfo sci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    VkSemaphore s1, s2;
    err = vkCreateSemaphore(m_device->device(), &sci, nullptr, &s1);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateSemaphore(m_device->device(), &sci, nullptr, &s2);
    ASSERT_VK_SUCCESS(err);

    // Submit CB once signaling s1, with fence so we can roll forward to its retirement.
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &m_commandBuffer->handle(), 1, &s1 };
    err = vkQueueSubmit(m_device->m_queue, 1, &si, fence);
    ASSERT_VK_SUCCESS(err);

    // Submit CB again, signaling s2.
    si.pSignalSemaphores = &s2;
    err = vkQueueSubmit(m_device->m_queue, 1, &si, VK_NULL_HANDLE);
    ASSERT_VK_SUCCESS(err);

    // Wait for fence.
    err = vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    ASSERT_VK_SUCCESS(err);

    // CB is still in flight from second submission, but semaphore s1 is no
    // longer in flight. delete it.
    vkDestroySemaphore(m_device->device(), s1, nullptr);

    m_errorMonitor->VerifyNotFound();

    // Force device idle and clean up remaining objects
    vkDeviceWaitIdle(m_device->device());
    vkDestroySemaphore(m_device->device(), s2, nullptr);
    vkDestroyFence(m_device->device(), fence, nullptr);
}

TEST_F(VkPositiveLayerTest, FenceCreateSignaledWaitHandling) {
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkResult err;

    // A fence created signaled
    VkFenceCreateInfo fci1 = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    VkFence f1;
    err = vkCreateFence(m_device->device(), &fci1, nullptr, &f1);
    ASSERT_VK_SUCCESS(err);

    // A fence created not
    VkFenceCreateInfo fci2 = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
    VkFence f2;
    err = vkCreateFence(m_device->device(), &fci2, nullptr, &f2);
    ASSERT_VK_SUCCESS(err);

    // Submit the unsignaled fence
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 0, nullptr, 0, nullptr };
    err = vkQueueSubmit(m_device->m_queue, 1, &si, f2);

    // Wait on both fences, with signaled first.
    VkFence fences[] = { f1, f2 };
    vkWaitForFences(m_device->device(), 2, fences, VK_TRUE, UINT64_MAX);

    // Should have both retired!
    vkDestroyFence(m_device->device(), f1, nullptr);
    vkDestroyFence(m_device->device(), f2, nullptr);

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, ValidUsage) {
    TEST_DESCRIPTION("Verify that creating an image view from an image with valid usage "
        "doesn't generate validation errors");

    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->ExpectSuccess();
    // Verify that we can create a view with usage INPUT_ATTACHMENT
    VkImageObj image(m_device);
    image.init(128, 128, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());
    VkImageView imageView;
    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = image.handle();
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R8G8B8A8_UNORM;
    ivci.subresourceRange.layerCount = 1;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCreateImageView(m_device->device(), &ivci, NULL, &imageView);
    m_errorMonitor->VerifyNotFound();
    vkDestroyImageView(m_device->device(), imageView, NULL);
}

// This is a positive test. No failures are expected.
TEST_F(VkPositiveLayerTest, BindSparse) {
    TEST_DESCRIPTION("Bind 2 memory ranges to one image using vkQueueBindSparse, destroy the image"
        "and then free the memory");

    ASSERT_NO_FATAL_FAILURE(InitState());

    auto index = m_device->graphics_queue_node_index_;
    if (!(m_device->queue_props[index].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT))
        return;

    m_errorMonitor->ExpectSuccess();

    VkImage image;
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    image_create_info.extent.width = 64;
    image_create_info.extent.height = 64;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_STORAGE_BIT;
    image_create_info.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
    VkResult err = vkCreateImage(m_device->device(), &image_create_info, NULL, &image);
    ASSERT_VK_SUCCESS(err);

    VkMemoryRequirements memory_reqs;
    VkDeviceMemory memory_one, memory_two;
    bool pass;
    VkMemoryAllocateInfo memory_info = {};
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.pNext = NULL;
    memory_info.allocationSize = 0;
    memory_info.memoryTypeIndex = 0;
    vkGetImageMemoryRequirements(m_device->device(), image, &memory_reqs);
    // Find an image big enough to allow sparse mapping of 2 memory regions
    // Increase the image size until it is at least twice the
    // size of the required alignment, to ensure we can bind both
    // allocated memory blocks to the image on aligned offsets.
    while (memory_reqs.size < (memory_reqs.alignment * 2)) {
        vkDestroyImage(m_device->device(), image, nullptr);
        image_create_info.extent.width *= 2;
        image_create_info.extent.height *= 2;
        err = vkCreateImage(m_device->device(), &image_create_info, nullptr, &image);
        ASSERT_VK_SUCCESS(err);
        vkGetImageMemoryRequirements(m_device->device(), image, &memory_reqs);
    }
    // Allocate 2 memory regions of minimum alignment size, bind one at 0, the other
    // at the end of the first
    memory_info.allocationSize = memory_reqs.alignment;
    pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
    ASSERT_TRUE(pass);
    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &memory_one);
    ASSERT_VK_SUCCESS(err);
    err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &memory_two);
    ASSERT_VK_SUCCESS(err);
    VkSparseMemoryBind binds[2];
    binds[0].flags = 0;
    binds[0].memory = memory_one;
    binds[0].memoryOffset = 0;
    binds[0].resourceOffset = 0;
    binds[0].size = memory_info.allocationSize;
    binds[1].flags = 0;
    binds[1].memory = memory_two;
    binds[1].memoryOffset = 0;
    binds[1].resourceOffset = memory_info.allocationSize;
    binds[1].size = memory_info.allocationSize;

    VkSparseImageOpaqueMemoryBindInfo opaqueBindInfo;
    opaqueBindInfo.image = image;
    opaqueBindInfo.bindCount = 2;
    opaqueBindInfo.pBinds = binds;

    VkFence fence = VK_NULL_HANDLE;
    VkBindSparseInfo bindSparseInfo = {};
    bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
    bindSparseInfo.imageOpaqueBindCount = 1;
    bindSparseInfo.pImageOpaqueBinds = &opaqueBindInfo;

    vkQueueBindSparse(m_device->m_queue, 1, &bindSparseInfo, fence);
    vkQueueWaitIdle(m_device->m_queue);
    vkDestroyImage(m_device->device(), image, NULL);
    vkFreeMemory(m_device->device(), memory_one, NULL);
    vkFreeMemory(m_device->device(), memory_two, NULL);
    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, RenderPassInitialLayoutUndefined) {
    TEST_DESCRIPTION("Ensure that CmdBeginRenderPass with an attachment's "
        "initialLayout of VK_IMAGE_LAYOUT_UNDEFINED works when "
        "the command buffer has prior knowledge of that "
        "attachment's layout.");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    // A renderpass with one color attachment.
    VkAttachmentDescription attachment = { 0,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentReference att_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &att_ref, nullptr, nullptr, 0, nullptr };

    VkRenderPassCreateInfo rpci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &attachment, 1, &subpass, 0, nullptr };

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    // A compatible framebuffer.
    VkImageObj image(m_device);
    image.init(32, 32, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageViewCreateInfo ivci = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        image.handle(),
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY },
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    VkImageView view;
    err = vkCreateImageView(m_device->device(), &ivci, nullptr, &view);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, rp, 1, &view, 32, 32, 1 };
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    // Record a single command buffer which uses this renderpass twice. The
    // bug is triggered at the beginning of the second renderpass, when the
    // command buffer already has a layout recorded for the attachment.
    VkRenderPassBeginInfo rpbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, rp, fb,{ { 0, 0 },{ 32, 32 } }, 0, nullptr };
    BeginCommandBuffer();
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(m_commandBuffer->handle());
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    m_errorMonitor->VerifyNotFound();

    vkCmdEndRenderPass(m_commandBuffer->handle());
    EndCommandBuffer();

    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
    vkDestroyImageView(m_device->device(), view, nullptr);
}

TEST_F(VkPositiveLayerTest, FramebufferBindingDestroyCommandPool) {
    TEST_DESCRIPTION("This test should pass. Create a Framebuffer and "
        "command buffer, bind them together, then destroy "
        "command pool and framebuffer and verify there are no "
        "errors.");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    // A renderpass with one color attachment.
    VkAttachmentDescription attachment = { 0,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentReference att_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &att_ref, nullptr, nullptr, 0, nullptr };

    VkRenderPassCreateInfo rpci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &attachment, 1, &subpass, 0, nullptr };

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    // A compatible framebuffer.
    VkImageObj image(m_device);
    image.init(32, 32, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageViewCreateInfo ivci = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        image.handle(),
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY },
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    VkImageView view;
    err = vkCreateImageView(m_device->device(), &ivci, nullptr, &view);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, rp, 1, &view, 32, 32, 1 };
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    // Explicitly create a command buffer to bind the FB to so that we can then
    //  destroy the command pool in order to implicitly free command buffer
    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, &command_buffer);

    // Begin our cmd buffer with renderpass using our framebuffer
    VkRenderPassBeginInfo rpbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, rp, fb,{ { 0, 0 },{ 32, 32 } }, 0, nullptr };
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    vkCmdBeginRenderPass(command_buffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffer);
    vkEndCommandBuffer(command_buffer);
    vkDestroyImageView(m_device->device(), view, nullptr);
    // Destroy command pool to implicitly free command buffer
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);
    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, RenderPassSubpassZeroTransitionsApplied) {
    TEST_DESCRIPTION("Ensure that CmdBeginRenderPass applies the layout "
        "transitions for the first subpass");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    // A renderpass with one color attachment.
    VkAttachmentDescription attachment = { 0,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentReference att_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &att_ref, nullptr, nullptr, 0, nullptr };

    VkSubpassDependency dep = { 0,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT };

    VkRenderPassCreateInfo rpci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &attachment, 1, &subpass, 1, &dep };

    VkResult err;
    VkRenderPass rp;
    err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    // A compatible framebuffer.
    VkImageObj image(m_device);
    image.init(32, 32, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());

    VkImageViewCreateInfo ivci = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        image.handle(),
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY },
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    VkImageView view;
    err = vkCreateImageView(m_device->device(), &ivci, nullptr, &view);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, rp, 1, &view, 32, 32, 1 };
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    // Record a single command buffer which issues a pipeline barrier w/
    // image memory barrier for the attachment. This detects the previously
    // missing tracking of the subpass layout by throwing a validation error
    // if it doesn't occur.
    VkRenderPassBeginInfo rpbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, rp, fb,{ { 0, 0 },{ 32, 32 } }, 0, nullptr };
    BeginCommandBuffer();
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    VkImageMemoryBarrier imb = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    vkCmdPipelineBarrier(m_commandBuffer->handle(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1,
        &imb);

    vkCmdEndRenderPass(m_commandBuffer->handle());
    m_errorMonitor->VerifyNotFound();
    EndCommandBuffer();

    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
    vkDestroyImageView(m_device->device(), view, nullptr);
}

TEST_F(VkPositiveLayerTest, DepthStencilLayoutTransitionForDepthOnlyImageview) {
    TEST_DESCRIPTION("Validate that when an imageView of a depth/stencil image "
        "is used as a depth/stencil framebuffer attachment, the "
        "aspectMask is ignored and both depth and stencil image "
        "subresources are used.");

    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(gpu(), VK_FORMAT_D32_SFLOAT_S8_UINT, &format_properties);
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        return;
    }

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkAttachmentDescription attachment = { 0,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkAttachmentReference att_ref = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 0, nullptr, nullptr, &att_ref, 0, nullptr };

    VkSubpassDependency dep = { 0,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT};

    VkRenderPassCreateInfo rpci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &attachment, 1, &subpass, 1, &dep };

    VkResult err;
    VkRenderPass rp;
    err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    VkImageObj image(m_device);
    image.init_no_layout(32, 32, VK_FORMAT_D32_SFLOAT_S8_UINT,
        0x26, // usage
        VK_IMAGE_TILING_OPTIMAL, 0);
    ASSERT_TRUE(image.initialized());
    image.SetLayout(0x6, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkImageViewCreateInfo ivci = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        image.handle(),
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
        { 0x2, 0, 1, 0, 1 },
    };
    VkImageView view;
    err = vkCreateImageView(m_device->device(), &ivci, nullptr, &view);
    ASSERT_VK_SUCCESS(err);

    VkFramebufferCreateInfo fci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, rp, 1, &view, 32, 32, 1 };
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    VkRenderPassBeginInfo rpbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, rp, fb,{ { 0, 0 },{ 32, 32 } }, 0, nullptr };
    BeginCommandBuffer();
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    VkImageMemoryBarrier imb = {};
    imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imb.pNext = nullptr;
    imb.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imb.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    imb.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imb.srcQueueFamilyIndex = 0;
    imb.dstQueueFamilyIndex = 0;
    imb.image = image.handle();
    imb.subresourceRange.aspectMask = 0x6;
    imb.subresourceRange.baseMipLevel = 0;
    imb.subresourceRange.levelCount = 0x1;
    imb.subresourceRange.baseArrayLayer = 0;
    imb.subresourceRange.layerCount = 0x1;

    vkCmdPipelineBarrier(m_commandBuffer->handle(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1,
        &imb);

    vkCmdEndRenderPass(m_commandBuffer->handle());
    EndCommandBuffer();
    QueueCommandBuffer(false);
    m_errorMonitor->VerifyNotFound();

    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
    vkDestroyImageView(m_device->device(), view, nullptr);
}

TEST_F(VkPositiveLayerTest, RenderPassTransitionsAttachmentUnused) {
    TEST_DESCRIPTION("Ensure that layout transitions work correctly without "
        "errors, when an attachment reference is "
        "VK_ATTACHMENT_UNUSED");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    // A renderpass with no attachments
    VkAttachmentReference att_ref = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &att_ref, nullptr, nullptr, 0, nullptr };

    VkRenderPassCreateInfo rpci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 0, nullptr, 1, &subpass, 0, nullptr };

    VkRenderPass rp;
    VkResult err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    // A compatible framebuffer.
    VkFramebufferCreateInfo fci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, rp, 0, nullptr, 32, 32, 1 };
    VkFramebuffer fb;
    err = vkCreateFramebuffer(m_device->device(), &fci, nullptr, &fb);
    ASSERT_VK_SUCCESS(err);

    // Record a command buffer which just begins and ends the renderpass. The
    // bug manifests in BeginRenderPass.
    VkRenderPassBeginInfo rpbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, rp, fb,{ { 0, 0 },{ 32, 32 } }, 0, nullptr };
    BeginCommandBuffer();
    vkCmdBeginRenderPass(m_commandBuffer->handle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(m_commandBuffer->handle());
    m_errorMonitor->VerifyNotFound();
    EndCommandBuffer();

    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
}

// This is a positive test. No errors are expected.
TEST_F(VkPositiveLayerTest, StencilLoadOp) {
    TEST_DESCRIPTION("Create a stencil-only attachment with a LOAD_OP set to "
        "CLEAR. stencil[Load|Store]Op used to be ignored.");
    VkResult result = VK_SUCCESS;
    VkImageFormatProperties formatProps;
    vkGetPhysicalDeviceImageFormatProperties(gpu(), VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0,
        &formatProps);
    if (formatProps.maxExtent.width < 100 || formatProps.maxExtent.height < 100) {
        return;
    }

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkFormat depth_stencil_fmt = VK_FORMAT_D24_UNORM_S8_UINT;
    m_depthStencil->Init(m_device, 100, 100, depth_stencil_fmt,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    VkAttachmentDescription att = {};
    VkAttachmentReference ref = {};
    att.format = depth_stencil_fmt;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkClearValue clear;
    clear.depthStencil.depth = 1.0;
    clear.depthStencil.stencil = 0;
    ref.attachment = 0;
    ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = 0;
    subpass.pColorAttachments = NULL;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = &ref;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkRenderPass rp;
    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = 1;
    rp_info.pAttachments = &att;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    result = vkCreateRenderPass(device(), &rp_info, NULL, &rp);
    ASSERT_VK_SUCCESS(result);

    VkImageView *depthView = m_depthStencil->BindInfo();
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = rp;
    fb_info.attachmentCount = 1;
    fb_info.pAttachments = depthView;
    fb_info.width = 100;
    fb_info.height = 100;
    fb_info.layers = 1;
    VkFramebuffer fb;
    result = vkCreateFramebuffer(device(), &fb_info, NULL, &fb);
    ASSERT_VK_SUCCESS(result);

    VkRenderPassBeginInfo rpbinfo = {};
    rpbinfo.clearValueCount = 1;
    rpbinfo.pClearValues = &clear;
    rpbinfo.pNext = NULL;
    rpbinfo.renderPass = rp;
    rpbinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbinfo.renderArea.extent.width = 100;
    rpbinfo.renderArea.extent.height = 100;
    rpbinfo.renderArea.offset.x = 0;
    rpbinfo.renderArea.offset.y = 0;
    rpbinfo.framebuffer = fb;

    VkFence fence = {};
    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.pNext = nullptr;
    fence_ci.flags = 0;
    result = vkCreateFence(m_device->device(), &fence_ci, nullptr, &fence);
    ASSERT_VK_SUCCESS(result);

    m_commandBuffer->BeginCommandBuffer();
    m_commandBuffer->BeginRenderPass(rpbinfo);
    m_commandBuffer->EndRenderPass();
    m_commandBuffer->EndCommandBuffer();
    m_commandBuffer->QueueCommandBuffer(fence);

    VkImageObj destImage(m_device);
    destImage.init(100, 100, depth_stencil_fmt, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_TILING_OPTIMAL, 0);
    VkImageMemoryBarrier barrier = {};
    VkImageSubresourceRange range;
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = m_depthStencil->handle();
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    barrier.subresourceRange = range;
    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    VkCommandBufferObj cmdbuf(m_device, m_commandPool);
    cmdbuf.BeginCommandBuffer();
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &barrier);
    barrier.srcAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = destImage.handle();
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &barrier);
    VkImageCopy cregion;
    cregion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    cregion.srcSubresource.mipLevel = 0;
    cregion.srcSubresource.baseArrayLayer = 0;
    cregion.srcSubresource.layerCount = 1;
    cregion.srcOffset.x = 0;
    cregion.srcOffset.y = 0;
    cregion.srcOffset.z = 0;
    cregion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    cregion.dstSubresource.mipLevel = 0;
    cregion.dstSubresource.baseArrayLayer = 0;
    cregion.dstSubresource.layerCount = 1;
    cregion.dstOffset.x = 0;
    cregion.dstOffset.y = 0;
    cregion.dstOffset.z = 0;
    cregion.extent.width = 100;
    cregion.extent.height = 100;
    cregion.extent.depth = 1;
    cmdbuf.CopyImage(m_depthStencil->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destImage.handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cregion);
    cmdbuf.EndCommandBuffer();

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmdbuf.handle();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    m_errorMonitor->ExpectSuccess();
    vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    m_errorMonitor->VerifyNotFound();

    vkQueueWaitIdle(m_device->m_queue);
    vkDestroyFence(m_device->device(), fence, nullptr);
    vkDestroyRenderPass(m_device->device(), rp, nullptr);
    vkDestroyFramebuffer(m_device->device(), fb, nullptr);
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, WaitEventThenSet) {
    TEST_DESCRIPTION("Wait on a event then set it after the wait has been submitted.");

    m_errorMonitor->ExpectSuccess();
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkEvent event;
    VkEventCreateInfo event_create_info{};
    event_create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    vkCreateEvent(m_device->device(), &event_create_info, nullptr, &event);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, &command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 0, &queue);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        vkCmdWaitEvents(command_buffer, 1, &event, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0,
            nullptr, 0, nullptr);
        vkCmdResetEvent(command_buffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        vkEndCommandBuffer(command_buffer);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    { vkSetEvent(m_device->device(), event); }

    vkQueueWaitIdle(queue);

    vkDestroyEvent(m_device->device(), event, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 1, &command_buffer);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}
// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, QueryAndCopySecondaryCommandBuffers) {
    TEST_DESCRIPTION("Issue a query on a secondary command buffery and copy it on a primary.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    if ((m_device->queue_props.empty()) || (m_device->queue_props[0].queueCount < 2))
        return;

    m_errorMonitor->ExpectSuccess();

    VkQueryPool query_pool;
    VkQueryPoolCreateInfo query_pool_create_info{};
    query_pool_create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_create_info.queryCount = 1;
    vkCreateQueryPool(m_device->device(), &query_pool_create_info, nullptr, &query_pool);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, &command_buffer);

    VkCommandBuffer secondary_command_buffer;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, &secondary_command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 1, &queue);

    uint32_t qfi = 0;
    VkBufferCreateInfo buff_create_info = {};
    buff_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_create_info.size = 1024;
    buff_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buff_create_info.queueFamilyIndexCount = 1;
    buff_create_info.pQueueFamilyIndices = &qfi;

    VkResult err;
    VkBuffer buffer;
    err = vkCreateBuffer(m_device->device(), &buff_create_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 1024;
    mem_alloc.memoryTypeIndex = 0;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device->device(), buffer, &memReqs);
    bool pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &mem_alloc, 0);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }

    VkDeviceMemory mem;
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);

    VkCommandBufferInheritanceInfo hinfo = {};
    hinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    hinfo.renderPass = VK_NULL_HANDLE;
    hinfo.subpass = 0;
    hinfo.framebuffer = VK_NULL_HANDLE;
    hinfo.occlusionQueryEnable = VK_FALSE;
    hinfo.queryFlags = 0;
    hinfo.pipelineStatistics = 0;

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pInheritanceInfo = &hinfo;
        vkBeginCommandBuffer(secondary_command_buffer, &begin_info);

        vkCmdResetQueryPool(secondary_command_buffer, query_pool, 0, 1);
        vkCmdWriteTimestamp(secondary_command_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, query_pool, 0);

        vkEndCommandBuffer(secondary_command_buffer);

        begin_info.pInheritanceInfo = nullptr;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        vkCmdExecuteCommands(command_buffer, 1, &secondary_command_buffer);
        vkCmdCopyQueryPoolResults(command_buffer, query_pool, 0, 1, buffer, 0, 0, 0);

        vkEndCommandBuffer(command_buffer);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }

    vkQueueWaitIdle(queue);

    vkDestroyQueryPool(m_device->device(), query_pool, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 1, &command_buffer);
    vkFreeCommandBuffers(m_device->device(), command_pool, 1, &secondary_command_buffer);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkFreeMemory(m_device->device(), mem, NULL);

    m_errorMonitor->VerifyNotFound();
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, QueryAndCopyMultipleCommandBuffers) {
    TEST_DESCRIPTION("Issue a query and copy from it on a second command buffer.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    if ((m_device->queue_props.empty()) || (m_device->queue_props[0].queueCount < 2))
        return;

    m_errorMonitor->ExpectSuccess();

    VkQueryPool query_pool;
    VkQueryPoolCreateInfo query_pool_create_info{};
    query_pool_create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_create_info.queryCount = 1;
    vkCreateQueryPool(m_device->device(), &query_pool_create_info, nullptr, &query_pool);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 1, &queue);

    uint32_t qfi = 0;
    VkBufferCreateInfo buff_create_info = {};
    buff_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_create_info.size = 1024;
    buff_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buff_create_info.queueFamilyIndexCount = 1;
    buff_create_info.pQueueFamilyIndices = &qfi;

    VkResult err;
    VkBuffer buffer;
    err = vkCreateBuffer(m_device->device(), &buff_create_info, NULL, &buffer);
    ASSERT_VK_SUCCESS(err);
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 1024;
    mem_alloc.memoryTypeIndex = 0;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device->device(), buffer, &memReqs);
    bool pass = m_device->phy().set_memory_type(memReqs.memoryTypeBits, &mem_alloc, 0);
    if (!pass) {
        vkDestroyBuffer(m_device->device(), buffer, NULL);
        return;
    }

    VkDeviceMemory mem;
    err = vkAllocateMemory(m_device->device(), &mem_alloc, NULL, &mem);
    ASSERT_VK_SUCCESS(err);
    err = vkBindBufferMemory(m_device->device(), buffer, mem, 0);
    ASSERT_VK_SUCCESS(err);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdResetQueryPool(command_buffer[0], query_pool, 0, 1);
        vkCmdWriteTimestamp(command_buffer[0], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, query_pool, 0);

        vkEndCommandBuffer(command_buffer[0]);

        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        vkCmdCopyQueryPoolResults(command_buffer[1], query_pool, 0, 1, buffer, 0, 0, 0);

        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 2;
        submit_info.pCommandBuffers = command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }

    vkQueueWaitIdle(queue);

    vkDestroyQueryPool(m_device->device(), query_pool, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, command_buffer);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);
    vkDestroyBuffer(m_device->device(), buffer, NULL);
    vkFreeMemory(m_device->device(), mem, NULL);

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, ResetEventThenSet) {
    TEST_DESCRIPTION("Reset an event then set it after the reset has been submitted.");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkEvent event;
    VkEventCreateInfo event_create_info{};
    event_create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    vkCreateEvent(m_device->device(), &event_create_info, nullptr, &event);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, &command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 0, &queue);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        vkCmdResetEvent(command_buffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        vkCmdWaitEvents(command_buffer, 1, &event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
            nullptr, 0, nullptr, 0, nullptr);
        vkEndCommandBuffer(command_buffer);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        m_errorMonitor->SetDesiredFailureMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, "that is already in use by a "
            "command buffer.");
        vkSetEvent(m_device->device(), event);
        m_errorMonitor->VerifyFound();
    }

    vkQueueWaitIdle(queue);

    vkDestroyEvent(m_device->device(), event, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 1, &command_buffer);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoFencesThreeFrames) {
    TEST_DESCRIPTION("Two command buffers with two separate fences are each "
        "run through a Submit & WaitForFences cycle 3 times. This "
        "previously revealed a bug so running this positive test "
        "to prevent a regression.");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 0, &queue);

    static const uint32_t NUM_OBJECTS = 2;
    static const uint32_t NUM_FRAMES = 3;
    VkCommandBuffer cmd_buffers[NUM_OBJECTS] = {};
    VkFence fences[NUM_OBJECTS] = {};

    VkCommandPool cmd_pool;
    VkCommandPoolCreateInfo cmd_pool_ci = {};
    cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_ci.queueFamilyIndex = m_device->graphics_queue_node_index_;
    cmd_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkResult err = vkCreateCommandPool(m_device->device(), &cmd_pool_ci, nullptr, &cmd_pool);
    ASSERT_VK_SUCCESS(err);

    VkCommandBufferAllocateInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_info.commandPool = cmd_pool;
    cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_info.commandBufferCount = 1;

    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.pNext = nullptr;
    fence_ci.flags = 0;

    for (uint32_t i = 0; i < NUM_OBJECTS; ++i) {
        err = vkAllocateCommandBuffers(m_device->device(), &cmd_buf_info, &cmd_buffers[i]);
        ASSERT_VK_SUCCESS(err);
        err = vkCreateFence(m_device->device(), &fence_ci, nullptr, &fences[i]);
        ASSERT_VK_SUCCESS(err);
    }

    for (uint32_t frame = 0; frame < NUM_FRAMES; ++frame) {
        for (uint32_t obj = 0; obj < NUM_OBJECTS; ++obj) {
            // Create empty cmd buffer
            VkCommandBufferBeginInfo cmdBufBeginDesc = {};
            cmdBufBeginDesc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            err = vkBeginCommandBuffer(cmd_buffers[obj], &cmdBufBeginDesc);
            ASSERT_VK_SUCCESS(err);
            err = vkEndCommandBuffer(cmd_buffers[obj]);
            ASSERT_VK_SUCCESS(err);

            VkSubmitInfo submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd_buffers[obj];
            // Submit cmd buffer and wait for fence
            err = vkQueueSubmit(queue, 1, &submit_info, fences[obj]);
            ASSERT_VK_SUCCESS(err);
            err = vkWaitForFences(m_device->device(), 1, &fences[obj], VK_TRUE, UINT64_MAX);
            ASSERT_VK_SUCCESS(err);
            err = vkResetFences(m_device->device(), 1, &fences[obj]);
            ASSERT_VK_SUCCESS(err);
        }
    }
    m_errorMonitor->VerifyNotFound();
    vkDestroyCommandPool(m_device->device(), cmd_pool, NULL);
    for (uint32_t i = 0; i < NUM_OBJECTS; ++i) {
        vkDestroyFence(m_device->device(), fences[i], nullptr);
    }
}
// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoQueueSubmitsSeparateQueuesWithSemaphoreAndOneFenceQWI) {

    TEST_DESCRIPTION("Two command buffers, each in a separate QueueSubmit call "
        "submitted on separate queues followed by a QueueWaitIdle.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    if ((m_device->queue_props.empty()) || (m_device->queue_props[0].queueCount < 2))
        return;

    m_errorMonitor->ExpectSuccess();

    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 1, &queue);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[0];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphore;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[1];
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphore;
        submit_info.pWaitDstStageMask = flags;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    }

    vkQueueWaitIdle(m_device->m_queue);

    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoQueueSubmitsSeparateQueuesWithSemaphoreAndOneFenceQWIFence) {

    TEST_DESCRIPTION("Two command buffers, each in a separate QueueSubmit call "
        "submitted on separate queues, the second having a fence"
        "followed by a QueueWaitIdle.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    if ((m_device->queue_props.empty()) || (m_device->queue_props[0].queueCount < 2))
        return;

    m_errorMonitor->ExpectSuccess();

    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence);

    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 1, &queue);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[0];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphore;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[1];
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphore;
        submit_info.pWaitDstStageMask = flags;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, fence);
    }

    vkQueueWaitIdle(m_device->m_queue);

    vkDestroyFence(m_device->device(), fence, nullptr);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoQueueSubmitsSeparateQueuesWithSemaphoreAndOneFenceTwoWFF) {

    TEST_DESCRIPTION("Two command buffers, each in a separate QueueSubmit call "
        "submitted on separate queues, the second having a fence"
        "followed by two consecutive WaitForFences calls on the same fence.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    if ((m_device->queue_props.empty()) || (m_device->queue_props[0].queueCount < 2))
        return;

    m_errorMonitor->ExpectSuccess();

    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence);

    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 1, &queue);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[0];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphore;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[1];
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphore;
        submit_info.pWaitDstStageMask = flags;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, fence);
    }

    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(m_device->device(), fence, nullptr);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, TwoQueuesEnsureCorrectRetirementWithWorkStolen) {

    ASSERT_NO_FATAL_FAILURE(InitState());
    if ((m_device->queue_props.empty()) || (m_device->queue_props[0].queueCount < 2)) {
        printf("Test requires two queues, skipping\n");
        return;
    }

    VkResult err;

    m_errorMonitor->ExpectSuccess();

    VkQueue q0 = m_device->m_queue;
    VkQueue q1 = nullptr;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 1, &q1);
    ASSERT_NE(q1, nullptr);

    // An (empty) command buffer. We must have work in the first submission --
    // the layer treats unfenced work differently from fenced work.
    VkCommandPoolCreateInfo cpci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, 0, 0 };
    VkCommandPool pool;
    err = vkCreateCommandPool(m_device->device(), &cpci, nullptr, &pool);
    ASSERT_VK_SUCCESS(err);
    VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    VkCommandBuffer cb;
    err = vkAllocateCommandBuffers(m_device->device(), &cbai, &cb);
    ASSERT_VK_SUCCESS(err);
    VkCommandBufferBeginInfo cbbi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
    err = vkBeginCommandBuffer(cb, &cbbi);
    ASSERT_VK_SUCCESS(err);
    err = vkEndCommandBuffer(cb);
    ASSERT_VK_SUCCESS(err);

    // A semaphore
    VkSemaphoreCreateInfo sci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    VkSemaphore s;
    err = vkCreateSemaphore(m_device->device(), &sci, nullptr, &s);
    ASSERT_VK_SUCCESS(err);

    // First submission, to q0
    VkSubmitInfo s0 = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &cb, 1, &s };

    err = vkQueueSubmit(q0, 1, &s0, VK_NULL_HANDLE);
    ASSERT_VK_SUCCESS(err);

    // Second submission, to q1, waiting on s
    VkFlags waitmask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // doesn't really matter what this value is.
    VkSubmitInfo s1 = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 1, &s, &waitmask, 0, nullptr, 0, nullptr };

    err = vkQueueSubmit(q1, 1, &s1, VK_NULL_HANDLE);
    ASSERT_VK_SUCCESS(err);

    // Wait for q0 idle
    err = vkQueueWaitIdle(q0);
    ASSERT_VK_SUCCESS(err);

    // Command buffer should have been completed (it was on q0); reset the pool.
    vkFreeCommandBuffers(m_device->device(), pool, 1, &cb);

    m_errorMonitor->VerifyNotFound();

    // Force device completely idle and clean up resources
    vkDeviceWaitIdle(m_device->device());
    vkDestroyCommandPool(m_device->device(), pool, nullptr);
    vkDestroySemaphore(m_device->device(), s, nullptr);
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoQueueSubmitsSeparateQueuesWithSemaphoreAndOneFence) {

    TEST_DESCRIPTION("Two command buffers, each in a separate QueueSubmit call "
        "submitted on separate queues, the second having a fence, "
        "followed by a WaitForFences call.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    if ((m_device->queue_props.empty()) || (m_device->queue_props[0].queueCount < 2))
        return;

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence);

    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device->device(), m_device->graphics_queue_node_index_, 1, &queue);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[0];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphore;
        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[1];
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphore;
        submit_info.pWaitDstStageMask = flags;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, fence);
    }

    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(m_device->device(), fence, nullptr);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoQueueSubmitsOneQueueWithSemaphoreAndOneFence) {

    TEST_DESCRIPTION("Two command buffers, each in a separate QueueSubmit call "
        "on the same queue, sharing a signal/wait semaphore, the "
        "second having a fence, "
        "followed by a WaitForFences call.");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence);

    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[0];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphore;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[1];
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphore;
        submit_info.pWaitDstStageMask = flags;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, fence);
    }

    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(m_device->device(), fence, nullptr);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoQueueSubmitsOneQueueNullQueueSubmitWithFence) {

    TEST_DESCRIPTION("Two command buffers, each in a separate QueueSubmit call "
        "on the same queue, no fences, followed by a third QueueSubmit with NO "
        "SubmitInfos but with a fence, followed by a WaitForFences call.");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[0];
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = VK_NULL_HANDLE;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[1];
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = VK_NULL_HANDLE;
        submit_info.pWaitDstStageMask = flags;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    }

    vkQueueSubmit(m_device->m_queue, 0, NULL, fence);

    VkResult err = vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);
    ASSERT_VK_SUCCESS(err);

    vkDestroyFence(m_device->device(), fence, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoQueueSubmitsOneQueueOneFence) {

    TEST_DESCRIPTION("Two command buffers, each in a separate QueueSubmit call "
        "on the same queue, the second having a fence, followed "
        "by a WaitForFences call.");

    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[0];
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = VK_NULL_HANDLE;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    {
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer[1];
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = VK_NULL_HANDLE;
        submit_info.pWaitDstStageMask = flags;
        vkQueueSubmit(m_device->m_queue, 1, &submit_info, fence);
    }

    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(m_device->device(), fence, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);

    m_errorMonitor->VerifyNotFound();
}

// This is a positive test.  No errors should be generated.
TEST_F(VkPositiveLayerTest, TwoSubmitInfosWithSemaphoreOneQueueSubmitsOneFence) {

    TEST_DESCRIPTION("Two command buffers each in a separate SubmitInfo sent in a single "
        "QueueSubmit call followed by a WaitForFences call.");
    ASSERT_NO_FATAL_FAILURE(InitState());

    m_errorMonitor->ExpectSuccess();

    VkFence fence;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device->device(), &fence_create_info, nullptr, &fence);

    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(m_device->device(), &semaphore_create_info, nullptr, &semaphore);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_device->graphics_queue_node_index_;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_device->device(), &pool_create_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer[2];
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 2;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_device->device(), &command_buffer_allocate_info, command_buffer);

    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[0], &begin_info);

        vkCmdPipelineBarrier(command_buffer[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            nullptr, 0, nullptr, 0, nullptr);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[0]);
    }
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer[1], &begin_info);

        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer[1], 0, 1, &viewport);
        vkEndCommandBuffer(command_buffer[1]);
    }
    {
        VkSubmitInfo submit_info[2];
        VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };

        submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info[0].pNext = NULL;
        submit_info[0].commandBufferCount = 1;
        submit_info[0].pCommandBuffers = &command_buffer[0];
        submit_info[0].signalSemaphoreCount = 1;
        submit_info[0].pSignalSemaphores = &semaphore;
        submit_info[0].waitSemaphoreCount = 0;
        submit_info[0].pWaitSemaphores = NULL;
        submit_info[0].pWaitDstStageMask = 0;

        submit_info[1].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info[1].pNext = NULL;
        submit_info[1].commandBufferCount = 1;
        submit_info[1].pCommandBuffers = &command_buffer[1];
        submit_info[1].waitSemaphoreCount = 1;
        submit_info[1].pWaitSemaphores = &semaphore;
        submit_info[1].pWaitDstStageMask = flags;
        submit_info[1].signalSemaphoreCount = 0;
        submit_info[1].pSignalSemaphores = NULL;
        vkQueueSubmit(m_device->m_queue, 2, &submit_info[0], fence);
    }

    vkWaitForFences(m_device->device(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(m_device->device(), fence, nullptr);
    vkFreeCommandBuffers(m_device->device(), command_pool, 2, &command_buffer[0]);
    vkDestroyCommandPool(m_device->device(), command_pool, NULL);
    vkDestroySemaphore(m_device->device(), semaphore, nullptr);

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, RenderPassSecondaryCommandBuffersMultipleTimes) {
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();                                   // Framework implicitly begins the renderpass.
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle()); // End implicit.

    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), &m_renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle());
    m_errorMonitor->VerifyNotFound();
    vkCmdBeginRenderPass(m_commandBuffer->GetBufferHandle(), &m_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    m_errorMonitor->VerifyNotFound();
    vkCmdEndRenderPass(m_commandBuffer->GetBufferHandle());
    m_errorMonitor->VerifyNotFound();

    m_commandBuffer->EndCommandBuffer();
    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, ValidRenderPassAttachmentLayoutWithLoadOp) {
    TEST_DESCRIPTION("Positive test where we create a renderpass with an "
                     "attachment that uses LOAD_OP_CLEAR, the first subpass "
                     "has a valid layout, and a second subpass then uses a "
                     "valid *READ_ONLY* layout.");
    m_errorMonitor->ExpectSuccess();
    ASSERT_NO_FATAL_FAILURE(InitState());

    VkAttachmentReference attach[2] = {};
    attach[0].attachment = 0;
    attach[0].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attach[1].attachment = 0;
    attach[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkSubpassDescription subpasses[2] = {};
    // First subpass clears DS attach on load
    subpasses[0].pDepthStencilAttachment = &attach[0];
    // 2nd subpass reads in DS as input attachment
    subpasses[1].inputAttachmentCount = 1;
    subpasses[1].pInputAttachments = &attach[1];
    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attach_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attach_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attach_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attach_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkRenderPassCreateInfo rpci = {};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &attach_desc;
    rpci.subpassCount = 2;
    rpci.pSubpasses = subpasses;

    // Now create RenderPass and verify no errors
    VkRenderPass rp;
    vkCreateRenderPass(m_device->device(), &rpci, NULL, &rp);
    m_errorMonitor->VerifyNotFound();

    vkDestroyRenderPass(m_device->device(), rp, NULL);
}

TEST_F(VkPositiveLayerTest, CreatePipelineAttribMatrixType) {
    TEST_DESCRIPTION("Test that pipeline validation accepts matrices passed "
        "as vertex attributes");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attribs[2];
    memset(input_attribs, 0, sizeof(input_attribs));

    for (int i = 0; i < 2; i++) {
        input_attribs[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        input_attribs[i].location = i;
    }

    char const *vsSource = "#version 450\n"
        "\n"
        "layout(location=0) in mat2x4 x;\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main(){\n"
        "   gl_Position = x[0] + x[1];\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(input_attribs, 2);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    /* expect success */
    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipelineAttribArrayType) {
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attribs[2];
    memset(input_attribs, 0, sizeof(input_attribs));

    for (int i = 0; i < 2; i++) {
        input_attribs[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        input_attribs[i].location = i;
    }

    char const *vsSource = "#version 450\n"
        "\n"
        "layout(location=0) in vec4 x[2];\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main(){\n"
        "   gl_Position = x[0] + x[1];\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(input_attribs, 2);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipelineAttribComponents) {
    TEST_DESCRIPTION("Test that pipeline validation accepts consuming a vertex attribute "
        "through multiple vertex shader inputs, each consuming a different "
        "subset of the components.");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attribs[3];
    memset(input_attribs, 0, sizeof(input_attribs));

    for (int i = 0; i < 3; i++) {
        input_attribs[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        input_attribs[i].location = i;
    }

    char const *vsSource = "#version 450\n"
        "\n"
        "layout(location=0) in vec4 x;\n"
        "layout(location=1) in vec3 y1;\n"
        "layout(location=1, component=3) in float y2;\n"
        "layout(location=2) in vec4 z;\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main(){\n"
        "   gl_Position = x + vec4(y1, y2) + z;\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(input_attribs, 3);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipelineSimplePositive) {
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main(){\n"
        "   gl_Position = vec4(0);\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipelineRelaxedTypeMatch) {
    TEST_DESCRIPTION("Test that pipeline validation accepts the relaxed type matching rules "
        "set out in 14.1.3: fundamental type must match, and producer side must "
        "have at least as many components");
    m_errorMonitor->ExpectSuccess();

    // VK 1.0.8 Specification, 14.1.3 "Additionally,..." block

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    char const *vsSource = "#version 450\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "layout(location=0) out vec3 x;\n"
        "layout(location=1) out ivec3 y;\n"
        "layout(location=2) out vec3 z;\n"
        "void main(){\n"
        "   gl_Position = vec4(0);\n"
        "   x = vec3(0); y = ivec3(0); z = vec3(0);\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "layout(location=0) in float x;\n"
        "layout(location=1) flat in int y;\n"
        "layout(location=2) in vec2 z;\n"
        "void main(){\n"
        "   color = vec4(1 + x + y + z.x);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    VkResult err = VK_SUCCESS;
    err = pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());
    ASSERT_VK_SUCCESS(err);

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipelineTessPerVertex) {
    TEST_DESCRIPTION("Test that pipeline validation accepts per-vertex variables "
        "passed between the TCS and TES stages");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    if (!m_device->phy().features().tessellationShader) {
        printf("Device does not support tessellation shaders; skipped.\n");
        return;
    }

    char const *vsSource = "#version 450\n"
        "void main(){}\n";
    char const *tcsSource = "#version 450\n"
        "layout(location=0) out int x[];\n"
        "layout(vertices=3) out;\n"
        "void main(){\n"
        "   gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = 1;\n"
        "   gl_TessLevelInner[0] = 1;\n"
        "   x[gl_InvocationID] = gl_InvocationID;\n"
        "}\n";
    char const *tesSource = "#version 450\n"
        "layout(triangles, equal_spacing, cw) in;\n"
        "layout(location=0) in int x[];\n"
        "out gl_PerVertex { vec4 gl_Position; };\n"
        "void main(){\n"
        "   gl_Position.xyz = gl_TessCoord;\n"
        "   gl_Position.w = x[0] + x[1] + x[2];\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj tcs(m_device, tcsSource, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, this);
    VkShaderObj tes(m_device, tesSource, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineInputAssemblyStateCreateInfo iasci{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, VK_FALSE };

    VkPipelineTessellationStateCreateInfo tsci{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, nullptr, 0, 3 };

    VkPipelineObj pipe(m_device);
    pipe.SetInputAssembly(&iasci);
    pipe.SetTessellation(&tsci);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&tcs);
    pipe.AddShader(&tes);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipelineGeometryInputBlockPositive) {
    TEST_DESCRIPTION("Test that pipeline validation accepts a user-defined "
        "interface block passed into the geometry shader. This "
        "is interesting because the 'extra' array level is not "
        "present on the member type, but on the block instance.");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    if (!m_device->phy().features().geometryShader) {
        printf("Device does not support geometry shaders; skipped.\n");
        return;
    }

    char const *vsSource = "#version 450\n"
        "layout(location=0) out VertexData { vec4 x; } vs_out;\n"
        "void main(){\n"
        "   vs_out.x = vec4(1);\n"
        "}\n";
    char const *gsSource = "#version 450\n"
        "layout(triangles) in;\n"
        "layout(triangle_strip, max_vertices=3) out;\n"
        "layout(location=0) in VertexData { vec4 x; } gs_in[];\n"
        "out gl_PerVertex { vec4 gl_Position; };\n"
        "void main() {\n"
        "   gl_Position = gs_in[0].x;\n"
        "   EmitVertex();\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj gs(m_device, gsSource, VK_SHADER_STAGE_GEOMETRY_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&gs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipeline64BitAttributesPositive) {
    TEST_DESCRIPTION("Test that pipeline validation accepts basic use of 64bit vertex "
        "attributes. This is interesting because they consume multiple "
        "locations.");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    if (!m_device->phy().features().shaderFloat64) {
        printf("Device does not support 64bit vertex attributes; skipped.\n");
        return;
    }

    VkVertexInputBindingDescription input_bindings[1];
    memset(input_bindings, 0, sizeof(input_bindings));

    VkVertexInputAttributeDescription input_attribs[4];
    memset(input_attribs, 0, sizeof(input_attribs));
    input_attribs[0].location = 0;
    input_attribs[0].offset = 0;
    input_attribs[0].format = VK_FORMAT_R64G64B64A64_SFLOAT;
    input_attribs[1].location = 2;
    input_attribs[1].offset = 32;
    input_attribs[1].format = VK_FORMAT_R64G64B64A64_SFLOAT;
    input_attribs[2].location = 4;
    input_attribs[2].offset = 64;
    input_attribs[2].format = VK_FORMAT_R64G64B64A64_SFLOAT;
    input_attribs[3].location = 6;
    input_attribs[3].offset = 96;
    input_attribs[3].format = VK_FORMAT_R64G64B64A64_SFLOAT;

    char const *vsSource = "#version 450\n"
        "\n"
        "layout(location=0) in dmat4 x;\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main(){\n"
        "   gl_Position = vec4(x[0][0]);\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(input_bindings, 1);
    pipe.AddVertexInputAttribs(input_attribs, 4);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    m_errorMonitor->VerifyNotFound();
}

TEST_F(VkPositiveLayerTest, CreatePipelineInputAttachmentPositive) {
    TEST_DESCRIPTION("Positive test for a correctly matched input attachment");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *vsSource = "#version 450\n"
        "\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main(){\n"
        "    gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource = "#version 450\n"
        "\n"
        "layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput x;\n"
        "layout(location=0) out vec4 color;\n"
        "void main() {\n"
        "   color = subpassLoad(x);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetLayoutBinding dslb = { 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutCreateInfo dslci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dslb };
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr };
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    VkAttachmentDescription descs[2] = {
        { 0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { 0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL },
    };
    VkAttachmentReference color = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference input = {
        1, VK_IMAGE_LAYOUT_GENERAL,
    };

    VkSubpassDescription sd = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 1, &input, 1, &color, nullptr, nullptr, 0, nullptr };

    VkRenderPassCreateInfo rpci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 2, descs, 1, &sd, 0, nullptr };
    VkRenderPass rp;
    err = vkCreateRenderPass(m_device->device(), &rpci, nullptr, &rp);
    ASSERT_VK_SUCCESS(err);

    // should be OK. would go wrong here if it's going to...
    pipe.CreateVKPipeline(pl, rp);

    m_errorMonitor->VerifyNotFound();

    vkDestroyRenderPass(m_device->device(), rp, nullptr);
    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkPositiveLayerTest, CreateComputePipelineMissingDescriptorUnusedPositive) {
    TEST_DESCRIPTION("Test that pipeline validation accepts a compute pipeline which declares a "
        "descriptor-backed resource which is not provided, but the shader does not "
        "statically use it. This is interesting because it requires compute pipelines "
        "to have a proper descriptor use walk, which they didn't for some time.");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    char const *csSource = "#version 450\n"
        "\n"
        "layout(local_size_x=1) in;\n"
        "layout(set=0, binding=0) buffer block { vec4 x; };\n"
        "void main(){\n"
        "   // x is not used.\n"
        "}\n";

    VkShaderObj cs(m_device, csSource, VK_SHADER_STAGE_COMPUTE_BIT, this);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.CreateVKDescriptorSet(m_commandBuffer);

    VkComputePipelineCreateInfo cpci = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
        VK_SHADER_STAGE_COMPUTE_BIT, cs.handle(), "main", nullptr },
        descriptorSet.GetPipelineLayout(),
        VK_NULL_HANDLE,
        -1 };

    VkPipeline pipe;
    VkResult err = vkCreateComputePipelines(m_device->device(), VK_NULL_HANDLE, 1, &cpci, nullptr, &pipe);

    m_errorMonitor->VerifyNotFound();

    if (err == VK_SUCCESS) {
        vkDestroyPipeline(m_device->device(), pipe, nullptr);
    }
}

TEST_F(VkPositiveLayerTest, CreateComputePipelineCombinedImageSamplerConsumedAsSampler) {
    TEST_DESCRIPTION("Test that pipeline validation accepts a shader consuming only the "
        "sampler portion of a combined image + sampler");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDescriptorSetLayoutBinding bindings[] = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };
    VkDescriptorSetLayoutCreateInfo dslci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 3, bindings };
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr };
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    char const *csSource = "#version 450\n"
        "\n"
        "layout(local_size_x=1) in;\n"
        "layout(set=0, binding=0) uniform sampler s;\n"
        "layout(set=0, binding=1) uniform texture2D t;\n"
        "layout(set=0, binding=2) buffer block { vec4 x; };\n"
        "void main() {\n"
        "   x = texture(sampler2D(t, s), vec2(0));\n"
        "}\n";
    VkShaderObj cs(m_device, csSource, VK_SHADER_STAGE_COMPUTE_BIT, this);

    VkComputePipelineCreateInfo cpci = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
        VK_SHADER_STAGE_COMPUTE_BIT, cs.handle(), "main", nullptr },
        pl,
        VK_NULL_HANDLE,
        -1 };

    VkPipeline pipe;
    err = vkCreateComputePipelines(m_device->device(), VK_NULL_HANDLE, 1, &cpci, nullptr, &pipe);

    m_errorMonitor->VerifyNotFound();

    if (err == VK_SUCCESS) {
        vkDestroyPipeline(m_device->device(), pipe, nullptr);
    }

    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkPositiveLayerTest, CreateComputePipelineCombinedImageSamplerConsumedAsImage) {
    TEST_DESCRIPTION("Test that pipeline validation accepts a shader consuming only the "
        "image portion of a combined image + sampler");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDescriptorSetLayoutBinding bindings[] = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };
    VkDescriptorSetLayoutCreateInfo dslci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 3, bindings };
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr };
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    char const *csSource = "#version 450\n"
        "\n"
        "layout(local_size_x=1) in;\n"
        "layout(set=0, binding=0) uniform texture2D t;\n"
        "layout(set=0, binding=1) uniform sampler s;\n"
        "layout(set=0, binding=2) buffer block { vec4 x; };\n"
        "void main() {\n"
        "   x = texture(sampler2D(t, s), vec2(0));\n"
        "}\n";
    VkShaderObj cs(m_device, csSource, VK_SHADER_STAGE_COMPUTE_BIT, this);

    VkComputePipelineCreateInfo cpci = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
        VK_SHADER_STAGE_COMPUTE_BIT, cs.handle(), "main", nullptr },
        pl,
        VK_NULL_HANDLE,
        -1 };

    VkPipeline pipe;
    err = vkCreateComputePipelines(m_device->device(), VK_NULL_HANDLE, 1, &cpci, nullptr, &pipe);

    m_errorMonitor->VerifyNotFound();

    if (err == VK_SUCCESS) {
        vkDestroyPipeline(m_device->device(), pipe, nullptr);
    }

    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkPositiveLayerTest, CreateComputePipelineCombinedImageSamplerConsumedAsBoth) {
    TEST_DESCRIPTION("Test that pipeline validation accepts a shader consuming "
        "both the sampler and the image of a combined image+sampler "
        "but via separate variables");
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkDescriptorSetLayoutBinding bindings[] = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };
    VkDescriptorSetLayoutCreateInfo dslci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 2, bindings };
    VkDescriptorSetLayout dsl;
    VkResult err = vkCreateDescriptorSetLayout(m_device->device(), &dslci, nullptr, &dsl);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo plci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 1, &dsl, 0, nullptr };
    VkPipelineLayout pl;
    err = vkCreatePipelineLayout(m_device->device(), &plci, nullptr, &pl);
    ASSERT_VK_SUCCESS(err);

    char const *csSource = "#version 450\n"
        "\n"
        "layout(local_size_x=1) in;\n"
        "layout(set=0, binding=0) uniform texture2D t;\n"
        "layout(set=0, binding=0) uniform sampler s;  // both binding 0!\n"
        "layout(set=0, binding=1) buffer block { vec4 x; };\n"
        "void main() {\n"
        "   x = texture(sampler2D(t, s), vec2(0));\n"
        "}\n";
    VkShaderObj cs(m_device, csSource, VK_SHADER_STAGE_COMPUTE_BIT, this);

    VkComputePipelineCreateInfo cpci = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
        VK_SHADER_STAGE_COMPUTE_BIT, cs.handle(), "main", nullptr },
        pl,
        VK_NULL_HANDLE,
        -1 };

    VkPipeline pipe;
    err = vkCreateComputePipelines(m_device->device(), VK_NULL_HANDLE, 1, &cpci, nullptr, &pipe);

    m_errorMonitor->VerifyNotFound();

    if (err == VK_SUCCESS) {
        vkDestroyPipeline(m_device->device(), pipe, nullptr);
    }

    vkDestroyPipelineLayout(m_device->device(), pl, nullptr);
    vkDestroyDescriptorSetLayout(m_device->device(), dsl, nullptr);
}

TEST_F(VkPositiveLayerTest, ValidStructPNext) {
    TEST_DESCRIPTION("Verify that a valid pNext value is handled correctly");

    ASSERT_NO_FATAL_FAILURE(InitState());

    // Positive test to check parameter_validation and unique_objects support
    // for NV_dedicated_allocation
    uint32_t extension_count = 0;
    bool supports_nv_dedicated_allocation = false;
    VkResult err = vkEnumerateDeviceExtensionProperties(gpu(), nullptr, &extension_count, nullptr);
    ASSERT_VK_SUCCESS(err);

    if (extension_count > 0) {
        std::vector<VkExtensionProperties> available_extensions(extension_count);

        err = vkEnumerateDeviceExtensionProperties(gpu(), nullptr, &extension_count, &available_extensions[0]);
        ASSERT_VK_SUCCESS(err);

        for (const auto &extension_props : available_extensions) {
            if (strcmp(extension_props.extensionName, VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
                supports_nv_dedicated_allocation = true;
            }
        }
    }

    if (supports_nv_dedicated_allocation) {
        m_errorMonitor->ExpectSuccess();

        VkDedicatedAllocationBufferCreateInfoNV dedicated_buffer_create_info = {};
        dedicated_buffer_create_info.sType = VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV;
        dedicated_buffer_create_info.pNext = nullptr;
        dedicated_buffer_create_info.dedicatedAllocation = VK_TRUE;

        uint32_t queue_family_index = 0;
        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = &dedicated_buffer_create_info;
        buffer_create_info.size = 1024;
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.queueFamilyIndexCount = 1;
        buffer_create_info.pQueueFamilyIndices = &queue_family_index;

        VkBuffer buffer;
        VkResult err = vkCreateBuffer(m_device->device(), &buffer_create_info, NULL, &buffer);
        ASSERT_VK_SUCCESS(err);

        VkMemoryRequirements memory_reqs;
        vkGetBufferMemoryRequirements(m_device->device(), buffer, &memory_reqs);

        VkDedicatedAllocationMemoryAllocateInfoNV dedicated_memory_info = {};
        dedicated_memory_info.sType = VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV;
        dedicated_memory_info.pNext = nullptr;
        dedicated_memory_info.buffer = buffer;
        dedicated_memory_info.image = VK_NULL_HANDLE;

        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.pNext = &dedicated_memory_info;
        memory_info.allocationSize = memory_reqs.size;

        bool pass;
        pass = m_device->phy().set_memory_type(memory_reqs.memoryTypeBits, &memory_info, 0);
        ASSERT_TRUE(pass);

        VkDeviceMemory buffer_memory;
        err = vkAllocateMemory(m_device->device(), &memory_info, NULL, &buffer_memory);
        ASSERT_VK_SUCCESS(err);

        err = vkBindBufferMemory(m_device->device(), buffer, buffer_memory, 0);
        ASSERT_VK_SUCCESS(err);

        vkDestroyBuffer(m_device->device(), buffer, NULL);
        vkFreeMemory(m_device->device(), buffer_memory, NULL);

        m_errorMonitor->VerifyNotFound();
    }
}

TEST_F(VkPositiveLayerTest, PSOPolygonModeValid) {
    VkResult err;

    TEST_DESCRIPTION("Verify that using a solid polygon fill mode works correctly.");

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    std::vector<const char *> device_extension_names;
    auto features = m_device->phy().features();
    // Artificially disable support for non-solid fill modes
    features.fillModeNonSolid = false;
    // The sacrificial device object
    VkDeviceObj test_device(0, gpu(), device_extension_names, &features);

    VkRenderpassObj render_pass(&test_device);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 0;
    pipeline_layout_ci.pSetLayouts = NULL;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(test_device.device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.pNext = nullptr;
    rs_ci.lineWidth = 1.0f;
    rs_ci.rasterizerDiscardEnable = true;

    VkShaderObj vs(&test_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT, this);
    VkShaderObj fs(&test_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT, this);

    // Set polygonMode=FILL. No error is expected
    m_errorMonitor->ExpectSuccess();
    {
        VkPipelineObj pipe(&test_device);
        pipe.AddShader(&vs);
        pipe.AddShader(&fs);
        pipe.AddColorAttachment();
        // Set polygonMode to a good value
        rs_ci.polygonMode = VK_POLYGON_MODE_FILL;
        pipe.SetRasterization(&rs_ci);
        pipe.CreateVKPipeline(pipeline_layout, render_pass.handle());
    }
    m_errorMonitor->VerifyNotFound();

    vkDestroyPipelineLayout(test_device.device(), pipeline_layout, NULL);
}

TEST_F(VkPositiveLayerTest, ValidPushConstants) {
    VkResult err;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkPipelineLayout pipeline_layout;
    VkPushConstantRange pc_range = {};
    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pushConstantRangeCount = 1;
    pipeline_layout_ci.pPushConstantRanges = &pc_range;

    //
    // Check for invalid push constant ranges in pipeline layouts.
    //
    struct PipelineLayoutTestCase {
        VkPushConstantRange const range;
        char const *msg;
    };

    // Check for overlapping ranges
    const uint32_t ranges_per_test = 5;
    struct OverlappingRangeTestCase {
        VkPushConstantRange const ranges[ranges_per_test];
        char const *msg;
    };

    // Run some positive tests to make sure overlap checking in the layer is OK
    const std::array<OverlappingRangeTestCase, 2> overlapping_range_tests_pos = { { { { { VK_SHADER_STAGE_VERTEX_BIT, 0, 4 },
    { VK_SHADER_STAGE_VERTEX_BIT, 4, 4 },
    { VK_SHADER_STAGE_VERTEX_BIT, 8, 4 },
    { VK_SHADER_STAGE_VERTEX_BIT, 12, 4 },
    { VK_SHADER_STAGE_VERTEX_BIT, 16, 4 } },
        "" },
        { { { VK_SHADER_STAGE_VERTEX_BIT, 92, 24 },
        { VK_SHADER_STAGE_VERTEX_BIT, 80, 4 },
        { VK_SHADER_STAGE_VERTEX_BIT, 64, 8 },
        { VK_SHADER_STAGE_VERTEX_BIT, 4, 16 },
        { VK_SHADER_STAGE_VERTEX_BIT, 0, 4 } },
        "" } } };
    for (const auto &iter : overlapping_range_tests_pos) {
        pipeline_layout_ci.pPushConstantRanges = iter.ranges;
        m_errorMonitor->ExpectSuccess();
        err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
        m_errorMonitor->VerifyNotFound();
        if (VK_SUCCESS == err) {
            vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);
        }
    }

    //
    // CmdPushConstants tests
    //
    const uint8_t dummy_values[100] = {};

    BeginCommandBuffer();

    // positive overlapping range tests with cmd
    const std::array<PipelineLayoutTestCase, 4> cmd_overlap_tests_pos = { {
        { { VK_SHADER_STAGE_VERTEX_BIT, 0, 16 }, "" },
        { { VK_SHADER_STAGE_VERTEX_BIT, 0, 4 }, "" },
        { { VK_SHADER_STAGE_VERTEX_BIT, 20, 12 }, "" },
        { { VK_SHADER_STAGE_VERTEX_BIT, 56, 36 }, "" },
        } };

    // Setup ranges: [0,16) [20,36) [36,44) [44,52) [56,80) [80,92)
    const VkPushConstantRange pc_range4[] = {
        { VK_SHADER_STAGE_VERTEX_BIT, 20, 16 },{ VK_SHADER_STAGE_VERTEX_BIT, 0, 16 },{ VK_SHADER_STAGE_VERTEX_BIT, 44, 8 },
        { VK_SHADER_STAGE_VERTEX_BIT, 80, 12 },{ VK_SHADER_STAGE_VERTEX_BIT, 36, 8 },{ VK_SHADER_STAGE_VERTEX_BIT, 56, 24 },
    };

    pipeline_layout_ci.pushConstantRangeCount = sizeof(pc_range4) / sizeof(VkPushConstantRange);
    pipeline_layout_ci.pPushConstantRanges = pc_range4;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, NULL, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);
    for (const auto &iter : cmd_overlap_tests_pos) {
        m_errorMonitor->ExpectSuccess();
        vkCmdPushConstants(m_commandBuffer->GetBufferHandle(), pipeline_layout, iter.range.stageFlags, iter.range.offset,
            iter.range.size, dummy_values);
        m_errorMonitor->VerifyNotFound();
    }
    vkDestroyPipelineLayout(m_device->device(), pipeline_layout, NULL);

    EndCommandBuffer();
}







#if 0 // A few devices have issues with this test so disabling for now
TEST_F(VkPositiveLayerTest, LongFenceChain)
{
    m_errorMonitor->ExpectSuccess();

    ASSERT_NO_FATAL_FAILURE(InitState());
    VkResult err;

    std::vector<VkFence> fences;

    const int chainLength = 32768;

    for (int i = 0; i < chainLength; i++) {
        VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
        VkFence fence;
        err = vkCreateFence(m_device->device(), &fci, nullptr, &fence);
        ASSERT_VK_SUCCESS(err);

        fences.push_back(fence);

        VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr,
            0, nullptr, 0, nullptr };
        err = vkQueueSubmit(m_device->m_queue, 1, &si, fence);
        ASSERT_VK_SUCCESS(err);

    }

    // BOOM, stack overflow.
    vkWaitForFences(m_device->device(), 1, &fences.back(), VK_TRUE, UINT64_MAX);

    for (auto fence : fences)
        vkDestroyFence(m_device->device(), fence, nullptr);

    m_errorMonitor->VerifyNotFound();
}
#endif


#if defined(ANDROID) && defined(VALIDATION_APK)
static bool initialized = false;
static bool active = false;

// Convert Intents to argv
// Ported from Hologram sample, only difference is flexible key
std::vector<std::string> get_args(android_app &app, const char *intent_extra_data_key) {
    std::vector<std::string> args;
    JavaVM &vm = *app.activity->vm;
    JNIEnv *p_env;
    if (vm.AttachCurrentThread(&p_env, nullptr) != JNI_OK)
        return args;

    JNIEnv &env = *p_env;
    jobject activity = app.activity->clazz;
    jmethodID get_intent_method = env.GetMethodID(env.GetObjectClass(activity), "getIntent", "()Landroid/content/Intent;");
    jobject intent = env.CallObjectMethod(activity, get_intent_method);
    jmethodID get_string_extra_method =
        env.GetMethodID(env.GetObjectClass(intent), "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
    jvalue get_string_extra_args;
    get_string_extra_args.l = env.NewStringUTF(intent_extra_data_key);
    jstring extra_str = static_cast<jstring>(env.CallObjectMethodA(intent, get_string_extra_method, &get_string_extra_args));

    std::string args_str;
    if (extra_str) {
        const char *extra_utf = env.GetStringUTFChars(extra_str, nullptr);
        args_str = extra_utf;
        env.ReleaseStringUTFChars(extra_str, extra_utf);
        env.DeleteLocalRef(extra_str);
    }

    env.DeleteLocalRef(get_string_extra_args.l);
    env.DeleteLocalRef(intent);
    vm.DetachCurrentThread();

    // split args_str
    std::stringstream ss(args_str);
    std::string arg;
    while (std::getline(ss, arg, ' ')) {
        if (!arg.empty())
            args.push_back(arg);
    }

    return args;
}

static int32_t processInput(struct android_app *app, AInputEvent *event) { return 0; }

static void processCommand(struct android_app *app, int32_t cmd) {
    switch (cmd) {
    case APP_CMD_INIT_WINDOW: {
        if (app->window) {
            initialized = true;
        }
        break;
    }
    case APP_CMD_GAINED_FOCUS: {
        active = true;
        break;
    }
    case APP_CMD_LOST_FOCUS: {
        active = false;
        break;
    }
    }
}

void android_main(struct android_app *app) {
    app_dummy();

    const char *appTag = "VulkanLayerValidationTests";

    int vulkanSupport = InitVulkan();
    if (vulkanSupport == 0) {
        __android_log_print(ANDROID_LOG_INFO, appTag, "==== FAILED ==== No Vulkan support found");
        return;
    }

    app->onAppCmd = processCommand;
    app->onInputEvent = processInput;

    while (1) {
        int events;
        struct android_poll_source *source;
        while (ALooper_pollAll(active ? 0 : -1, NULL, &events, (void **)&source) >= 0) {
            if (source) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                VkTestFramework::Finish();
                return;
            }
        }

        if (initialized && active) {
            // Use the following key to send arguments to gtest, i.e.
            // --es args "--gtest_filter=-VkLayerTest.foo"
            const char key[] = "args";
            std::vector<std::string> args = get_args(*app, key);

            std::string filter = "";
            if (args.size() > 0) {
                __android_log_print(ANDROID_LOG_INFO, appTag, "Intent args = %s", args[0].c_str());
                filter += args[0];
            } else {
                __android_log_print(ANDROID_LOG_INFO, appTag, "No Intent args detected");
            }

            int argc = 2;
            char *argv[] = {(char *)"foo", (char *)filter.c_str()};
            __android_log_print(ANDROID_LOG_DEBUG, appTag, "filter = %s", argv[1]);

            // Route output to files until we can override the gtest output
            freopen("/sdcard/Android/data/com.example.VulkanLayerValidationTests/files/out.txt", "w", stdout);
            freopen("/sdcard/Android/data/com.example.VulkanLayerValidationTests/files/err.txt", "w", stderr);

            ::testing::InitGoogleTest(&argc, argv);
            VkTestFramework::InitArgs(&argc, argv);
            ::testing::AddGlobalTestEnvironment(new TestEnvironment);

            int result = RUN_ALL_TESTS();

            if (result != 0) {
                __android_log_print(ANDROID_LOG_INFO, appTag, "==== Tests FAILED ====");
            } else {
                __android_log_print(ANDROID_LOG_INFO, appTag, "==== Tests PASSED ====");
            }

            VkTestFramework::Finish();

            fclose(stdout);
            fclose(stderr);

            ANativeActivity_finish(app->activity);

            return;
        }
    }
}
#endif

int main(int argc, char **argv) {
    int result;

#ifdef ANDROID
    int vulkanSupport = InitVulkan();
    if (vulkanSupport == 0)
        return 1;
#endif

    ::testing::InitGoogleTest(&argc, argv);
    VkTestFramework::InitArgs(&argc, argv);

    ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    VkTestFramework::Finish();
    return result;
}
