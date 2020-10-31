// Copyright 2020 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/DawnTest.h"

#include "common/Math.h"
#include "common/vulkan_platform.h"
#include "dawn_native/VulkanBackend.h"
#include "dawn_native/vulkan/AdapterVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/ResourceMemoryAllocatorVk.h"
#include "dawn_native/vulkan/TextureVk.h"
#include "utils/SystemUtils.h"
#include "utils/WGPUHelpers.h"

#include <fcntl.h>
#include <gbm.h>

namespace dawn_native { namespace vulkan {

    namespace {

        class VulkanImageWrappingTestBase : public DawnTest {
          public:
            void SetUp() override {
                DAWN_SKIP_TEST_IF(UsesWire());

                gbmDevice = CreateGbmDevice();
                deviceVk = reinterpret_cast<dawn_native::vulkan::Device*>(device.Get());

                defaultGbmBo = CreateGbmBo(1, 1, true /* linear */);
                defaultStride = gbm_bo_get_stride_for_plane(defaultGbmBo, 0);
                defaultModifier = gbm_bo_get_modifier(defaultGbmBo);
                defaultFd = gbm_bo_get_fd(defaultGbmBo);

                defaultDescriptor.dimension = wgpu::TextureDimension::e2D;
                defaultDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
                defaultDescriptor.size = {1, 1, 1};
                defaultDescriptor.sampleCount = 1;
                defaultDescriptor.mipLevelCount = 1;
                defaultDescriptor.usage = wgpu::TextureUsage::OutputAttachment |
                                          wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
            }

            void TearDown() override {
                if (UsesWire())
                    return;

                gbm_bo_destroy(defaultGbmBo);
                gbm_device_destroy(gbmDevice);
            }

            gbm_device* CreateGbmDevice() {
                // Render nodes [1] are the primary interface for communicating with the GPU on
                // devices that support DRM. The actual filename of the render node is
                // implementation-specific, so we must scan through all possible filenames to find
                // one that we can use [2].
                //
                // [1] https://dri.freedesktop.org/docs/drm/gpu/drm-uapi.html#render-nodes
                // [2]
                // https://cs.chromium.org/chromium/src/ui/ozone/platform/wayland/gpu/drm_render_node_path_finder.cc
                const uint32_t kRenderNodeStart = 128;
                const uint32_t kRenderNodeEnd = kRenderNodeStart + 16;
                const std::string kRenderNodeTemplate = "/dev/dri/renderD";

                int renderNodeFd = -1;
                for (uint32_t i = kRenderNodeStart; i < kRenderNodeEnd; i++) {
                    std::string renderNode = kRenderNodeTemplate + std::to_string(i);
                    renderNodeFd = open(renderNode.c_str(), O_RDWR);
                    if (renderNodeFd >= 0)
                        break;
                }
                EXPECT_GE(renderNodeFd, 0) << "Failed to get file descriptor for render node";

                gbm_device* gbmDevice = gbm_create_device(renderNodeFd);
                EXPECT_NE(gbmDevice, nullptr) << "Failed to create GBM device";
                return gbmDevice;
            }

            gbm_bo* CreateGbmBo(uint32_t width, uint32_t height, bool linear) {
                uint32_t flags = GBM_BO_USE_RENDERING;
                if (linear)
                    flags |= GBM_BO_USE_LINEAR;
                gbm_bo* gbmBo = gbm_bo_create(gbmDevice, width, height, GBM_FORMAT_XBGR8888, flags);
                EXPECT_NE(gbmBo, nullptr) << "Failed to create GBM buffer object";
                return gbmBo;
            }

            wgpu::Texture WrapVulkanImage(wgpu::Device dawnDevice,
                                          const wgpu::TextureDescriptor* textureDescriptor,
                                          int memoryFd,
                                          uint32_t stride,
                                          uint64_t drmModifier,
                                          std::vector<int> waitFDs,
                                          bool isCleared = true,
                                          bool expectValid = true) {
                dawn_native::vulkan::ExternalImageDescriptorDmaBuf descriptor;
                descriptor.cTextureDescriptor =
                    reinterpret_cast<const WGPUTextureDescriptor*>(textureDescriptor);
                descriptor.isCleared = isCleared;
                descriptor.stride = stride;
                descriptor.drmModifier = drmModifier;
                descriptor.memoryFD = memoryFd;
                descriptor.waitFDs = waitFDs;

                WGPUTexture texture =
                    dawn_native::vulkan::WrapVulkanImage(dawnDevice.Get(), &descriptor);

                if (expectValid) {
                    EXPECT_NE(texture, nullptr) << "Failed to wrap image, are external memory / "
                                                   "semaphore extensions supported?";
                } else {
                    EXPECT_EQ(texture, nullptr);
                }

                return wgpu::Texture::Acquire(texture);
            }

            // Exports the signal from a wrapped texture and ignores it
            // We have to export the signal before destroying the wrapped texture else it's an
            // assertion failure
            void IgnoreSignalSemaphore(wgpu::Device dawnDevice, wgpu::Texture wrappedTexture) {
                int fd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(dawnDevice.Get(),
                                                                            wrappedTexture.Get());
                ASSERT_NE(fd, -1);
                close(fd);
            }

          protected:
            dawn_native::vulkan::Device* deviceVk;
            gbm_device* gbmDevice;
            wgpu::TextureDescriptor defaultDescriptor;
            gbm_bo* defaultGbmBo;
            int defaultFd;
            uint32_t defaultStride;
            uint64_t defaultModifier;
        };

    }  // anonymous namespace

    using VulkanImageWrappingValidationTests = VulkanImageWrappingTestBase;

    // Test no error occurs if the import is valid
    TEST_P(VulkanImageWrappingValidationTests, SuccessfulImport) {
        wgpu::Texture texture = WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                defaultStride, defaultModifier, {}, true, true);
        EXPECT_NE(texture.Get(), nullptr);
        IgnoreSignalSemaphore(device, texture);
    }

    // Test an error occurs if the texture descriptor is missing
    TEST_P(VulkanImageWrappingValidationTests, MissingTextureDescriptor) {
        ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                                WrapVulkanImage(device, nullptr, defaultFd, defaultStride,
                                                defaultModifier, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
        close(defaultFd);
    }

    // Test an error occurs if the texture descriptor is invalid
    TEST_P(VulkanImageWrappingValidationTests, InvalidTextureDescriptor) {
        wgpu::ChainedStruct chainedDescriptor;
        defaultDescriptor.nextInChain = &chainedDescriptor;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                                WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                defaultStride, defaultModifier, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
        close(defaultFd);
    }

    // Test an error occurs if the descriptor dimension isn't 2D
    TEST_P(VulkanImageWrappingValidationTests, InvalidTextureDimension) {
        defaultDescriptor.dimension = wgpu::TextureDimension::e1D;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                                WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                defaultStride, defaultModifier, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
        close(defaultFd);
    }

    // Test an error occurs if the descriptor mip level count isn't 1
    TEST_P(VulkanImageWrappingValidationTests, InvalidMipLevelCount) {
        defaultDescriptor.mipLevelCount = 2;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                                WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                defaultStride, defaultModifier, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
        close(defaultFd);
    }

    // Test an error occurs if the descriptor depth isn't 1
    TEST_P(VulkanImageWrappingValidationTests, InvalidDepth) {
        defaultDescriptor.size.depth = 2;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                                WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                defaultStride, defaultModifier, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
        close(defaultFd);
    }

    // Test an error occurs if the descriptor sample count isn't 1
    TEST_P(VulkanImageWrappingValidationTests, InvalidSampleCount) {
        defaultDescriptor.sampleCount = 4;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                                WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                defaultStride, defaultModifier, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
        close(defaultFd);
    }

    // Test an error occurs if we try to export the signal semaphore twice
    TEST_P(VulkanImageWrappingValidationTests, DoubleSignalSemaphoreExport) {
        wgpu::Texture texture = WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                defaultStride, defaultModifier, {}, true, true);
        ASSERT_NE(texture.Get(), nullptr);
        IgnoreSignalSemaphore(device, texture);
        ASSERT_DEVICE_ERROR(int fd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
                                device.Get(), texture.Get()));
        ASSERT_EQ(fd, -1);
    }

    // Test an error occurs if we try to export the signal semaphore from a normal texture
    TEST_P(VulkanImageWrappingValidationTests, NormalTextureSignalSemaphoreExport) {
        close(defaultFd);

        wgpu::Texture texture = device.CreateTexture(&defaultDescriptor);
        ASSERT_NE(texture.Get(), nullptr);
        ASSERT_DEVICE_ERROR(int fd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
                                device.Get(), texture.Get()));
        ASSERT_EQ(fd, -1);
    }

    // Test an error occurs if we try to export the signal semaphore from a destroyed texture
    TEST_P(VulkanImageWrappingValidationTests, DestroyedTextureSignalSemaphoreExport) {
        close(defaultFd);

        wgpu::Texture texture = device.CreateTexture(&defaultDescriptor);
        ASSERT_NE(texture.Get(), nullptr);
        texture.Destroy();
        ASSERT_DEVICE_ERROR(int fd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
                                device.Get(), texture.Get()));
        ASSERT_EQ(fd, -1);
    }

    // Fixture to test using external memory textures through different usages.
    // These tests are skipped if the harness is using the wire.
    class VulkanImageWrappingUsageTests : public VulkanImageWrappingTestBase {
      public:
        void SetUp() override {
            VulkanImageWrappingTestBase::SetUp();
            if (UsesWire()) {
                return;
            }

            // Create another device based on the original
            backendAdapter =
                reinterpret_cast<dawn_native::vulkan::Adapter*>(deviceVk->GetAdapter());
            deviceDescriptor.forceEnabledToggles = GetParam().forceEnabledWorkarounds;
            deviceDescriptor.forceDisabledToggles = GetParam().forceDisabledWorkarounds;

            secondDeviceVk = reinterpret_cast<dawn_native::vulkan::Device*>(
                backendAdapter->CreateDevice(&deviceDescriptor));
            secondDevice = wgpu::Device::Acquire(reinterpret_cast<WGPUDevice>(secondDeviceVk));
        }

      protected:
        dawn_native::vulkan::Adapter* backendAdapter;
        dawn_native::DeviceDescriptor deviceDescriptor;

        wgpu::Device secondDevice;
        dawn_native::vulkan::Device* secondDeviceVk;

        // Clear a texture on a given device
        void ClearImage(wgpu::Device dawnDevice,
                        wgpu::Texture wrappedTexture,
                        wgpu::Color clearColor) {
            wgpu::TextureView wrappedView = wrappedTexture.CreateView();

            // Submit a clear operation
            utils::ComboRenderPassDescriptor renderPassDescriptor({wrappedView}, {});
            renderPassDescriptor.cColorAttachments[0].clearColor = clearColor;
            renderPassDescriptor.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;

            wgpu::CommandEncoder encoder = dawnDevice.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
            pass.EndPass();

            wgpu::CommandBuffer commands = encoder.Finish();

            wgpu::Queue queue = dawnDevice.GetDefaultQueue();
            queue.Submit(1, &commands);
        }

        // Submits a 1x1x1 copy from source to destination
        void SimpleCopyTextureToTexture(wgpu::Device dawnDevice,
                                        wgpu::Queue dawnQueue,
                                        wgpu::Texture source,
                                        wgpu::Texture destination) {
            wgpu::TextureCopyView copySrc = utils::CreateTextureCopyView(source, 0, {0, 0, 0});
            wgpu::TextureCopyView copyDst = utils::CreateTextureCopyView(destination, 0, {0, 0, 0});

            wgpu::Extent3D copySize = {1, 1, 1};

            wgpu::CommandEncoder encoder = dawnDevice.CreateCommandEncoder();
            encoder.CopyTextureToTexture(&copySrc, &copyDst, &copySize);
            wgpu::CommandBuffer commands = encoder.Finish();

            dawnQueue.Submit(1, &commands);
        }
    };

    // Clear an image in |secondDevice|
    // Verify clear color is visible in |device|
    TEST_P(VulkanImageWrappingUsageTests, ClearImageAcrossDevices) {
        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture = WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on signalFd
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture nextWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Verify |device| sees the changes from |secondDevice|
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), nextWrappedTexture, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    // Import texture to |device| and |secondDevice|
    // Clear image in |secondDevice|
    // Verify clear color is visible in |device|
    // Verify the very first import into |device| also sees the change, since it should
    // alias the same memory
    TEST_P(VulkanImageWrappingUsageTests, ClearImageAcrossDevicesAliased) {
        // Import the image on |device|
        wgpu::Texture wrappedTextureAlias = WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                            defaultStride, defaultModifier, {});

        // Import the image on |secondDevice|
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture wrappedTexture = WrapVulkanImage(secondDevice, &defaultDescriptor, nextFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on signalFd
        nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture nextWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Verify |device| sees the changes from |secondDevice| (waits)
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), nextWrappedTexture, 0, 0);

        // Verify aliased texture sees changes from |secondDevice| (without waiting!)
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), wrappedTextureAlias, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
        IgnoreSignalSemaphore(device, wrappedTextureAlias);
    }

    // Clear an image in |secondDevice|
    // Verify clear color is not visible in |device| if we import the texture as not cleared
    TEST_P(VulkanImageWrappingUsageTests, UnclearedTextureIsCleared) {
        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture = WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on signalFd
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture nextWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd}, false);

        // Verify |device| doesn't see the changes from |secondDevice|
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), nextWrappedTexture, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    // Import a texture into |secondDevice|
    // Clear the texture on |secondDevice|
    // Issue a copy of the imported texture inside |device| to |copyDstTexture|
    // Verify the clear color from |secondDevice| is visible in |copyDstTexture|
    TEST_P(VulkanImageWrappingUsageTests, CopyTextureToTextureSrcSync) {
        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture = WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on |signalFd|
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture deviceWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Create a second texture on |device|
        wgpu::Texture copyDstTexture = device.CreateTexture(&defaultDescriptor);

        // Copy |deviceWrappedTexture| into |copyDstTexture|
        SimpleCopyTextureToTexture(device, queue, deviceWrappedTexture, copyDstTexture);

        // Verify |copyDstTexture| sees changes from |secondDevice|
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), copyDstTexture, 0, 0);

        IgnoreSignalSemaphore(device, deviceWrappedTexture);
    }

    // Import a texture into |device|
    // Clear texture with color A on |device|
    // Import same texture into |secondDevice|, waiting on the copy signal
    // Clear the new texture with color B on |secondDevice|
    // Copy color B using Texture to Texture copy on |secondDevice|
    // Import texture back into |device|, waiting on color B signal
    // Verify texture contains color B
    // If texture destination isn't synchronized, |secondDevice| could copy color B
    // into the texture first, then |device| writes color A
    TEST_P(VulkanImageWrappingUsageTests, CopyTextureToTextureDstSync) {
        // Import the image on |device|
        wgpu::Texture wrappedTexture = WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |device|
        ClearImage(device, wrappedTexture, {5 / 255.0f, 6 / 255.0f, 7 / 255.0f, 8 / 255.0f});

        int signalFd =
            dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(device.Get(), wrappedTexture.Get());

        // Import the image to |secondDevice|, making sure we wait on |signalFd|
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture secondDeviceWrappedTexture = WrapVulkanImage(
            secondDevice, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Create a texture with color B on |secondDevice|
        wgpu::Texture copySrcTexture = secondDevice.CreateTexture(&defaultDescriptor);
        ClearImage(secondDevice, copySrcTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        // Copy color B on |secondDevice|
        wgpu::Queue secondDeviceQueue = secondDevice.GetDefaultQueue();
        SimpleCopyTextureToTexture(secondDevice, secondDeviceQueue, copySrcTexture,
                                   secondDeviceWrappedTexture);

        // Re-import back into |device|, waiting on |secondDevice|'s signal
        signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
            secondDevice.Get(), secondDeviceWrappedTexture.Get());
        nextFd = gbm_bo_get_fd(defaultGbmBo);

        wgpu::Texture nextWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Verify |nextWrappedTexture| contains the color from our copy
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), nextWrappedTexture, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    // Import a texture from |secondDevice|
    // Clear the texture on |secondDevice|
    // Issue a copy of the imported texture inside |device| to |copyDstBuffer|
    // Verify the clear color from |secondDevice| is visible in |copyDstBuffer|
    TEST_P(VulkanImageWrappingUsageTests, CopyTextureToBufferSrcSync) {
        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture = WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on |signalFd|
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture deviceWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Create a destination buffer on |device|
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = 4;
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
        wgpu::Buffer copyDstBuffer = device.CreateBuffer(&bufferDesc);

        // Copy |deviceWrappedTexture| into |copyDstBuffer|
        wgpu::TextureCopyView copySrc =
            utils::CreateTextureCopyView(deviceWrappedTexture, 0, {0, 0, 0});
        wgpu::BufferCopyView copyDst = utils::CreateBufferCopyView(copyDstBuffer, 0, 256, 0);

        wgpu::Extent3D copySize = {1, 1, 1};

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyTextureToBuffer(&copySrc, &copyDst, &copySize);
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // Verify |copyDstBuffer| sees changes from |secondDevice|
        uint32_t expected = 1;
        EXPECT_BUFFER_U32_EQ(expected, copyDstBuffer, 0);

        IgnoreSignalSemaphore(device, deviceWrappedTexture);
    }

    // Import a texture into |device|
    // Clear texture with color A on |device|
    // Import same texture into |secondDevice|, waiting on the copy signal
    // Copy color B using Buffer to Texture copy on |secondDevice|
    // Import texture back into |device|, waiting on color B signal
    // Verify texture contains color B
    // If texture destination isn't synchronized, |secondDevice| could copy color B
    // into the texture first, then |device| writes color A
    TEST_P(VulkanImageWrappingUsageTests, CopyBufferToTextureDstSync) {
        // Import the image on |device|
        wgpu::Texture wrappedTexture = WrapVulkanImage(device, &defaultDescriptor, defaultFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |device|
        ClearImage(device, wrappedTexture, {5 / 255.0f, 6 / 255.0f, 7 / 255.0f, 8 / 255.0f});

        int signalFd =
            dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(device.Get(), wrappedTexture.Get());

        // Import the image to |secondDevice|, making sure we wait on |signalFd|
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture secondDeviceWrappedTexture = WrapVulkanImage(
            secondDevice, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Copy color B on |secondDevice|
        wgpu::Queue secondDeviceQueue = secondDevice.GetDefaultQueue();

        // Create a buffer on |secondDevice|
        wgpu::Buffer copySrcBuffer =
            utils::CreateBufferFromData(secondDevice, wgpu::BufferUsage::CopySrc, {0x04030201});

        // Copy |copySrcBuffer| into |secondDeviceWrappedTexture|
        wgpu::BufferCopyView copySrc = utils::CreateBufferCopyView(copySrcBuffer, 0, 256, 0);
        wgpu::TextureCopyView copyDst =
            utils::CreateTextureCopyView(secondDeviceWrappedTexture, 0, {0, 0, 0});

        wgpu::Extent3D copySize = {1, 1, 1};

        wgpu::CommandEncoder encoder = secondDevice.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&copySrc, &copyDst, &copySize);
        wgpu::CommandBuffer commands = encoder.Finish();
        secondDeviceQueue.Submit(1, &commands);

        // Re-import back into |device|, waiting on |secondDevice|'s signal
        signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
            secondDevice.Get(), secondDeviceWrappedTexture.Get());
        nextFd = gbm_bo_get_fd(defaultGbmBo);

        wgpu::Texture nextWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Verify |nextWrappedTexture| contains the color from our copy
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), nextWrappedTexture, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    // Import a texture from |secondDevice|
    // Clear the texture on |secondDevice|
    // Issue a copy of the imported texture inside |device| to |copyDstTexture|
    // Issue second copy to |secondCopyDstTexture|
    // Verify the clear color from |secondDevice| is visible in both copies
    TEST_P(VulkanImageWrappingUsageTests, DoubleTextureUsage) {
        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture = WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd,
                                                       defaultStride, defaultModifier, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on |signalFd|
        int nextFd = gbm_bo_get_fd(defaultGbmBo);
        wgpu::Texture deviceWrappedTexture = WrapVulkanImage(
            device, &defaultDescriptor, nextFd, defaultStride, defaultModifier, {signalFd});

        // Create a second texture on |device|
        wgpu::Texture copyDstTexture = device.CreateTexture(&defaultDescriptor);

        // Create a third texture on |device|
        wgpu::Texture secondCopyDstTexture = device.CreateTexture(&defaultDescriptor);

        // Copy |deviceWrappedTexture| into |copyDstTexture|
        SimpleCopyTextureToTexture(device, queue, deviceWrappedTexture, copyDstTexture);

        // Copy |deviceWrappedTexture| into |secondCopyDstTexture|
        SimpleCopyTextureToTexture(device, queue, deviceWrappedTexture, secondCopyDstTexture);

        // Verify |copyDstTexture| sees changes from |secondDevice|
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), copyDstTexture, 0, 0);

        // Verify |secondCopyDstTexture| sees changes from |secondDevice|
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), secondCopyDstTexture, 0, 0);

        IgnoreSignalSemaphore(device, deviceWrappedTexture);
    }

    // Tex A on device 3 (external export)
    // Tex B on device 2 (external export)
    // Tex C on device 1 (external export)
    // Clear color for A on device 3
    // Copy A->B on device 3
    // Copy B->C on device 2 (wait on B from previous op)
    // Copy C->D on device 1 (wait on C from previous op)
    // Verify D has same color as A
    TEST_P(VulkanImageWrappingUsageTests, ChainTextureCopy) {
        // Close |defaultFd| since this test doesn't import it anywhere
        close(defaultFd);

        // device 1 = |device|
        // device 2 = |secondDevice|
        // Create device 3
        dawn_native::vulkan::Device* thirdDeviceVk = reinterpret_cast<dawn_native::vulkan::Device*>(
            backendAdapter->CreateDevice(&deviceDescriptor));
        wgpu::Device thirdDevice =
            wgpu::Device::Acquire(reinterpret_cast<WGPUDevice>(thirdDeviceVk));

        // Make queue for device 2 and 3
        wgpu::Queue secondDeviceQueue = secondDevice.GetDefaultQueue();
        wgpu::Queue thirdDeviceQueue = thirdDevice.GetDefaultQueue();

        // Create BOs for A, B, C
        gbm_bo* gbmBoA = CreateGbmBo(1, 1, true /* linear */);
        uint32_t fdA = gbm_bo_get_fd(gbmBoA);
        uint32_t strideA = gbm_bo_get_stride_for_plane(gbmBoA, 0);
        uint64_t modifierA = gbm_bo_get_modifier(gbmBoA);

        gbm_bo* gbmBoB = CreateGbmBo(1, 1, true /* linear */);
        uint32_t fdB = gbm_bo_get_fd(gbmBoB);
        uint32_t strideB = gbm_bo_get_stride_for_plane(gbmBoB, 0);
        uint64_t modifierB = gbm_bo_get_modifier(gbmBoB);

        gbm_bo* gbmBoC = CreateGbmBo(1, 1, true /* linear */);
        uint32_t fdC = gbm_bo_get_fd(gbmBoC);
        uint32_t strideC = gbm_bo_get_stride_for_plane(gbmBoC, 0);
        uint64_t modifierC = gbm_bo_get_modifier(gbmBoC);

        // Import TexA, TexB on device 3
        wgpu::Texture wrappedTexADevice3 =
            WrapVulkanImage(thirdDevice, &defaultDescriptor, fdA, strideA, modifierA, {});

        wgpu::Texture wrappedTexBDevice3 =
            WrapVulkanImage(thirdDevice, &defaultDescriptor, fdB, strideB, modifierB, {});

        // Clear TexA
        ClearImage(thirdDevice, wrappedTexADevice3,
                   {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        // Copy A->B
        SimpleCopyTextureToTexture(thirdDevice, thirdDeviceQueue, wrappedTexADevice3,
                                   wrappedTexBDevice3);

        int signalFdTexBDevice3 = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
            thirdDevice.Get(), wrappedTexBDevice3.Get());
        IgnoreSignalSemaphore(thirdDevice, wrappedTexADevice3);

        // Import TexB, TexC on device 2
        fdB = gbm_bo_get_fd(gbmBoB);
        wgpu::Texture wrappedTexBDevice2 = WrapVulkanImage(
            secondDevice, &defaultDescriptor, fdB, strideB, modifierB, {signalFdTexBDevice3});

        wgpu::Texture wrappedTexCDevice2 =
            WrapVulkanImage(secondDevice, &defaultDescriptor, fdC, strideC, modifierC, {});

        // Copy B->C on device 2
        SimpleCopyTextureToTexture(secondDevice, secondDeviceQueue, wrappedTexBDevice2,
                                   wrappedTexCDevice2);

        int signalFdTexCDevice2 = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
            secondDevice.Get(), wrappedTexCDevice2.Get());
        IgnoreSignalSemaphore(secondDevice, wrappedTexBDevice2);

        // Import TexC on device 1
        fdC = gbm_bo_get_fd(gbmBoC);
        wgpu::Texture wrappedTexCDevice1 = WrapVulkanImage(device, &defaultDescriptor, fdC, strideC,
                                                           modifierC, {signalFdTexCDevice2});

        // Create TexD on device 1
        wgpu::Texture texD = device.CreateTexture(&defaultDescriptor);

        // Copy C->D on device 1
        SimpleCopyTextureToTexture(device, queue, wrappedTexCDevice1, texD);

        // Verify D matches clear color
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), texD, 0, 0);

        IgnoreSignalSemaphore(device, wrappedTexCDevice1);
    }

    // Tests a larger image is preserved when importing
    TEST_P(VulkanImageWrappingUsageTests, LargerImage) {
        close(defaultFd);

        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = 640;
        descriptor.size.height = 480;
        descriptor.size.depth = 1;
        descriptor.sampleCount = 1;
        descriptor.format = wgpu::TextureFormat::BGRA8Unorm;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;

        // Fill memory with textures
        std::vector<wgpu::Texture> textures;
        for (int i = 0; i < 20; i++) {
            textures.push_back(device.CreateTexture(&descriptor));
        }

        wgpu::Queue secondDeviceQueue = secondDevice.GetDefaultQueue();

        // Make an image on |secondDevice|
        gbm_bo* gbmBo = CreateGbmBo(640, 480, false /* linear */);
        uint32_t fd = gbm_bo_get_fd(gbmBo);
        uint32_t stride = gbm_bo_get_stride_for_plane(gbmBo, 0);
        uint64_t modifier = gbm_bo_get_modifier(gbmBo);

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(secondDevice, &descriptor, fd, stride, modifier, {});

        // Draw a non-trivial picture
        uint32_t width = 640, height = 480, pixelSize = 4;
        uint32_t bytesPerRow = Align(width * pixelSize, kTextureBytesPerRowAlignment);
        std::vector<unsigned char> data(bytesPerRow * (height - 1) + width * pixelSize);

        for (uint32_t row = 0; row < height; row++) {
            for (uint32_t col = 0; col < width; col++) {
                float normRow = static_cast<float>(row) / height;
                float normCol = static_cast<float>(col) / width;
                float dist = sqrt(normRow * normRow + normCol * normCol) * 3;
                dist = dist - static_cast<int>(dist);
                data[4 * (row * width + col)] = static_cast<unsigned char>(dist * 255);
                data[4 * (row * width + col) + 1] = static_cast<unsigned char>(dist * 255);
                data[4 * (row * width + col) + 2] = static_cast<unsigned char>(dist * 255);
                data[4 * (row * width + col) + 3] = 255;
            }
        }

        // Write the picture
        {
            wgpu::Buffer copySrcBuffer = utils::CreateBufferFromData(
                secondDevice, data.data(), data.size(), wgpu::BufferUsage::CopySrc);
            wgpu::BufferCopyView copySrc =
                utils::CreateBufferCopyView(copySrcBuffer, 0, bytesPerRow, 0);
            wgpu::TextureCopyView copyDst =
                utils::CreateTextureCopyView(wrappedTexture, 0, {0, 0, 0});
            wgpu::Extent3D copySize = {width, height, 1};

            wgpu::CommandEncoder encoder = secondDevice.CreateCommandEncoder();
            encoder.CopyBufferToTexture(&copySrc, &copyDst, &copySize);
            wgpu::CommandBuffer commands = encoder.Finish();
            secondDeviceQueue.Submit(1, &commands);
        }

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());
        int nextFd = gbm_bo_get_fd(gbmBo);

        // Import the image on |device|
        wgpu::Texture nextWrappedTexture =
            WrapVulkanImage(device, &descriptor, nextFd, stride, modifier, {signalFd});

        // Copy the image into a buffer for comparison
        wgpu::BufferDescriptor copyDesc;
        copyDesc.size = data.size();
        copyDesc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        wgpu::Buffer copyDstBuffer = device.CreateBuffer(&copyDesc);
        {
            wgpu::TextureCopyView copySrc =
                utils::CreateTextureCopyView(nextWrappedTexture, 0, {0, 0, 0});
            wgpu::BufferCopyView copyDst =
                utils::CreateBufferCopyView(copyDstBuffer, 0, bytesPerRow, 0);

            wgpu::Extent3D copySize = {width, height, 1};

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            encoder.CopyTextureToBuffer(&copySrc, &copyDst, &copySize);
            wgpu::CommandBuffer commands = encoder.Finish();
            queue.Submit(1, &commands);
        }

        // Check the image is not corrupted on |device|
        EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<uint32_t*>(data.data()), copyDstBuffer, 0,
                                   data.size() / 4);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    DAWN_INSTANTIATE_TEST(VulkanImageWrappingValidationTests, VulkanBackend());
    DAWN_INSTANTIATE_TEST(VulkanImageWrappingUsageTests, VulkanBackend());

}}  // namespace dawn_native::vulkan
