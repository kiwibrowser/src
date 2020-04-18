// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/path_service.h"
#include "build/build_config.h"
#include "components/pdf/renderer/pdf_accessibility_tree.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_accessibility.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "content/public/test/render_view_test.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/base/resource/resource_bundle.h"

namespace pdf {
namespace {

class FakeRendererPpapiHost : public content::RendererPpapiHost {
 public:
  explicit FakeRendererPpapiHost(content::RenderFrame* render_frame)
      : render_frame_(render_frame) {}
  ~FakeRendererPpapiHost() override {}

  ppapi::host::PpapiHost* GetPpapiHost() override { return nullptr; }
  bool IsValidInstance(PP_Instance instance) const override { return true; }
  content::PepperPluginInstance* GetPluginInstance(
      PP_Instance instance) const override {
    return nullptr;
  }
  content::RenderFrame* GetRenderFrameForInstance(
      PP_Instance instance) const override {
    return render_frame_;
  }
  content::RenderView* GetRenderViewForInstance(
      PP_Instance instance) const override {
    return nullptr;
  }
  blink::WebPluginContainer* GetContainerForInstance(
      PP_Instance instance) const override {
    return nullptr;
  }
  bool HasUserGesture(PP_Instance instance) const override { return false; }
  int GetRoutingIDForWidget(PP_Instance instance) const override { return 0; }
  gfx::Point PluginPointToRenderFrame(PP_Instance instance,
                                      const gfx::Point& pt) const override {
    return gfx::Point();
  }
  IPC::PlatformFileForTransit ShareHandleWithRemote(
      base::PlatformFile handle,
      bool should_close_source) override {
    return IPC::PlatformFileForTransit();
  }
  base::SharedMemoryHandle ShareSharedMemoryHandleWithRemote(
      const base::SharedMemoryHandle& handle) override {
    return base::SharedMemoryHandle();
  }
  bool IsRunningInProcess() const override { return false; }
  std::string GetPluginName() const override { return std::string(); }
  void SetToExternalPluginHost() override {}
  void CreateBrowserResourceHosts(
      PP_Instance instance,
      const std::vector<IPC::Message>& nested_msgs,
      const base::Callback<void(const std::vector<int>&)>& callback)
      const override {}
  GURL GetDocumentURL(PP_Instance instance) const override { return GURL(); }

 private:
  content::RenderFrame* render_frame_;
};

}  // namespace

class PdfAccessibilityTreeTest : public content::RenderViewTest {
 public:
  PdfAccessibilityTreeTest() {}
  ~PdfAccessibilityTreeTest() override {}

  void SetUp() override {
    content::RenderViewTest::SetUp();

    base::FilePath pak_dir;
    base::PathService::Get(base::DIR_MODULE, &pak_dir);
    base::FilePath pak_file =
        pak_dir.Append(FILE_PATH_LITERAL("components_tests_resources.pak"));
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        pak_file, ui::SCALE_FACTOR_NONE);

    viewport_info_.zoom = 1.0;
    viewport_info_.scroll = {0, 0};
    viewport_info_.offset = {0, 0};
    viewport_info_.selection_start_page_index = 0;
    viewport_info_.selection_start_char_index = 0;
    viewport_info_.selection_end_page_index = 0;
    viewport_info_.selection_end_char_index = 0;
    doc_info_.page_count = 1;
    page_info_.page_index = 0;
    page_info_.text_run_count = 0;
    page_info_.char_count = 0;
    page_info_.bounds = PP_MakeRectFromXYWH(0, 0, 1, 1);
  }

 protected:
  PP_PrivateAccessibilityViewportInfo viewport_info_;
  PP_PrivateAccessibilityDocInfo doc_info_;
  PP_PrivateAccessibilityPageInfo page_info_;
  std::vector<PP_PrivateAccessibilityTextRunInfo> text_runs_;
  std::vector<PP_PrivateAccessibilityCharInfo> chars_;
};

TEST_F(PdfAccessibilityTreeTest, TestEmptyPDFPage) {
  content::RenderFrame* render_frame = view_->GetMainRenderFrame();
  render_frame->SetAccessibilityModeForTest(ui::AXMode::kWebContents);
  ASSERT_TRUE(render_frame->GetRenderAccessibility());

  FakeRendererPpapiHost host(view_->GetMainRenderFrame());
  PP_Instance instance = 0;
  pdf::PdfAccessibilityTree pdf_accessibility_tree(&host, instance);

  pdf_accessibility_tree.SetAccessibilityViewportInfo(viewport_info_);
  pdf_accessibility_tree.SetAccessibilityDocInfo(doc_info_);
  pdf_accessibility_tree.SetAccessibilityPageInfo(page_info_, text_runs_,
                                                  chars_);

  EXPECT_EQ(ax::mojom::Role::kGroup,
            pdf_accessibility_tree.GetRoot()->data().role);
}

TEST_F(PdfAccessibilityTreeTest, TestAccessibilityDisabledDuringPDFLoad) {
  content::RenderFrame* render_frame = view_->GetMainRenderFrame();
  render_frame->SetAccessibilityModeForTest(ui::AXMode::kWebContents);
  ASSERT_TRUE(render_frame->GetRenderAccessibility() != nullptr);

  FakeRendererPpapiHost host(view_->GetMainRenderFrame());
  PP_Instance instance = 0;
  pdf::PdfAccessibilityTree pdf_accessibility_tree(&host, instance);

  pdf_accessibility_tree.SetAccessibilityViewportInfo(viewport_info_);
  pdf_accessibility_tree.SetAccessibilityDocInfo(doc_info_);

  // Disable accessibility while the PDF is loading, make sure this
  // doesn't crash.
  blink::WebView* web_view = render_frame->GetRenderView()->GetWebView();
  blink::WebSettings* settings = web_view->GetSettings();
  settings->SetAccessibilityEnabled(false);

  pdf_accessibility_tree.SetAccessibilityPageInfo(page_info_, text_runs_,
                                                  chars_);
}

}  // namespace pdf
