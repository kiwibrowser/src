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

#include "dawn/dawncpp.h"
#include "dawn_native/DawnNative.h"

#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>
#include <vector>

// Getting data back from Dawn is done in an async manners so all expectations are "deferred"
// until the end of the test. Also expectations use a copy to a MapRead buffer to get the data
// so resources should have the TransferSrc allowed usage bit if you want to add expectations on
// them.
#define EXPECT_BUFFER_U32_EQ(expected, buffer, offset)                         \
    AddBufferExpectation(__FILE__, __LINE__, buffer, offset, sizeof(uint32_t), \
                         new detail::ExpectEq<uint32_t>(expected))

#define EXPECT_BUFFER_U32_RANGE_EQ(expected, buffer, offset, count)                    \
    AddBufferExpectation(__FILE__, __LINE__, buffer, offset, sizeof(uint32_t) * count, \
                         new detail::ExpectEq<uint32_t>(expected, count))

// Test a pixel of the mip level 0 of a 2D texture.
#define EXPECT_PIXEL_RGBA8_EQ(expected, texture, x, y)                                  \
    AddTextureExpectation(__FILE__, __LINE__, texture, x, y, 1, 1, 0, 0, sizeof(RGBA8), \
                          new detail::ExpectEq<RGBA8>(expected))

#define EXPECT_TEXTURE_RGBA8_EQ(expected, texture, x, y, width, height, level, slice)     \
    AddTextureExpectation(__FILE__, __LINE__, texture, x, y, width, height, level, slice, \
                          sizeof(RGBA8),                                                  \
                          new detail::ExpectEq<RGBA8>(expected, (width) * (height)))

// Should only be used to test validation of function that can't be tested by regular validation
// tests;
#define ASSERT_DEVICE_ERROR(statement) \
    StartExpectDeviceError();          \
    statement;                         \
    FlushWire();                       \
    ASSERT_TRUE(EndExpectDeviceError());

struct RGBA8 {
    constexpr RGBA8() : RGBA8(0, 0, 0, 0) {
    }
    constexpr RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {
    }
    bool operator==(const RGBA8& other) const;
    bool operator!=(const RGBA8& other) const;

    uint8_t r, g, b, a;
};
std::ostream& operator<<(std::ostream& stream, const RGBA8& color);

struct DawnTestParam {
    explicit DawnTestParam(dawn_native::BackendType backendType) : backendType(backendType) {
    }

    dawn_native::BackendType backendType;

    std::vector<const char*> forceEnabledWorkarounds;
    std::vector<const char*> forceDisabledWorkarounds;
};

// Shorthands for backend types used in the DAWN_INSTANTIATE_TEST
extern const DawnTestParam D3D12Backend;
extern const DawnTestParam MetalBackend;
extern const DawnTestParam OpenGLBackend;
extern const DawnTestParam VulkanBackend;

DawnTestParam ForceWorkarounds(const DawnTestParam& originParam,
                               std::initializer_list<const char*> forceEnabledWorkarounds,
                               std::initializer_list<const char*> forceDisabledWorkarounds = {});

struct GLFWwindow;

namespace utils {
    class BackendBinding;
    class TerribleCommandBuffer;
}  // namespace utils

namespace detail {
    class Expectation;
}  // namespace detail

namespace dawn_wire {
    class WireClient;
    class WireServer;
}  // namespace dawn_wire

void InitDawnEnd2EndTestEnvironment(int argc, char** argv);

class DawnTestEnvironment : public testing::Environment {
  public:
    DawnTestEnvironment(int argc, char** argv);
    ~DawnTestEnvironment() = default;

    void SetUp() override;

    bool UsesWire() const;
    dawn_native::Instance* GetInstance() const;
    GLFWwindow* GetWindowForBackend(dawn_native::BackendType type) const;

  private:
    void CreateBackendWindow(dawn_native::BackendType type);

    bool mUseWire = false;
    bool mEnableBackendValidation = false;
    std::unique_ptr<dawn_native::Instance> mInstance;

    // Windows don't usually like to be bound to one API than the other, for example switching
    // from Vulkan to OpenGL causes crashes on some drivers. Because of this, we lazily created
    // a window for each backing API.
    std::unordered_map<dawn_native::BackendType, GLFWwindow*> mWindows;
};

class DawnTest : public ::testing::TestWithParam<DawnTestParam> {
  public:
    DawnTest();
    ~DawnTest();

    void SetUp() override;
    void TearDown() override;

    bool IsD3D12() const;
    bool IsMetal() const;
    bool IsOpenGL() const;
    bool IsVulkan() const;

    bool IsAMD() const;
    bool IsARM() const;
    bool IsImgTec() const;
    bool IsIntel() const;
    bool IsNvidia() const;
    bool IsQualcomm() const;

    bool IsWindows() const;
    bool IsLinux() const;
    bool IsMacOS() const;

    bool UsesWire() const;

    void StartExpectDeviceError();
    bool EndExpectDeviceError();

  protected:
    dawn::Device device;
    dawn::Queue queue;
    dawn::SwapChain swapchain;

    DawnProcTable backendProcs = {};
    DawnDevice backendDevice = nullptr;

    // Helper methods to implement the EXPECT_ macros
    std::ostringstream& AddBufferExpectation(const char* file,
                                             int line,
                                             const dawn::Buffer& buffer,
                                             uint64_t offset,
                                             uint64_t size,
                                             detail::Expectation* expectation);
    std::ostringstream& AddTextureExpectation(const char* file,
                                              int line,
                                              const dawn::Texture& texture,
                                              uint32_t x,
                                              uint32_t y,
                                              uint32_t width,
                                              uint32_t height,
                                              uint32_t level,
                                              uint32_t slice,
                                              uint32_t pixelSize,
                                              detail::Expectation* expectation);

    void WaitABit();
    void FlushWire();

    void SwapBuffersForCapture();

  private:
    // Things used to set up testing through the Wire.
    std::unique_ptr<dawn_wire::WireServer> mWireServer;
    std::unique_ptr<dawn_wire::WireClient> mWireClient;
    std::unique_ptr<utils::TerribleCommandBuffer> mC2sBuf;
    std::unique_ptr<utils::TerribleCommandBuffer> mS2cBuf;

    // Tracking for validation errors
    static void OnDeviceError(const char* message, void* userdata);
    bool mExpectError = false;
    bool mError = false;

    // MapRead buffers used to get data for the expectations
    struct ReadbackSlot {
        dawn::Buffer buffer;
        uint64_t bufferSize;
        const void* mappedData = nullptr;
    };
    std::vector<ReadbackSlot> mReadbackSlots;

    // Maps all the buffers and fill ReadbackSlot::mappedData
    void MapSlotsSynchronously();
    static void SlotMapReadCallback(DawnBufferMapAsyncStatus status,
                                    const void* data,
                                    uint64_t dataLength,
                                    void* userdata);
    size_t mNumPendingMapOperations = 0;

    // Reserve space where the data for an expectation can be copied
    struct ReadbackReservation {
        dawn::Buffer buffer;
        size_t slot;
        uint64_t offset;
    };
    ReadbackReservation ReserveReadback(uint64_t readbackSize);

    struct DeferredExpectation {
        const char* file;
        int line;
        size_t readbackSlot;
        uint64_t readbackOffset;
        uint64_t size;
        uint32_t rowBytes;
        uint32_t rowPitch;
        std::unique_ptr<detail::Expectation> expectation;
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316
        // Use unique_ptr because of missing move/copy constructors on std::basic_ostringstream
        std::unique_ptr<std::ostringstream> message;
    };
    std::vector<DeferredExpectation> mDeferredExpectations;

    // Assuming the data is mapped, checks all expectations
    void ResolveExpectations();

    std::unique_ptr<utils::BackendBinding> mBinding;

    dawn_native::PCIInfo mPCIInfo;
};

// Instantiate the test once for each backend provided after the first argument. Use it like this:
//     DAWN_INSTANTIATE_TEST(MyTestFixture, MetalBackend, OpenGLBackend)
#define DAWN_INSTANTIATE_TEST(testName, firstParam, ...)                         \
    const decltype(firstParam) testName##params[] = {firstParam, ##__VA_ARGS__}; \
    INSTANTIATE_TEST_SUITE_P(                                                    \
        , testName,                                                              \
        testing::ValuesIn(::detail::FilterBackends(                              \
            testName##params, sizeof(testName##params) / sizeof(firstParam))),   \
        ::detail::GetParamName)

// Skip a test when the given condition is satisfied.
#define DAWN_SKIP_TEST_IF(condition)                               \
    if (condition) {                                               \
        std::cout << "Test skipped: " #condition "." << std::endl; \
        return;                                                    \
    }

namespace detail {
    // Helper functions used for DAWN_INSTANTIATE_TEST
    bool IsBackendAvailable(dawn_native::BackendType type);
    std::vector<DawnTestParam> FilterBackends(const DawnTestParam* params, size_t numParams);
    std::string GetParamName(const testing::TestParamInfo<DawnTestParam>& info);

    // All classes used to implement the deferred expectations should inherit from this.
    class Expectation {
      public:
        virtual ~Expectation() = default;

        // Will be called with the buffer or texture data the expectation should check.
        virtual testing::AssertionResult Check(const void* data, size_t size) = 0;
    };

    // Expectation that checks the data is equal to some expected values.
    template <typename T>
    class ExpectEq : public Expectation {
      public:
        ExpectEq(T singleValue);
        ExpectEq(const T* values, const unsigned int count);

        testing::AssertionResult Check(const void* data, size_t size) override;

      private:
        std::vector<T> mExpected;
    };
    extern template class ExpectEq<uint8_t>;
    extern template class ExpectEq<uint32_t>;
    extern template class ExpectEq<RGBA8>;
}  // namespace detail
