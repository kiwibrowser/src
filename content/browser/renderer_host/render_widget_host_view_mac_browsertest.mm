// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_mac.h"

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace content {

namespace {

class TextCallbackWaiter {
 public:
  TextCallbackWaiter() {}

  void Wait() { run_loop_.Run(); }

  const base::string16& text() const { return text_; }

  void GetText(const base::string16& text) {
    text_ = text;
    run_loop_.Quit();
  }

 private:
  base::string16 text_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TextCallbackWaiter);
};

}  // namespace

class RenderWidgetHostViewMacTest : public ContentBrowserTest {};

IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewMacTest, GetPageTextForSpeech) {
  GURL url(
      "data:text/html,<span>Hello</span>"
      "<span style='display:none'>Goodbye</span>"
      "<span>World</span>");
  EXPECT_TRUE(NavigateToURL(shell(), url));

  RenderWidgetHostView* rwhv =
      shell()->web_contents()->GetMainFrame()->GetView();
  RenderWidgetHostViewMac* rwhv_mac =
      static_cast<RenderWidgetHostViewMac*>(rwhv);

  TextCallbackWaiter waiter;
  rwhv_mac->GetPageTextForSpeech(
      base::BindOnce(&TextCallbackWaiter::GetText, base::Unretained(&waiter)));
  waiter.Wait();

  EXPECT_EQ(base::ASCIIToUTF16("Hello\nWorld"), waiter.text());
}

}  // namespace content
