// Copyright 2019 The Dawn Authors
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

#include "common/Math.h"
#include "tests/DawnTest.h"

#include "common/vulkan_platform.h"
#include "dawn_native/VulkanBackend.h"
#include "dawn_native/vulkan/AdapterVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/ResourceMemoryAllocatorVk.h"
#include "dawn_native/vulkan/TextureVk.h"
#include "utils/SystemUtils.h"
#include "utils/WGPUHelpers.h"

namespace dawn_native { namespace vulkan {

    namespace {

        class VulkanImageWrappingTestBase : public DawnTest {
          public:
            void SetUp() override {
                DawnTest::SetUp();
                DAWN_SKIP_TEST_IF(UsesWire());

                deviceVk = reinterpret_cast<dawn_native::vulkan::Device*>(device.Get());
            }

            // Creates a VkImage with external memory
            ::VkResult CreateImage(dawn_native::vulkan::Device* deviceVk,
                                   uint32_t width,
                                   uint32_t height,
                                   VkFormat format,
                                   VkImage* image) {
                VkExternalMemoryImageCreateInfoKHR externalInfo;
                externalInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR;
                externalInfo.pNext = nullptr;
                externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

                auto usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT;

                VkImageCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                createInfo.pNext = &externalInfo;
                createInfo.flags = VK_IMAGE_CREATE_ALIAS_BIT_KHR;
                createInfo.imageType = VK_IMAGE_TYPE_2D;
                createInfo.format = format;
                createInfo.extent = {width, height, 1};
                createInfo.mipLevels = 1;
                createInfo.arrayLayers = 1;
                createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                createInfo.usage = usage;
                createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0;
                createInfo.pQueueFamilyIndices = nullptr;
                createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                return deviceVk->fn.CreateImage(deviceVk->GetVkDevice(), &createInfo, nullptr,
                                                &**image);
            }

            // Allocates memory for an image
            ::VkResult AllocateMemory(dawn_native::vulkan::Device* deviceVk,
                                      VkImage handle,
                                      VkDeviceMemory* allocation,
                                      VkDeviceSize* allocationSize,
                                      uint32_t* memoryTypeIndex) {
                // Create the image memory and associate it with the container
                VkMemoryRequirements requirements;
                deviceVk->fn.GetImageMemoryRequirements(deviceVk->GetVkDevice(), handle,
                                                        &requirements);

                // Import memory from file descriptor
                VkExportMemoryAllocateInfoKHR externalInfo;
                externalInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
                externalInfo.pNext = nullptr;
                externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

                int bestType = deviceVk->GetResourceMemoryAllocatorForTesting()->FindBestTypeIndex(
                    requirements, false);
                VkMemoryAllocateInfo allocateInfo;
                allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocateInfo.pNext = &externalInfo;
                allocateInfo.allocationSize = requirements.size;
                allocateInfo.memoryTypeIndex = static_cast<uint32_t>(bestType);

                *allocationSize = allocateInfo.allocationSize;
                *memoryTypeIndex = allocateInfo.memoryTypeIndex;

                return deviceVk->fn.AllocateMemory(deviceVk->GetVkDevice(), &allocateInfo, nullptr,
                                                   &**allocation);
            }

            // Binds memory to an image
            ::VkResult BindMemory(dawn_native::vulkan::Device* deviceVk,
                                  VkImage handle,
                                  VkDeviceMemory* memory) {
                return deviceVk->fn.BindImageMemory(deviceVk->GetVkDevice(), handle, *memory, 0);
            }

            // Extracts a file descriptor representing memory on a device
            int GetMemoryFd(dawn_native::vulkan::Device* deviceVk, VkDeviceMemory memory) {
                VkMemoryGetFdInfoKHR getFdInfo;
                getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
                getFdInfo.pNext = nullptr;
                getFdInfo.memory = memory;
                getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

                int memoryFd = -1;
                deviceVk->fn.GetMemoryFdKHR(deviceVk->GetVkDevice(), &getFdInfo, &memoryFd);

                EXPECT_GE(memoryFd, 0) << "Failed to get file descriptor for external memory";
                return memoryFd;
            }

            // Prepares and exports memory for an image on a given device
            void CreateBindExportImage(dawn_native::vulkan::Device* deviceVk,
                                       uint32_t width,
                                       uint32_t height,
                                       VkFormat format,
                                       VkImage* handle,
                                       VkDeviceMemory* allocation,
                                       VkDeviceSize* allocationSize,
                                       uint32_t* memoryTypeIndex,
                                       int* memoryFd) {
                ::VkResult result = CreateImage(deviceVk, width, height, format, handle);
                EXPECT_EQ(result, VK_SUCCESS) << "Failed to create external image";

                ::VkResult resultBool =
                    AllocateMemory(deviceVk, *handle, allocation, allocationSize, memoryTypeIndex);
                EXPECT_EQ(resultBool, VK_SUCCESS) << "Failed to allocate external memory";

                result = BindMemory(deviceVk, *handle, allocation);
                EXPECT_EQ(result, VK_SUCCESS) << "Failed to bind image memory";

                *memoryFd = GetMemoryFd(deviceVk, *allocation);
            }

            // Wraps a vulkan image from external memory
            wgpu::Texture WrapVulkanImage(wgpu::Device dawnDevice,
                                          const wgpu::TextureDescriptor* textureDescriptor,
                                          int memoryFd,
                                          VkDeviceSize allocationSize,
                                          uint32_t memoryTypeIndex,
                                          std::vector<int> waitFDs,
                                          bool isCleared = true,
                                          bool expectValid = true) {
                dawn_native::vulkan::ExternalImageDescriptorOpaqueFD descriptor;
                descriptor.cTextureDescriptor =
                    reinterpret_cast<const WGPUTextureDescriptor*>(textureDescriptor);
                descriptor.isCleared = isCleared;
                descriptor.allocationSize = allocationSize;
                descriptor.memoryTypeIndex = memoryTypeIndex;
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
        };

    }  // anonymous namespace

    class VulkanImageWrappingValidationTests : public VulkanImageWrappingTestBase {
      public:
        void SetUp() override {
            VulkanImageWrappingTestBase::SetUp();
            if (UsesWire()) {
                return;
            }

            CreateBindExportImage(deviceVk, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &defaultImage,
                                  &defaultAllocation, &defaultAllocationSize,
                                  &defaultMemoryTypeIndex, &defaultFd);
            defaultDescriptor.dimension = wgpu::TextureDimension::e2D;
            defaultDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
            defaultDescriptor.size = {1, 1, 1};
            defaultDescriptor.sampleCount = 1;
            defaultDescriptor.mipLevelCount = 1;
            defaultDescriptor.usage = wgpu::TextureUsage::OutputAttachment |
                                      wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
        }

        void TearDown() override {
            if (UsesWire()) {
                VulkanImageWrappingTestBase::TearDown();
                return;
            }

            deviceVk->GetFencedDeleter()->DeleteWhenUnused(defaultImage);
            deviceVk->GetFencedDeleter()->DeleteWhenUnused(defaultAllocation);
            VulkanImageWrappingTestBase::TearDown();
        }

      protected:
        wgpu::TextureDescriptor defaultDescriptor;
        VkImage defaultImage;
        VkDeviceMemory defaultAllocation;
        VkDeviceSize defaultAllocationSize;
        uint32_t defaultMemoryTypeIndex;
        int defaultFd;
    };

    // Test no error occurs if the import is valid
    TEST_P(VulkanImageWrappingValidationTests, SuccessfulImport) {
        DAWN_SKIP_TEST_IF(UsesWire());
        wgpu::Texture texture =
            WrapVulkanImage(device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {}, true, true);
        EXPECT_NE(texture.Get(), nullptr);
        IgnoreSignalSemaphore(device, texture);
    }

    // Test an error occurs if the texture descriptor is missing
    TEST_P(VulkanImageWrappingValidationTests, MissingTextureDescriptor) {
        DAWN_SKIP_TEST_IF(UsesWire());
        ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                                WrapVulkanImage(device, nullptr, defaultFd, defaultAllocationSize,
                                                defaultMemoryTypeIndex, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
    }

    // Test an error occurs if the texture descriptor is invalid
    TEST_P(VulkanImageWrappingValidationTests, InvalidTextureDescriptor) {
        DAWN_SKIP_TEST_IF(UsesWire());
        wgpu::ChainedStruct chainedDescriptor;
        defaultDescriptor.nextInChain = &chainedDescriptor;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture = WrapVulkanImage(
                                device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                                defaultMemoryTypeIndex, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
    }

    // Test an error occurs if the descriptor dimension isn't 2D
    TEST_P(VulkanImageWrappingValidationTests, InvalidTextureDimension) {
        DAWN_SKIP_TEST_IF(UsesWire());
        defaultDescriptor.dimension = wgpu::TextureDimension::e1D;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture = WrapVulkanImage(
                                device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                                defaultMemoryTypeIndex, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
    }

    // Test an error occurs if the descriptor mip level count isn't 1
    TEST_P(VulkanImageWrappingValidationTests, InvalidMipLevelCount) {
        DAWN_SKIP_TEST_IF(UsesWire());
        defaultDescriptor.mipLevelCount = 2;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture = WrapVulkanImage(
                                device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                                defaultMemoryTypeIndex, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
    }

    // Test an error occurs if the descriptor depth isn't 1
    TEST_P(VulkanImageWrappingValidationTests, InvalidDepth) {
        DAWN_SKIP_TEST_IF(UsesWire());
        defaultDescriptor.size.depth = 2;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture = WrapVulkanImage(
                                device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                                defaultMemoryTypeIndex, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
    }

    // Test an error occurs if the descriptor sample count isn't 1
    TEST_P(VulkanImageWrappingValidationTests, InvalidSampleCount) {
        DAWN_SKIP_TEST_IF(UsesWire());
        defaultDescriptor.sampleCount = 4;

        ASSERT_DEVICE_ERROR(wgpu::Texture texture = WrapVulkanImage(
                                device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                                defaultMemoryTypeIndex, {}, true, false));
        EXPECT_EQ(texture.Get(), nullptr);
    }

    // Test an error occurs if we try to export the signal semaphore twice
    TEST_P(VulkanImageWrappingValidationTests, DoubleSignalSemaphoreExport) {
        DAWN_SKIP_TEST_IF(UsesWire());
        wgpu::Texture texture =
            WrapVulkanImage(device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {}, true, true);
        ASSERT_NE(texture.Get(), nullptr);
        IgnoreSignalSemaphore(device, texture);
        ASSERT_DEVICE_ERROR(int fd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
                                device.Get(), texture.Get()));
        ASSERT_EQ(fd, -1);
    }

    // Test an error occurs if we try to export the signal semaphore from a normal texture
    TEST_P(VulkanImageWrappingValidationTests, NormalTextureSignalSemaphoreExport) {
        DAWN_SKIP_TEST_IF(UsesWire());
        wgpu::Texture texture = device.CreateTexture(&defaultDescriptor);
        ASSERT_NE(texture.Get(), nullptr);
        ASSERT_DEVICE_ERROR(int fd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
                                device.Get(), texture.Get()));
        ASSERT_EQ(fd, -1);
    }

    // Test an error occurs if we try to export the signal semaphore from a destroyed texture
    TEST_P(VulkanImageWrappingValidationTests, DestroyedTextureSignalSemaphoreExport) {
        DAWN_SKIP_TEST_IF(UsesWire());
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

            CreateBindExportImage(deviceVk, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &defaultImage,
                                  &defaultAllocation, &defaultAllocationSize,
                                  &defaultMemoryTypeIndex, &defaultFd);
            defaultDescriptor.dimension = wgpu::TextureDimension::e2D;
            defaultDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
            defaultDescriptor.size = {1, 1, 1};
            defaultDescriptor.sampleCount = 1;
            defaultDescriptor.arrayLayerCount = 1;
            defaultDescriptor.mipLevelCount = 1;
            defaultDescriptor.usage = wgpu::TextureUsage::OutputAttachment |
                                      wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
        }

        void TearDown() override {
            if (UsesWire()) {
                VulkanImageWrappingTestBase::TearDown();
                return;
            }

            deviceVk->GetFencedDeleter()->DeleteWhenUnused(defaultImage);
            deviceVk->GetFencedDeleter()->DeleteWhenUnused(defaultAllocation);
            VulkanImageWrappingTestBase::TearDown();
        }

      protected:
        wgpu::Device secondDevice;
        dawn_native::vulkan::Device* secondDeviceVk;

        dawn_native::vulkan::Adapter* backendAdapter;
        dawn_native::DeviceDescriptor deviceDescriptor;

        wgpu::TextureDescriptor defaultDescriptor;
        VkImage defaultImage;
        VkDeviceMemory defaultAllocation;
        VkDeviceSize defaultAllocationSize;
        uint32_t defaultMemoryTypeIndex;
        int defaultFd;

        // Clear a texture on a given device
        void ClearImage(wgpu::Device dawnDevice,
                        wgpu::Texture wrappedTexture,
                        wgpu::Color clearColor) {
            wgpu::TextureView wrappedView = wrappedTexture.CreateView();

            // Submit a clear operation
            utils::ComboRenderPassDescriptor renderPassDescriptor({wrappedView}, {});
            renderPassDescriptor.cColorAttachments[0].clearColor = clearColor;

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
            wgpu::TextureCopyView copySrc;
            copySrc.texture = source;
            copySrc.mipLevel = 0;
            copySrc.arrayLayer = 0;
            copySrc.origin = {0, 0, 0};

            wgpu::TextureCopyView copyDst;
            copyDst.texture = destination;
            copyDst.mipLevel = 0;
            copyDst.arrayLayer = 0;
            copyDst.origin = {0, 0, 0};

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
        DAWN_SKIP_TEST_IF(UsesWire());

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on signalFd
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture nextWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

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
        DAWN_SKIP_TEST_IF(UsesWire());

        // WrapVulkanImage consumes the file descriptor so we can't import defaultFd twice.
        // Duplicate the file descriptor so we can import it twice.
        int defaultFdCopy = dup(defaultFd);
        ASSERT(defaultFdCopy != -1);

        // Import the image on |device
        wgpu::Texture wrappedTextureAlias =
            WrapVulkanImage(device, &defaultDescriptor, defaultFdCopy, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on signalFd
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture nextWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

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
        DAWN_SKIP_TEST_IF(UsesWire());

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on signalFd
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture nextWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd}, false);

        // Verify |device| doesn't see the changes from |secondDevice|
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), nextWrappedTexture, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    // Import a texture into |secondDevice|
    // Issue a copy of the imported texture inside |device| to |copyDstTexture|
    // Verify the clear color from |secondDevice| is visible in |copyDstTexture|
    TEST_P(VulkanImageWrappingUsageTests, CopyTextureToTextureSrcSync) {
        DAWN_SKIP_TEST_IF(UsesWire());

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on |signalFd|
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture deviceWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

        // Create a second texture on |device|
        wgpu::Texture copyDstTexture = device.CreateTexture(&defaultDescriptor);

        // Copy |deviceWrappedTexture| into |copyDstTexture|
        SimpleCopyTextureToTexture(device, queue, deviceWrappedTexture, copyDstTexture);

        // Verify |copyDstTexture| sees changes from |secondDevice|
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), copyDstTexture, 0, 0);

        IgnoreSignalSemaphore(device, deviceWrappedTexture);
    }

    // Import a texture into |device|
    // Copy color A into texture on |device|
    // Import same texture into |secondDevice|, waiting on the copy signal
    // Copy color B using Texture to Texture copy on |secondDevice|
    // Import texture back into |device|, waiting on color B signal
    // Verify texture contains color B
    // If texture destination isn't synchronized, |secondDevice| could copy color B
    // into the texture first, then |device| writes color A
    TEST_P(VulkanImageWrappingUsageTests, CopyTextureToTextureDstSync) {
        DAWN_SKIP_TEST_IF(UsesWire());

        // Import the image on |device|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |device|
        ClearImage(device, wrappedTexture, {5 / 255.0f, 6 / 255.0f, 7 / 255.0f, 8 / 255.0f});

        int signalFd =
            dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(device.Get(), wrappedTexture.Get());

        // Import the image to |secondDevice|, making sure we wait on |signalFd|
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture secondDeviceWrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

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
        memoryFd = GetMemoryFd(deviceVk, defaultAllocation);

        wgpu::Texture nextWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

        // Verify |nextWrappedTexture| contains the color from our copy
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), nextWrappedTexture, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    // Import a texture from |secondDevice|
    // Issue a copy of the imported texture inside |device| to |copyDstBuffer|
    // Verify the clear color from |secondDevice| is visible in |copyDstBuffer|
    TEST_P(VulkanImageWrappingUsageTests, CopyTextureToBufferSrcSync) {
        DAWN_SKIP_TEST_IF(UsesWire());

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on |signalFd|
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture deviceWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

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
        uint32_t expected = 0x04030201;
        EXPECT_BUFFER_U32_EQ(expected, copyDstBuffer, 0);

        IgnoreSignalSemaphore(device, deviceWrappedTexture);
    }

    // Import a texture into |device|
    // Copy color A into texture on |device|
    // Import same texture into |secondDevice|, waiting on the copy signal
    // Copy color B using Buffer to Texture copy on |secondDevice|
    // Import texture back into |device|, waiting on color B signal
    // Verify texture contains color B
    // If texture destination isn't synchronized, |secondDevice| could copy color B
    // into the texture first, then |device| writes color A
    TEST_P(VulkanImageWrappingUsageTests, CopyBufferToTextureDstSync) {
        DAWN_SKIP_TEST_IF(UsesWire());

        // Import the image on |device|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |device|
        ClearImage(device, wrappedTexture, {5 / 255.0f, 6 / 255.0f, 7 / 255.0f, 8 / 255.0f});

        int signalFd =
            dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(device.Get(), wrappedTexture.Get());

        // Import the image to |secondDevice|, making sure we wait on |signalFd|
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture secondDeviceWrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

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
        memoryFd = GetMemoryFd(deviceVk, defaultAllocation);

        wgpu::Texture nextWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

        // Verify |nextWrappedTexture| contains the color from our copy
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), nextWrappedTexture, 0, 0);

        IgnoreSignalSemaphore(device, nextWrappedTexture);
    }

    // Import a texture from |secondDevice|
    // Issue a copy of the imported texture inside |device| to |copyDstTexture|
    // Issue second copy to |secondCopyDstTexture|
    // Verify the clear color from |secondDevice| is visible in both copies
    TEST_P(VulkanImageWrappingUsageTests, DoubleTextureUsage) {
        DAWN_SKIP_TEST_IF(UsesWire());

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture =
            WrapVulkanImage(secondDevice, &defaultDescriptor, defaultFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {});

        // Clear |wrappedTexture| on |secondDevice|
        ClearImage(secondDevice, wrappedTexture, {1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f});

        int signalFd = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(secondDevice.Get(),
                                                                          wrappedTexture.Get());

        // Import the image to |device|, making sure we wait on |signalFd|
        int memoryFd = GetMemoryFd(deviceVk, defaultAllocation);
        wgpu::Texture deviceWrappedTexture =
            WrapVulkanImage(device, &defaultDescriptor, memoryFd, defaultAllocationSize,
                            defaultMemoryTypeIndex, {signalFd});

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
        DAWN_SKIP_TEST_IF(UsesWire());

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

        // Allocate memory for A, B, C
        VkImage imageA;
        VkDeviceMemory allocationA;
        int memoryFdA;
        VkDeviceSize allocationSizeA;
        uint32_t memoryTypeIndexA;
        CreateBindExportImage(thirdDeviceVk, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &imageA, &allocationA,
                              &allocationSizeA, &memoryTypeIndexA, &memoryFdA);

        VkImage imageB;
        VkDeviceMemory allocationB;
        int memoryFdB;
        VkDeviceSize allocationSizeB;
        uint32_t memoryTypeIndexB;
        CreateBindExportImage(secondDeviceVk, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &imageB, &allocationB,
                              &allocationSizeB, &memoryTypeIndexB, &memoryFdB);

        VkImage imageC;
        VkDeviceMemory allocationC;
        int memoryFdC;
        VkDeviceSize allocationSizeC;
        uint32_t memoryTypeIndexC;
        CreateBindExportImage(deviceVk, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &imageC, &allocationC,
                              &allocationSizeC, &memoryTypeIndexC, &memoryFdC);

        // Import TexA, TexB on device 3
        wgpu::Texture wrappedTexADevice3 = WrapVulkanImage(
            thirdDevice, &defaultDescriptor, memoryFdA, allocationSizeA, memoryTypeIndexA, {});

        wgpu::Texture wrappedTexBDevice3 = WrapVulkanImage(
            thirdDevice, &defaultDescriptor, memoryFdB, allocationSizeB, memoryTypeIndexB, {});

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
        memoryFdB = GetMemoryFd(secondDeviceVk, allocationB);
        wgpu::Texture wrappedTexBDevice2 =
            WrapVulkanImage(secondDevice, &defaultDescriptor, memoryFdB, allocationSizeB,
                            memoryTypeIndexB, {signalFdTexBDevice3});

        wgpu::Texture wrappedTexCDevice2 = WrapVulkanImage(
            secondDevice, &defaultDescriptor, memoryFdC, allocationSizeC, memoryTypeIndexC, {});

        // Copy B->C on device 2
        SimpleCopyTextureToTexture(secondDevice, secondDeviceQueue, wrappedTexBDevice2,
                                   wrappedTexCDevice2);

        int signalFdTexCDevice2 = dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(
            secondDevice.Get(), wrappedTexCDevice2.Get());
        IgnoreSignalSemaphore(secondDevice, wrappedTexBDevice2);

        // Import TexC on device 1
        memoryFdC = GetMemoryFd(deviceVk, allocationC);
        wgpu::Texture wrappedTexCDevice1 =
            WrapVulkanImage(device, &defaultDescriptor, memoryFdC, allocationSizeC,
                            memoryTypeIndexC, {signalFdTexCDevice2});

        // Create TexD on device 1
        wgpu::Texture texD = device.CreateTexture(&defaultDescriptor);

        // Copy C->D on device 1
        SimpleCopyTextureToTexture(device, queue, wrappedTexCDevice1, texD);

        // Verify D matches clear color
        EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 3, 4), texD, 0, 0);

        thirdDeviceVk->GetFencedDeleter()->DeleteWhenUnused(imageA);
        thirdDeviceVk->GetFencedDeleter()->DeleteWhenUnused(allocationA);
        secondDeviceVk->GetFencedDeleter()->DeleteWhenUnused(imageB);
        secondDeviceVk->GetFencedDeleter()->DeleteWhenUnused(allocationB);
        deviceVk->GetFencedDeleter()->DeleteWhenUnused(imageC);
        deviceVk->GetFencedDeleter()->DeleteWhenUnused(allocationC);

        IgnoreSignalSemaphore(device, wrappedTexCDevice1);
    }

    // Tests a larger image is preserved when importing
    // TODO(http://crbug.com/dawn/206): This fails on AMD
    TEST_P(VulkanImageWrappingUsageTests, LargerImage) {
        DAWN_SKIP_TEST_IF(UsesWire() || IsAMD());

        close(defaultFd);

        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = 640;
        descriptor.size.height = 480;
        descriptor.size.depth = 1;
        descriptor.arrayLayerCount = 1;
        descriptor.sampleCount = 1;
        descriptor.format = wgpu::TextureFormat::BGRA8Unorm;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;

        // Fill memory with textures to trigger layout issues on AMD
        std::vector<wgpu::Texture> textures;
        for (int i = 0; i < 20; i++) {
            textures.push_back(device.CreateTexture(&descriptor));
        }

        wgpu::Queue secondDeviceQueue = secondDevice.GetDefaultQueue();

        // Make an image on |secondDevice|
        VkImage imageA;
        VkDeviceMemory allocationA;
        int memoryFdA;
        VkDeviceSize allocationSizeA;
        uint32_t memoryTypeIndexA;
        CreateBindExportImage(secondDeviceVk, 640, 480, VK_FORMAT_R8G8B8A8_UNORM, &imageA,
                              &allocationA, &allocationSizeA, &memoryTypeIndexA, &memoryFdA);

        // Import the image on |secondDevice|
        wgpu::Texture wrappedTexture = WrapVulkanImage(secondDevice, &descriptor, memoryFdA,
                                                       allocationSizeA, memoryTypeIndexA, {});

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
        int memoryFd = GetMemoryFd(secondDeviceVk, allocationA);

        // Import the image on |device|
        wgpu::Texture nextWrappedTexture = WrapVulkanImage(
            device, &descriptor, memoryFd, allocationSizeA, memoryTypeIndexA, {signalFd});

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
        secondDeviceVk->GetFencedDeleter()->DeleteWhenUnused(imageA);
        secondDeviceVk->GetFencedDeleter()->DeleteWhenUnused(allocationA);
    }

    DAWN_INSTANTIATE_TEST(VulkanImageWrappingValidationTests, VulkanBackend());
    DAWN_INSTANTIATE_TEST(VulkanImageWrappingUsageTests, VulkanBackend());

}}  // namespace dawn_native::vulkan
