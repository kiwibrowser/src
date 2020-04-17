// Copyright 2017 The Dawn Authors
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

#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"
#include "common/Platform.h"
#include "dawn_native/DawnNative.h"
#include "dawn_wire/WireClient.h"
#include "dawn_wire/WireServer.h"
#include "utils/BackendBinding.h"
#include "utils/DawnHelpers.h"
#include "utils/SystemUtils.h"
#include "utils/TerribleCommandBuffer.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include "GLFW/glfw3.h"

namespace {

    std::string ParamName(dawn_native::BackendType type) {
        switch (type) {
            case dawn_native::BackendType::D3D12:
                return "D3D12";
            case dawn_native::BackendType::Metal:
                return "Metal";
            case dawn_native::BackendType::Null:
                return "Null";
            case dawn_native::BackendType::OpenGL:
                return "OpenGL";
            case dawn_native::BackendType::Vulkan:
                return "Vulkan";
            default:
                UNREACHABLE();
        }
    }

    const char* DeviceTypeName(dawn_native::DeviceType type) {
        switch (type) {
            case dawn_native::DeviceType::DiscreteGPU:
                return "Discrete GPU";
            case dawn_native::DeviceType::IntegratedGPU:
                return "Integrated GPU";
            case dawn_native::DeviceType::CPU:
                return "CPU";
            case dawn_native::DeviceType::Unknown:
                return "Unknown";
            default:
                UNREACHABLE();
        }
    }

    struct MapReadUserdata {
        DawnTest* test;
        size_t slot;
    };

    DawnTestEnvironment* gTestEnv = nullptr;

}  // namespace

const DawnTestParam D3D12Backend(dawn_native::BackendType::D3D12);
const DawnTestParam MetalBackend(dawn_native::BackendType::Metal);
const DawnTestParam OpenGLBackend(dawn_native::BackendType::OpenGL);
const DawnTestParam VulkanBackend(dawn_native::BackendType::Vulkan);

DawnTestParam ForceWorkarounds(const DawnTestParam& originParam,
                               std::initializer_list<const char*> forceEnabledWorkarounds,
                               std::initializer_list<const char*> forceDisabledWorkarounds) {
    DawnTestParam newTestParam = originParam;
    newTestParam.forceEnabledWorkarounds = forceEnabledWorkarounds;
    newTestParam.forceDisabledWorkarounds = forceDisabledWorkarounds;
    return newTestParam;
}

// Implementation of DawnTestEnvironment

void InitDawnEnd2EndTestEnvironment(int argc, char** argv) {
    gTestEnv = new DawnTestEnvironment(argc, argv);
    testing::AddGlobalTestEnvironment(gTestEnv);
}

DawnTestEnvironment::DawnTestEnvironment(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp("-w", argv[i]) == 0 || strcmp("--use-wire", argv[i]) == 0) {
            mUseWire = true;
            continue;
        }

        if (strcmp("-d", argv[i]) == 0 || strcmp("--enable-backend-validation", argv[i]) == 0) {
            mEnableBackendValidation = true;
            continue;
        }

        if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
            std::cout << "\n\nUsage: " << argv[0]
                      << " [GTEST_FLAGS...] [-w] [--enable-validation-layers]\n";
            std::cout << "  -w, --use-wire: Run the tests through the wire (defaults to no wire)\n";
            std::cout << "  -d, --enable-backend-validation: Enable backend validation (defaults"
                      << " to disabled)\n";
            std::cout << std::endl;
            continue;
        }
    }
}

void DawnTestEnvironment::SetUp() {
    ASSERT_TRUE(glfwInit());

    mInstance = std::make_unique<dawn_native::Instance>();
    mInstance->EnableBackendValidation(mEnableBackendValidation);

    static constexpr dawn_native::BackendType kAllBackends[] = {
        dawn_native::BackendType::D3D12,
        dawn_native::BackendType::Metal,
        dawn_native::BackendType::OpenGL,
        dawn_native::BackendType::Vulkan,
    };

    // Create a test window for each backend and discover an adapter using it.
    for (dawn_native::BackendType backend : kAllBackends) {
        if (detail::IsBackendAvailable(backend)) {
            CreateBackendWindow(backend);
            utils::DiscoverAdapter(mInstance.get(), mWindows[backend], backend);
        }
    }

    std::cout << "Testing configuration\n";
    std::cout << "---------------------\n";
    std::cout << "UseWire: " << (mUseWire ? "true" : "false") << "\n";
    std::cout << "EnableBackendValidation: " << (mEnableBackendValidation ? "true" : "false")
              << "\n";
    std::cout << "\n";

    // Preparing for outputting hex numbers
    std::cout << std::showbase << std::hex << std::setfill('0') << std::setw(4);

    std::cout << "System adapters: \n";
    for (const dawn_native::Adapter& adapter : mInstance->GetAdapters()) {
        const dawn_native::PCIInfo& pci = adapter.GetPCIInfo();

        std::ostringstream vendorId;
        std::ostringstream deviceId;
        vendorId << std::setfill('0') << std::uppercase << std::internal << std::hex << std::setw(4)
                 << pci.vendorId;
        deviceId << std::setfill('0') << std::uppercase << std::internal << std::hex << std::setw(4)
                 << pci.deviceId;

        std::cout << " - \"" << pci.name << "\"\n";
        std::cout << "   type: " << DeviceTypeName(adapter.GetDeviceType())
                  << ", backend: " << ParamName(adapter.GetBackendType()) << "\n";
        std::cout << "   vendorId: 0x" << vendorId.str() << ", deviceId: 0x" << deviceId.str()
                  << "\n";
    }
    std::cout << std::endl;
}

bool DawnTestEnvironment::UsesWire() const {
    return mUseWire;
}

dawn_native::Instance* DawnTestEnvironment::GetInstance() const {
    return mInstance.get();
}

GLFWwindow* DawnTestEnvironment::GetWindowForBackend(dawn_native::BackendType type) const {
    return mWindows.at(type);
}

void DawnTestEnvironment::CreateBackendWindow(dawn_native::BackendType type) {
    glfwDefaultWindowHints();
    utils::SetupGLFWWindowHintsForBackend(type);

    std::string windowName = "Dawn " + ParamName(type) + " test window";
    GLFWwindow* window = glfwCreateWindow(400, 400, windowName.c_str(), nullptr, nullptr);

    mWindows[type] = window;
}

// Implementation of DawnTest

DawnTest::DawnTest() = default;

DawnTest::~DawnTest() {
    // We need to destroy child objects before the Device
    mReadbackSlots.clear();
    queue = dawn::Queue();
    swapchain = dawn::SwapChain();
    device = dawn::Device();

    mWireClient = nullptr;
    mWireServer = nullptr;
    if (gTestEnv->UsesWire()) {
        backendProcs.deviceRelease(backendDevice);
    }

    dawnSetProcs(nullptr);
}

bool DawnTest::IsD3D12() const {
    return GetParam().backendType == dawn_native::BackendType::D3D12;
}

bool DawnTest::IsMetal() const {
    return GetParam().backendType == dawn_native::BackendType::Metal;
}

bool DawnTest::IsOpenGL() const {
    return GetParam().backendType == dawn_native::BackendType::OpenGL;
}

bool DawnTest::IsVulkan() const {
    return GetParam().backendType == dawn_native::BackendType::Vulkan;
}

bool DawnTest::IsAMD() const {
    return mPCIInfo.vendorId == kVendorID_AMD;
}

bool DawnTest::IsARM() const {
    return mPCIInfo.vendorId == kVendorID_ARM;
}

bool DawnTest::IsImgTec() const {
    return mPCIInfo.vendorId == kVendorID_ImgTec;
}

bool DawnTest::IsIntel() const {
    return mPCIInfo.vendorId == kVendorID_Intel;
}

bool DawnTest::IsNvidia() const {
    return mPCIInfo.vendorId == kVendorID_Nvidia;
}

bool DawnTest::IsQualcomm() const {
    return mPCIInfo.vendorId == kVendorID_Qualcomm;
}

bool DawnTest::IsWindows() const {
#ifdef DAWN_PLATFORM_WINDOWS
    return true;
#else
    return false;
#endif
}

bool DawnTest::IsLinux() const {
#ifdef DAWN_PLATFORM_LINUX
    return true;
#else
    return false;
#endif
}

bool DawnTest::IsMacOS() const {
#ifdef DAWN_PLATFORM_APPLE
    return true;
#else
    return false;
#endif
}

bool DawnTest::UsesWire() const {
    return gTestEnv->UsesWire();
}

void DawnTest::SetUp() {
    // Get an adapter for the backend to use, and create the device.
    dawn_native::Adapter backendAdapter;
    const dawn_native::BackendType backendType = GetParam().backendType;
    {
        dawn_native::Instance* instance = gTestEnv->GetInstance();
        std::vector<dawn_native::Adapter> adapters = instance->GetAdapters();

        for (const dawn_native::Adapter& adapter : adapters) {
            if (adapter.GetBackendType() == backendType) {
                backendAdapter = adapter;
                // On Metal, select the last adapter so that the discrete GPU is tested on
                // multi-GPU systems.
                // TODO(cwallez@chromium.org): Replace this with command line arguments requesting
                // a specific device / vendor ID once the macOS 10.13 SDK is rolled and correct
                // PCI info collection is implemented on Metal.
                if (backendType != dawn_native::BackendType::Metal) {
                    break;
                }
            }
        }

        ASSERT(backendAdapter);
    }

    mPCIInfo = backendAdapter.GetPCIInfo();

    for (const char* forceEnabledWorkaround : GetParam().forceEnabledWorkarounds) {
        ASSERT(gTestEnv->GetInstance()->GetToggleInfo(forceEnabledWorkaround) != nullptr);
    }
    for (const char* forceDisabledWorkaround : GetParam().forceDisabledWorkarounds) {
        ASSERT(gTestEnv->GetInstance()->GetToggleInfo(forceDisabledWorkaround) != nullptr);
    }
    dawn_native::DeviceDescriptor deviceDescriptor;
    deviceDescriptor.forceEnabledToggles = GetParam().forceEnabledWorkarounds;
    deviceDescriptor.forceDisabledToggles = GetParam().forceDisabledWorkarounds;
    backendDevice = backendAdapter.CreateDevice(&deviceDescriptor);

    backendProcs = dawn_native::GetProcs();

    // Get the test window and create the device using it (esp. for OpenGL)
    GLFWwindow* testWindow = gTestEnv->GetWindowForBackend(backendType);
    DAWN_ASSERT(testWindow != nullptr);
    mBinding.reset(utils::CreateBinding(backendType, testWindow, backendDevice));
    DAWN_ASSERT(mBinding != nullptr);

    // Choose whether to use the backend procs and devices directly, or set up the wire.
    DawnDevice cDevice = nullptr;
    DawnProcTable procs;

    if (gTestEnv->UsesWire()) {
        mC2sBuf = std::make_unique<utils::TerribleCommandBuffer>();
        mS2cBuf = std::make_unique<utils::TerribleCommandBuffer>();

        mWireServer.reset(new dawn_wire::WireServer(backendDevice, backendProcs, mS2cBuf.get()));
        mC2sBuf->SetHandler(mWireServer.get());

        mWireClient.reset(new dawn_wire::WireClient(mC2sBuf.get()));
        DawnDevice clientDevice = mWireClient->GetDevice();
        DawnProcTable clientProcs = mWireClient->GetProcs();
        mS2cBuf->SetHandler(mWireClient.get());

        procs = clientProcs;
        cDevice = clientDevice;
    } else {
        procs = backendProcs;
        cDevice = backendDevice;
    }

    // Set up the device and queue because all tests need them, and DawnTest needs them too for the
    // deferred expectations.
    dawnSetProcs(&procs);
    device = dawn::Device::Acquire(cDevice);
    queue = device.CreateQueue();

    // The swapchain isn't used by tests but is useful when debugging with graphics debuggers that
    // capture at frame boundaries.
    dawn::SwapChainDescriptor swapChainDesc;
    swapChainDesc.implementation = mBinding->GetSwapChainImplementation();
    swapchain = device.CreateSwapChain(&swapChainDesc);
    FlushWire();
    swapchain.Configure(
        static_cast<dawn::TextureFormat>(mBinding->GetPreferredSwapChainTextureFormat()),
        dawn::TextureUsageBit::OutputAttachment, 400, 400);

    device.SetErrorCallback(OnDeviceError, this);
}

void DawnTest::TearDown() {
    swapchain = dawn::SwapChain();
    FlushWire();

    MapSlotsSynchronously();
    ResolveExpectations();

    for (size_t i = 0; i < mReadbackSlots.size(); ++i) {
        mReadbackSlots[i].buffer.Unmap();
    }
}

void DawnTest::StartExpectDeviceError() {
    mExpectError = true;
    mError = false;
}
bool DawnTest::EndExpectDeviceError() {
    mExpectError = false;
    return mError;
}

// static
void DawnTest::OnDeviceError(const char* message, void* userdata) {
    DawnTest* self = static_cast<DawnTest*>(userdata);

    ASSERT_TRUE(self->mExpectError) << "Got unexpected device error: " << message;
    ASSERT_FALSE(self->mError) << "Got two errors in expect block";
    self->mError = true;
}

std::ostringstream& DawnTest::AddBufferExpectation(const char* file,
                                                   int line,
                                                   const dawn::Buffer& buffer,
                                                   uint64_t offset,
                                                   uint64_t size,
                                                   detail::Expectation* expectation) {
    auto readback = ReserveReadback(size);

    // We need to enqueue the copy immediately because by the time we resolve the expectation,
    // the buffer might have been modified.
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(buffer, offset, readback.buffer, readback.offset, size);

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    DeferredExpectation deferred;
    deferred.file = file;
    deferred.line = line;
    deferred.readbackSlot = readback.slot;
    deferred.readbackOffset = readback.offset;
    deferred.size = size;
    deferred.rowBytes = size;
    deferred.rowPitch = size;
    deferred.expectation.reset(expectation);

    mDeferredExpectations.push_back(std::move(deferred));
    mDeferredExpectations.back().message = std::make_unique<std::ostringstream>();
    return *(mDeferredExpectations.back().message.get());
}

std::ostringstream& DawnTest::AddTextureExpectation(const char* file,
                                                    int line,
                                                    const dawn::Texture& texture,
                                                    uint32_t x,
                                                    uint32_t y,
                                                    uint32_t width,
                                                    uint32_t height,
                                                    uint32_t level,
                                                    uint32_t slice,
                                                    uint32_t pixelSize,
                                                    detail::Expectation* expectation) {
    uint32_t rowPitch = Align(width * pixelSize, kTextureRowPitchAlignment);
    uint32_t size = rowPitch * (height - 1) + width * pixelSize;

    auto readback = ReserveReadback(size);

    // We need to enqueue the copy immediately because by the time we resolve the expectation,
    // the texture might have been modified.
    dawn::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, level, slice, {x, y, 0});
    dawn::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(readback.buffer, readback.offset, rowPitch, 0);
    dawn::Extent3D copySize = {width, height, 1};

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    DeferredExpectation deferred;
    deferred.file = file;
    deferred.line = line;
    deferred.readbackSlot = readback.slot;
    deferred.readbackOffset = readback.offset;
    deferred.size = size;
    deferred.rowBytes = width * pixelSize;
    deferred.rowPitch = rowPitch;
    deferred.expectation.reset(expectation);

    mDeferredExpectations.push_back(std::move(deferred));
    mDeferredExpectations.back().message = std::make_unique<std::ostringstream>();
    return *(mDeferredExpectations.back().message.get());
}

void DawnTest::WaitABit() {
    device.Tick();
    FlushWire();

    utils::USleep(100);
}

void DawnTest::SwapBuffersForCapture() {
    // Insert a frame boundary for API capture tools.
    dawn::Texture backBuffer = swapchain.GetNextTexture();
    swapchain.Present(backBuffer);
}

void DawnTest::FlushWire() {
    if (gTestEnv->UsesWire()) {
        bool C2SFlushed = mC2sBuf->Flush();
        bool S2CFlushed = mS2cBuf->Flush();
        ASSERT(C2SFlushed);
        ASSERT(S2CFlushed);
    }
}

DawnTest::ReadbackReservation DawnTest::ReserveReadback(uint64_t readbackSize) {
    // For now create a new MapRead buffer for each readback
    // TODO(cwallez@chromium.org): eventually make bigger buffers and allocate linearly?
    dawn::BufferDescriptor descriptor;
    descriptor.size = readbackSize;
    descriptor.usage = dawn::BufferUsageBit::MapRead | dawn::BufferUsageBit::TransferDst;

    ReadbackSlot slot;
    slot.bufferSize = readbackSize;
    slot.buffer = device.CreateBuffer(&descriptor);

    ReadbackReservation reservation;
    reservation.buffer = slot.buffer;
    reservation.slot = mReadbackSlots.size();
    reservation.offset = 0;

    mReadbackSlots.push_back(std::move(slot));
    return reservation;
}

void DawnTest::MapSlotsSynchronously() {
    // Initialize numPendingMapOperations before mapping, just in case the callback is called
    // immediately.
    mNumPendingMapOperations = mReadbackSlots.size();

    // Map all readback slots
    for (size_t i = 0; i < mReadbackSlots.size(); ++i) {
        MapReadUserdata* userdata = new MapReadUserdata{this, i};

        auto& slot = mReadbackSlots[i];
        slot.buffer.MapReadAsync(SlotMapReadCallback, userdata);
    }

    // Busy wait until all map operations are done.
    while (mNumPendingMapOperations != 0) {
        WaitABit();
    }
}

// static
void DawnTest::SlotMapReadCallback(DawnBufferMapAsyncStatus status,
                                   const void* data,
                                   uint64_t,
                                   void* userdata_) {
    DAWN_ASSERT(status == DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS);

    auto userdata = static_cast<MapReadUserdata*>(userdata_);
    userdata->test->mReadbackSlots[userdata->slot].mappedData = data;
    userdata->test->mNumPendingMapOperations--;

    delete userdata;
}

void DawnTest::ResolveExpectations() {
    for (const auto& expectation : mDeferredExpectations) {
        DAWN_ASSERT(mReadbackSlots[expectation.readbackSlot].mappedData != nullptr);

        // Get a pointer to the mapped copy of the data for the expectation.
        const char* data =
            static_cast<const char*>(mReadbackSlots[expectation.readbackSlot].mappedData);
        data += expectation.readbackOffset;

        uint32_t size;
        std::vector<char> packedData;
        if (expectation.rowBytes != expectation.rowPitch) {
            DAWN_ASSERT(expectation.rowPitch > expectation.rowBytes);
            uint32_t rowCount =
                (expectation.size + expectation.rowPitch - 1) / expectation.rowPitch;
            uint32_t packedSize = rowCount * expectation.rowBytes;
            packedData.resize(packedSize);
            for (uint32_t r = 0; r < rowCount; ++r) {
                for (uint32_t i = 0; i < expectation.rowBytes; ++i) {
                    packedData[i + r * expectation.rowBytes] = data[i + r * expectation.rowPitch];
                }
            }
            data = packedData.data();
            size = packedSize;
        } else {
            size = expectation.size;
        }

        // Get the result for the expectation and add context to failures
        testing::AssertionResult result = expectation.expectation->Check(data, size);
        if (!result) {
            result << " Expectation created at " << expectation.file << ":" << expectation.line
                   << std::endl;
            result << expectation.message->str();
        }

        EXPECT_TRUE(result);
    }
}

bool RGBA8::operator==(const RGBA8& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool RGBA8::operator!=(const RGBA8& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& stream, const RGBA8& color) {
    return stream << "RGBA8(" << static_cast<int>(color.r) << ", " << static_cast<int>(color.g)
                  << ", " << static_cast<int>(color.b) << ", " << static_cast<int>(color.a) << ")";
}

namespace detail {
    bool IsBackendAvailable(dawn_native::BackendType type) {
        switch (type) {
#if defined(DAWN_ENABLE_BACKEND_D3D12)
            case dawn_native::BackendType::D3D12:
#endif
#if defined(DAWN_ENABLE_BACKEND_METAL)
            case dawn_native::BackendType::Metal:
#endif
#if defined(DAWN_ENABLE_BACKEND_OPENGL)
            case dawn_native::BackendType::OpenGL:
#endif
#if defined(DAWN_ENABLE_BACKEND_VULKAN)
            case dawn_native::BackendType::Vulkan:
#endif
                return true;

            default:
                return false;
        }
    }

    std::vector<DawnTestParam> FilterBackends(const DawnTestParam* params, size_t numParams) {
        std::vector<DawnTestParam> backends;

        for (size_t i = 0; i < numParams; ++i) {
            if (IsBackendAvailable(params[i].backendType)) {
                backends.push_back(params[i]);
            }
        }
        return backends;
    }

    std::string GetParamName(const testing::TestParamInfo<DawnTestParam>& info) {
        std::ostringstream ostream;
        ostream << ParamName(info.param.backendType);

        for (const char* forceEnabledWorkaround : info.param.forceEnabledWorkarounds) {
            ostream << "_" << forceEnabledWorkaround;
        }

        return ostream.str();
    }

    // Helper classes to set expectations

    template <typename T>
    ExpectEq<T>::ExpectEq(T singleValue) {
        mExpected.push_back(singleValue);
    }

    template <typename T>
    ExpectEq<T>::ExpectEq(const T* values, const unsigned int count) {
        mExpected.assign(values, values + count);
    }

    template <typename T>
    testing::AssertionResult ExpectEq<T>::Check(const void* data, size_t size) {
        DAWN_ASSERT(size == sizeof(T) * mExpected.size());

        const T* actual = static_cast<const T*>(data);

        testing::AssertionResult failure = testing::AssertionFailure();
        for (size_t i = 0; i < mExpected.size(); ++i) {
            if (actual[i] != mExpected[i]) {
                testing::AssertionResult result = testing::AssertionFailure()
                                                  << "Expected data[" << i << "] to be "
                                                  << mExpected[i] << ", actual " << actual[i]
                                                  << std::endl;

                auto printBuffer = [&](const T* buffer) {
                    static constexpr unsigned int kBytes = sizeof(T);

                    for (size_t index = 0; index < mExpected.size(); ++index) {
                        auto byteView = reinterpret_cast<const uint8_t*>(buffer + index);
                        for (unsigned int b = 0; b < kBytes; ++b) {
                            char buf[4];
                            sprintf(buf, "%02X ", byteView[b]);
                            result << buf;
                        }
                    }
                    result << std::endl;
                };

                if (mExpected.size() <= 1024) {
                    result << "Expected:" << std::endl;
                    printBuffer(mExpected.data());

                    result << "Actual:" << std::endl;
                    printBuffer(actual);
                }

                return result;
            }
        }

        return testing::AssertionSuccess();
    }

    template class ExpectEq<uint8_t>;
    template class ExpectEq<uint32_t>;
    template class ExpectEq<RGBA8>;
}  // namespace detail
