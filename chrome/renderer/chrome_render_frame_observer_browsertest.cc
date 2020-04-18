// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/chrome_render_frame_observer.h"

#include <tuple>

#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "chrome/test/base/chrome_render_view_test.h"
#include "components/translate/content/common/translate.mojom.h"
#include "components/translate/content/renderer/translate_helper.h"
#include "components/translate/core/common/translate_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/web/web_view.h"

namespace {

class FakeContentTranslateDriver
    : public translate::mojom::ContentTranslateDriver {
 public:
  FakeContentTranslateDriver()
      : called_new_page_(false), page_needs_translation_(false) {}
  ~FakeContentTranslateDriver() override {}

  void BindHandle(mojo::ScopedMessagePipeHandle handle) {
    bindings_.AddBinding(this, translate::mojom::ContentTranslateDriverRequest(
                                   std::move(handle)));
  }

  // translate::mojom::ContentTranslateDriver implementation.
  void RegisterPage(translate::mojom::PagePtr page,
                    const translate::LanguageDetectionDetails& details,
                    bool page_needs_translation) override {
    called_new_page_ = true;
    page_needs_translation_ = page_needs_translation;
  }

  bool called_new_page_;
  bool page_needs_translation_;

 private:
  mojo::BindingSet<translate::mojom::ContentTranslateDriver> bindings_;
};

}  // namespace

// Constants for UMA statistic collection.
static const char kTranslateCaptureText[] = "Translate.CaptureText";

class ChromeRenderFrameObserverTest : public ChromeRenderViewTest {
 protected:
  void SetUp() override {
    ChromeRenderViewTest::SetUp();

    service_manager::InterfaceProvider* remote_interfaces =
        view_->GetMainRenderFrame()->GetRemoteInterfaces();
    service_manager::InterfaceProvider::TestApi test_api(remote_interfaces);
    test_api.SetBinderForName(
        translate::mojom::ContentTranslateDriver::Name_,
        base::Bind(&FakeContentTranslateDriver::BindHandle,
                   base::Unretained(&fake_translate_driver_)));
  }

  FakeContentTranslateDriver fake_translate_driver_;
};

TEST_F(ChromeRenderFrameObserverTest, SkipCapturingSubFrames) {
  base::HistogramTester histogram_tester;
  LoadHTML(
      "<!DOCTYPE html><body>"
      "This is a main document"
      "<iframe srcdoc=\"This a document in an iframe.\">"
      "</body>");
  view_->GetWebView()->UpdateAllLifecyclePhases();

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(fake_translate_driver_.called_new_page_);
  EXPECT_TRUE(fake_translate_driver_.page_needs_translation_)
      << "Page should be translatable.";
  // Should have 2 samples: one for preliminary capture, one for final capture.
  // If there are more, then subframes are being captured more than once.
  histogram_tester.ExpectTotalCount(kTranslateCaptureText, 2);
}
