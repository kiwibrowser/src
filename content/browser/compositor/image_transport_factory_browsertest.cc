// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/image_transport_factory.h"

#include "base/run_loop.h"
#include "build/build_config.h"
#include "components/viz/common/gpu/context_provider.h"
#include "content/browser/compositor/owned_mailbox.h"
#include "content/public/test/content_browser_test.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/config/gpu_feature_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/compositor/compositor.h"

namespace content {
namespace {

typedef ContentBrowserTest ImageTransportFactoryBrowserTest;

class MockContextFactoryObserver : public ui::ContextFactoryObserver {
 public:
  MOCK_METHOD0(OnLostResources, void());
};

// This crashes on Mac ASAN: http://crbug.com/335083
// Flaky on ChromeOS: crbug.com/394083
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
#define MAYBE_TestLostContext DISABLED_TestLostContext
#else
#define MAYBE_TestLostContext TestLostContext
#endif
// Checks that upon context loss, the observer is called and the created
// resources are reset.
IN_PROC_BROWSER_TEST_F(ImageTransportFactoryBrowserTest,
                       MAYBE_TestLostContext) {
  ImageTransportFactory* factory = ImageTransportFactory::GetInstance();

  // This test doesn't make sense in software compositing mode.
  scoped_refptr<viz::ContextProvider> context_provider =
      factory->GetContextFactory()->SharedMainThreadContextProvider();
  if (context_provider->GetGpuFeatureInfo()
          .status_values[gpu::GPU_FEATURE_TYPE_GPU_COMPOSITING] !=
      gpu::kGpuFeatureStatusEnabled) {
    return;
  }

  scoped_refptr<OwnedMailbox> mailbox =
      new OwnedMailbox(factory->GetGLHelper());
  EXPECT_FALSE(mailbox->mailbox().IsZero());

  MockContextFactoryObserver observer;
  factory->GetContextFactory()->AddObserver(&observer);

  base::RunLoop run_loop;
  EXPECT_CALL(observer, OnLostResources())
      .WillOnce(testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));

  gpu::gles2::GLES2Interface* gl = context_provider->ContextGL();
  gl->LoseContextCHROMIUM(GL_GUILTY_CONTEXT_RESET_ARB,
                          GL_INNOCENT_CONTEXT_RESET_ARB);

  // We have to flush to make sure that the client side gets a chance to notice
  // the context is gone.
  gl->Flush();

  run_loop.Run();
  EXPECT_TRUE(mailbox->mailbox().IsZero());

  factory->GetContextFactory()->RemoveObserver(&observer);
}

class ImageTransportFactoryTearDownBrowserTest : public ContentBrowserTest {
 public:
  ImageTransportFactoryTearDownBrowserTest() {}

  void TearDown() override {
    if (mailbox_.get())
      EXPECT_TRUE(mailbox_->mailbox().IsZero());
    ContentBrowserTest::TearDown();
  }

 protected:
  scoped_refptr<OwnedMailbox> mailbox_;
};

// This crashes on Mac. ImageTransportFactory is NULL unless
// --enable-delegated-renderer is passed, and after that, we'd need to spawn a
// renderer and get a frame before we create a browser compositor, necessary for
// the GLHelper to not be NULL.
// http://crbug.com/335083
#if defined(OS_MACOSX)
#define MAYBE_LoseOnTearDown DISABLED_LoseOnTearDown
#else
#define MAYBE_LoseOnTearDown LoseOnTearDown
#endif
// Checks that upon destruction of the ImageTransportFactory, the observer is
// called and the created resources are reset.
IN_PROC_BROWSER_TEST_F(ImageTransportFactoryTearDownBrowserTest,
                       MAYBE_LoseOnTearDown) {
  ImageTransportFactory* factory = ImageTransportFactory::GetInstance();
  // This test doesn't make sense in software compositing mode.
  scoped_refptr<viz::ContextProvider> context_provider =
      factory->GetContextFactory()->SharedMainThreadContextProvider();
  if (context_provider->GetGpuFeatureInfo()
          .status_values[gpu::GPU_FEATURE_TYPE_GPU_COMPOSITING] !=
      gpu::kGpuFeatureStatusEnabled) {
    return;
  }

  viz::GLHelper* helper = factory->GetGLHelper();
  ASSERT_TRUE(helper);
  mailbox_ = new OwnedMailbox(helper);
  EXPECT_FALSE(mailbox_->mailbox().IsZero());
}

}  // anonymous namespace
}  // namespace content
