// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/chrome_render_view_test.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/test/frame_load_waiter.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

using blink::WebLocalFrame;

namespace extensions {
namespace {

class ScriptContextTest : public ChromeRenderViewTest {
 protected:
  GURL GetEffectiveDocumentURL(WebLocalFrame* frame) {
    return ScriptContext::GetEffectiveDocumentURL(
        frame, frame->GetDocument().Url(), true);
  }
};

TEST_F(ScriptContextTest, GetEffectiveDocumentURL) {
  GURL top_url("http://example.com/");
  GURL different_url("http://example.net/");
  GURL blank_url("about:blank");
  GURL srcdoc_url("about:srcdoc");

  const char frame_html[] =
      "<iframe name='frame1' srcdoc=\""
      "  <iframe name='frame1_1'></iframe>"
      "  <iframe name='frame1_2' sandbox=''></iframe>"
      "\"></iframe>"
      "<iframe name='frame2' sandbox='' srcdoc=\""
      "  <iframe name='frame2_1'></iframe>"
      "\"></iframe>"
      "<iframe name='frame3'></iframe>";

  const char frame3_html[] = "<iframe name='frame3_1'></iframe>";

  WebLocalFrame* frame = GetMainFrame();
  ASSERT_TRUE(frame);

  frame->LoadHTMLString(frame_html, top_url);
  content::FrameLoadWaiter(content::RenderFrame::FromWebFrame(frame)).Wait();

  WebLocalFrame* frame1 = frame->FirstChild()->ToWebLocalFrame();
  ASSERT_TRUE(frame1);
  ASSERT_EQ("frame1", frame1->AssignedName());
  WebLocalFrame* frame1_1 = frame1->FirstChild()->ToWebLocalFrame();
  ASSERT_TRUE(frame1_1);
  ASSERT_EQ("frame1_1", frame1_1->AssignedName());
  WebLocalFrame* frame1_2 = frame1_1->NextSibling()->ToWebLocalFrame();
  ASSERT_TRUE(frame1_2);
  ASSERT_EQ("frame1_2", frame1_2->AssignedName());
  WebLocalFrame* frame2 = frame1->NextSibling()->ToWebLocalFrame();
  ASSERT_TRUE(frame2);
  ASSERT_EQ("frame2", frame2->AssignedName());
  WebLocalFrame* frame2_1 = frame2->FirstChild()->ToWebLocalFrame();
  ASSERT_TRUE(frame2_1);
  ASSERT_EQ("frame2_1", frame2_1->AssignedName());
  WebLocalFrame* frame3 = frame2->NextSibling()->ToWebLocalFrame();
  ASSERT_TRUE(frame3);
  ASSERT_EQ("frame3", frame3->AssignedName());

  // Load a blank document in a frame from a different origin.
  frame3->LoadHTMLString(frame3_html, different_url);
  content::FrameLoadWaiter(content::RenderFrame::FromWebFrame(frame3)).Wait();

  WebLocalFrame* frame3_1 = frame3->FirstChild()->ToWebLocalFrame();
  ASSERT_TRUE(frame3_1);
  ASSERT_EQ("frame3_1", frame3_1->AssignedName());

  // Top-level frame
  EXPECT_EQ(GetEffectiveDocumentURL(frame), top_url);
  // top -> srcdoc = inherit
  EXPECT_EQ(GetEffectiveDocumentURL(frame1), top_url);
  // top -> srcdoc -> about:blank = inherit
  EXPECT_EQ(GetEffectiveDocumentURL(frame1_1), top_url);
  // top -> srcdoc -> about:blank sandboxed = same URL
  EXPECT_EQ(GetEffectiveDocumentURL(frame1_2), blank_url);

  // top -> srcdoc [sandboxed] = same URL
  EXPECT_EQ(GetEffectiveDocumentURL(frame2), srcdoc_url);
  // top -> srcdoc [sandboxed] -> about:blank = same URL
  EXPECT_EQ(GetEffectiveDocumentURL(frame2_1), blank_url);

  // top -> different origin = different origin
  EXPECT_EQ(GetEffectiveDocumentURL(frame3), different_url);
  // top -> different origin -> about:blank = inherit
  EXPECT_EQ(GetEffectiveDocumentURL(frame3_1), different_url);
}

TEST_F(ScriptContextTest, GetMainWorldContextForFrame) {
  // ScriptContextSet::GetMainWorldContextForFrame should work, even without an
  // existing v8::HandleScope.
  content::RenderFrame* render_frame =
      content::RenderFrame::FromWebFrame(GetMainFrame());
  ScriptContext* script_context =
      ScriptContextSet::GetMainWorldContextForFrame(render_frame);
  ASSERT_TRUE(script_context);
  EXPECT_EQ(render_frame, script_context->GetRenderFrame());
}

}  // namespace
}  // namespace extensions
