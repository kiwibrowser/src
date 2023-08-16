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

#include "common/Log.h"
#include "common/Platform.h"
#include "dawn/dawn_proc.h"
#include "dawn_native/DawnNative.h"
#include "tests/DawnTest.h"
#include "utils/GLFWUtils.h"

#include <gtest/gtest.h>

#include <cstdlib>

// Include windows.h before GLFW so GLFW's APIENTRY macro doesn't conflict with windows.h's.
#if defined(DAWN_PLATFORM_WINDOWS)
#    include "common/windows_with_undefs.h"
#endif  // defined(DAWN_PLATFORM_WINDOWS)

#include "GLFW/glfw3.h"

#if defined(DAWN_USE_X11)
#    include "common/xlib_with_undefs.h"
#endif  // defined(DAWN_USE_X11)

#if defined(DAWN_ENABLE_BACKEND_METAL)
#    include "utils/ObjCUtils.h"
#endif  // defined(DAWN_ENABLE_BACKEND_METAL)

#include "GLFW/glfw3native.h"

// Test for wgpu::Surface creation that only need an instance (no devices) and don't need all the
// complexity of DawnTest.
class WindowSurfaceInstanceTests : public testing::Test {
  public:
    void SetUp() override {
        glfwSetErrorCallback([](int code, const char* message) {
            dawn::ErrorLog() << "GLFW error " << code << " " << message;
        });
        DAWN_SKIP_TEST_IF(!glfwInit());

        DawnProcTable procs = dawn_native::GetProcs();
        dawnProcSetProcs(&procs);

        mInstance = wgpu::CreateInstance();
    }

    void TearDown() override {
        if (mWindow != nullptr) {
            glfwDestroyWindow(mWindow);
            mWindow = nullptr;
        }
    }

    void AssertSurfaceCreation(const wgpu::SurfaceDescriptor* descriptor, bool succeeds) {
        ASSERT_EQ(mInstance.CreateSurface(descriptor).Get() != nullptr, succeeds);
    }

    GLFWwindow* CreateWindow() {
        // The WindowSurfaceInstance tests don't create devices so we don't need to call
        // SetupGLFWWindowHintsForBackend. Set GLFW_NO_API anyway to avoid GLFW bringing up a GL
        // context that we won't use.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        mWindow = glfwCreateWindow(400, 400, "WindowSurfaceInstanceTests window", nullptr, nullptr);
        return mWindow;
    }

  private:
    wgpu::Instance mInstance;
    GLFWwindow* mWindow = nullptr;
};

// Test that a valid chained descriptor works (and that GLFWUtils creates a valid chained
// descriptor).
TEST_F(WindowSurfaceInstanceTests, ControlCase) {
    GLFWwindow* window = CreateWindow();
    std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor =
        utils::SetupWindowAndGetSurfaceDescriptorForTesting(window);

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = chainedDescriptor.get();

    AssertSurfaceCreation(&descriptor, true);
}

// Test that just wgpu::SurfaceDescriptor isn't enough and needs a chained descriptor.
TEST_F(WindowSurfaceInstanceTests, NoChainedDescriptors) {
    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = nullptr;  // That's the default value but we set it for clarity.

    AssertSurfaceCreation(&descriptor, false);
}

// Test that a chained descriptor with a garbage sType produces an error.
TEST_F(WindowSurfaceInstanceTests, BadChainedDescriptors) {
    wgpu::ChainedStruct chainedDescriptor;
    chainedDescriptor.sType = wgpu::SType::Invalid;  // The default but we set it for clarity.

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;

    AssertSurfaceCreation(&descriptor, false);
}

// Test that a chained descriptor with HTMLCanvas produces an error.
TEST_F(WindowSurfaceInstanceTests, HTMLCanvasDescriptor) {
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector chainedDescriptor;
    chainedDescriptor.selector = "#myCanvas";

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;

    AssertSurfaceCreation(&descriptor, false);
}

// Test that it is invalid to give two valid chained descriptors
TEST_F(WindowSurfaceInstanceTests, TwoChainedDescriptors) {
    GLFWwindow* window = CreateWindow();
    std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor1 =
        utils::SetupWindowAndGetSurfaceDescriptorForTesting(window);
    std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor2 =
        utils::SetupWindowAndGetSurfaceDescriptorForTesting(window);

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = chainedDescriptor1.get();
    chainedDescriptor1->nextInChain = chainedDescriptor2.get();

    AssertSurfaceCreation(&descriptor, false);
}

#if defined(DAWN_PLATFORM_WINDOWS)

// Tests that GLFWUtils returns a descriptor of HWND type
TEST_F(WindowSurfaceInstanceTests, CorrectSTypeHWND) {
    GLFWwindow* window = CreateWindow();
    std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor =
        utils::SetupWindowAndGetSurfaceDescriptorForTesting(window);
    ASSERT_EQ(chainedDescriptor->sType, wgpu::SType::SurfaceDescriptorFromWindowsHWND);
}

// Test with setting an invalid hwnd
TEST_F(WindowSurfaceInstanceTests, InvalidHWND) {
    wgpu::SurfaceDescriptorFromWindowsHWND chainedDescriptor;
    chainedDescriptor.hinstance = GetModuleHandle(nullptr);
    chainedDescriptor.hwnd = 0;  // This always is an invalid HWND value.

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;
    AssertSurfaceCreation(&descriptor, false);
}

#else  // defined(DAWN_PLATFORM_WINDOWS)

// Test using HWND when it is not supported
TEST_F(WindowSurfaceInstanceTests, HWNDSurfacesAreInvalid) {
    wgpu::SurfaceDescriptorFromWindowsHWND chainedDescriptor;
    chainedDescriptor.hinstance = nullptr;
    chainedDescriptor.hwnd = 0;

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;
    AssertSurfaceCreation(&descriptor, false);
}

#endif  // defined(DAWN_PLATFORM_WINDOWS)

#if defined(DAWN_USE_X11)

// Tests that GLFWUtils returns a descriptor of Xlib type
TEST_F(WindowSurfaceInstanceTests, CorrectSTypeXlib) {
    GLFWwindow* window = CreateWindow();
    std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor =
        utils::SetupWindowAndGetSurfaceDescriptorForTesting(window);
    ASSERT_EQ(chainedDescriptor->sType, wgpu::SType::SurfaceDescriptorFromXlib);
}

// Test with setting an invalid window
TEST_F(WindowSurfaceInstanceTests, InvalidXWindow) {
    wgpu::SurfaceDescriptorFromXlib chainedDescriptor;
    chainedDescriptor.display = XOpenDisplay(nullptr);
    // From the "X Window System Protocol" "X Version 11, Release 6.8" page 2 at
    // https://www.x.org/releases/X11R7.5/doc/x11proto/proto.pdf
    //    WINDOW 32-bit value (top three bits guaranteed to be zero.
    // So UINT32_MAX should be an invalid window.
    chainedDescriptor.window = 0xFFFFFFFF;

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;
    AssertSurfaceCreation(&descriptor, false);
}

#else  // defined(DAWN_USE_X11)

// Test using Xlib when it is not supported
TEST_F(WindowSurfaceInstanceTests, XlibSurfacesAreInvalid) {
    wgpu::SurfaceDescriptorFromXlib chainedDescriptor;
    chainedDescriptor.display = nullptr;
    chainedDescriptor.window = 0;

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;
    AssertSurfaceCreation(&descriptor, false);
}

#endif  // defined(DAWN_USE_X11)

#if defined(DAWN_ENABLE_BACKEND_METAL)

// Tests that GLFWUtils returns a descriptor of Metal type
TEST_F(WindowSurfaceInstanceTests, CorrectSTypeMetal) {
    GLFWwindow* window = CreateWindow();
    std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor =
        utils::SetupWindowAndGetSurfaceDescriptorForTesting(window);
    ASSERT_EQ(chainedDescriptor->sType, wgpu::SType::SurfaceDescriptorFromMetalLayer);
}

// Test with setting an invalid layer
TEST_F(WindowSurfaceInstanceTests, InvalidMetalLayer) {
    wgpu::SurfaceDescriptorFromMetalLayer chainedDescriptor;
    // The CALayer is autoreleased. Releasing it causes a test failure when the Chromium GTest
    // autoreleasepool is emptied.
    chainedDescriptor.layer = utils::CreateDummyCALayer();

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;
    AssertSurfaceCreation(&descriptor, false);
}

#else  // defined(DAWN_ENABLE_BACKEND_METAL)

// Test using Metal when it is not supported
TEST_F(WindowSurfaceInstanceTests, MetalSurfacesAreInvalid) {
    wgpu::SurfaceDescriptorFromMetalLayer chainedDescriptor;
    chainedDescriptor.layer = nullptr;

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &chainedDescriptor;
    AssertSurfaceCreation(&descriptor, false);
}

#endif  // defined(DAWN_ENABLE_BACKEND_METAL)
