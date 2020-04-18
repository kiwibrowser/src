// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_TEST_BLINK_WEB_UNIT_TEST_SUPPORT_H_
#define CONTENT_TEST_TEST_BLINK_WEB_UNIT_TEST_SUPPORT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "content/child/blink_platform_impl.h"
#include "content/test/mock_webblob_registry_impl.h"
#include "third_party/blink/public/platform/web_scrollbar_behavior.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"

namespace blink {
namespace scheduler {
class WebMainThreadScheduler;
}
}

namespace content {

class BlinkInterfaceProviderImpl;
class MockClipboardHost;

// An implementation of BlinkPlatformImpl for tests.
class TestBlinkWebUnitTestSupport : public BlinkPlatformImpl {
 public:
  TestBlinkWebUnitTestSupport();
  ~TestBlinkWebUnitTestSupport() override;

  blink::WebBlobRegistry* GetBlobRegistry() override;
  blink::WebIDBFactory* IdbFactory() override;

  std::unique_ptr<blink::WebURLLoaderFactory> CreateDefaultURLLoaderFactory()
      override;
  std::unique_ptr<blink::WebDataConsumerHandle> CreateDataConsumerHandle(
      mojo::ScopedDataPipeConsumerHandle handle) override;
  blink::WebString UserAgent() override;
  blink::WebString QueryLocalizedString(
      blink::WebLocalizedString::Name name) override;
  blink::WebString QueryLocalizedString(blink::WebLocalizedString::Name name,
                                        const blink::WebString& value) override;
  blink::WebString QueryLocalizedString(
      blink::WebLocalizedString::Name name,
      const blink::WebString& value1,
      const blink::WebString& value2) override;
  blink::WebString DefaultLocale() override;

  std::unique_ptr<blink::WebGestureCurve> CreateFlingAnimationCurve(
      blink::WebGestureDevice device_source,
      const blink::WebFloatPoint& velocity,
      const blink::WebSize& cumulative_scroll) override;

  blink::WebURLLoaderMockFactory* GetURLLoaderMockFactory() override;

  blink::WebThread* CurrentThread() override;

  void GetPluginList(bool refresh,
                     const blink::WebSecurityOrigin& mainFrameOrigin,
                     blink::WebPluginListBuilder* builder) override;

  std::unique_ptr<blink::WebRTCCertificateGenerator>
  CreateRTCCertificateGenerator() override;

  service_manager::Connector* GetConnector() override;
  blink::InterfaceProvider* GetInterfaceProvider() override;

  blink::WebScrollbarBehavior* ScrollbarBehavior() override;

 private:
  void BindClipboardHost(mojo::ScopedMessagePipeHandle handle);

  std::unique_ptr<service_manager::Connector> connector_;
  std::unique_ptr<BlinkInterfaceProviderImpl> blink_interface_provider_;
  MockWebBlobRegistryImpl blob_registry_;
  std::unique_ptr<MockClipboardHost> mock_clipboard_host_;
  base::ScopedTempDir file_system_root_;
  std::unique_ptr<blink::WebURLLoaderMockFactory> url_loader_factory_;
  std::unique_ptr<blink::scheduler::WebMainThreadScheduler>
      main_thread_scheduler_;
  std::unique_ptr<blink::WebThread> web_thread_;
  std::unique_ptr<blink::WebScrollbarBehavior> web_scrollbar_behavior_;

  base::WeakPtrFactory<TestBlinkWebUnitTestSupport> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestBlinkWebUnitTestSupport);
};

}  // namespace content

#endif  // CONTENT_TEST_TEST_BLINK_WEB_UNIT_TEST_SUPPORT_H_
