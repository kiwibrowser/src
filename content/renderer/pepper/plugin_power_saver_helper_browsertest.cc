// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/macros.h"
#include "base/run_loop.h"
#include "content/common/frame_messages.h"
#include "content/common/view_message_enums.h"
#include "content/public/common/content_constants.h"
#include "content/public/test/frame_load_waiter.h"
#include "content/public/test/render_view_test.h"
#include "content/renderer/pepper/plugin_power_saver_helper.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_view_impl.h"
#include "content/test/test_render_frame.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_plugin_params.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace content {

class PluginPowerSaverHelperTest : public RenderViewTest {
 public:
  PluginPowerSaverHelperTest() : sink_(nullptr) {}

  void SetUp() override {
    RenderViewTest::SetUp();
    sink_ = &render_thread_->sink();
  }

  RenderFrameImpl* frame() {
    return static_cast<RenderFrameImpl*>(view_->GetMainRenderFrame());
  }

 protected:
  IPC::TestSink* sink_;

  DISALLOW_COPY_AND_ASSIGN(PluginPowerSaverHelperTest);
};

TEST_F(PluginPowerSaverHelperTest, TemporaryOriginWhitelist) {
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_PERIPHERAL,
            frame()->GetPeripheralContentStatus(
                url::Origin::Create(GURL("http://same.com")),
                url::Origin::Create(GURL("http://other.com")),
                gfx::Size(100, 100), RenderFrame::DONT_RECORD_DECISION));

  // Clear out other messages so we find just the plugin power saver IPCs.
  sink_->ClearMessages();

  frame()->WhitelistContentOrigin(
      url::Origin::Create(GURL("http://other.com")));

  EXPECT_EQ(RenderFrame::CONTENT_STATUS_ESSENTIAL_CROSS_ORIGIN_WHITELISTED,
            frame()->GetPeripheralContentStatus(
                url::Origin::Create(GURL("http://same.com")),
                url::Origin::Create(GURL("http://other.com")),
                gfx::Size(100, 100), RenderFrame::DONT_RECORD_DECISION));

  // Test that we've sent an IPC to the browser.
  ASSERT_EQ(1u, sink_->message_count());
  const IPC::Message* msg = sink_->GetMessageAt(0);
  EXPECT_EQ(static_cast<uint32_t>(FrameHostMsg_PluginContentOriginAllowed::ID),
            msg->type());
  FrameHostMsg_PluginContentOriginAllowed::Param params;
  FrameHostMsg_PluginContentOriginAllowed::Read(msg, &params);
  EXPECT_TRUE(url::Origin::Create(GURL("http://other.com"))
                  .IsSameOriginWith(std::get<0>(params)));
}

TEST_F(PluginPowerSaverHelperTest, UnthrottleOnExPostFactoWhitelist) {
  base::RunLoop loop;
  frame()->RegisterPeripheralPlugin(
      url::Origin::Create(GURL("http://other.com")), loop.QuitClosure());

  std::set<url::Origin> origin_whitelist;
  origin_whitelist.insert(url::Origin::Create(GURL("http://other.com")));
  frame()->OnMessageReceived(FrameMsg_UpdatePluginContentOriginWhitelist(
      frame()->GetRoutingID(), origin_whitelist));

  // Runs until the unthrottle closure is run.
  loop.Run();
}

TEST_F(PluginPowerSaverHelperTest, ClearWhitelistOnNavigate) {
  frame()->WhitelistContentOrigin(
      url::Origin::Create(GURL("http://other.com")));

  EXPECT_EQ(RenderFrame::CONTENT_STATUS_ESSENTIAL_CROSS_ORIGIN_WHITELISTED,
            frame()->GetPeripheralContentStatus(
                url::Origin::Create(GURL("http://same.com")),
                url::Origin::Create(GURL("http://other.com")),
                gfx::Size(100, 100), RenderFrame::DONT_RECORD_DECISION));

  LoadHTML("<html></html>");

  EXPECT_EQ(RenderFrame::CONTENT_STATUS_PERIPHERAL,
            frame()->GetPeripheralContentStatus(
                url::Origin::Create(GURL("http://same.com")),
                url::Origin::Create(GURL("http://other.com")),
                gfx::Size(100, 100), RenderFrame::DONT_RECORD_DECISION));
}

}  // namespace content
