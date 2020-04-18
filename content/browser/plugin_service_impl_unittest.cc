// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/plugin_service_impl.h"

#include <memory>

#include "build/build_config.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/browser/ppapi_plugin_process_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

constexpr char kURL1[] = "http://google.com/";
constexpr char kURL2[] = "http://youtube.com/";
constexpr char kPepperBrokerEvent[] = "Pepper.Broker";

class TestBrokerClient : public PpapiPluginProcessHost::BrokerClient {
 public:
  void GetPpapiChannelInfo(base::ProcessHandle* renderer_handle,
                           int* renderer_id) override {}
  void OnPpapiChannelOpened(const IPC::ChannelHandle& channel_handle,
                            base::ProcessId plugin_pid,
                            int plugin_child_id) override {}
  bool Incognito() override { return false; }
};

}  // anonymous namespace

class PluginServiceImplTest : public RenderViewHostTestHarness {
 public:
  PluginServiceImplTest() = default;
  ~PluginServiceImplTest() override = default;

  void SetUp() override {
    RenderViewHostTestHarness::SetUp();

    test_ukm_recorder_ = std::make_unique<ukm::TestAutoSetUkmRecorder>();
  }

  bool RecordedBrokerEvent(const GURL& url) {
    RunAllPendingInMessageLoop(BrowserThread::UI);
    auto entries = test_ukm_recorder_->GetEntriesByName(kPepperBrokerEvent);
    for (const auto* const entry : entries) {
      const ukm::UkmSource* source =
          test_ukm_recorder_->GetSourceForSourceId(entry->source_id);
      if (source && source->url() == url)
        return true;
    }
    return false;
  }

  void ResetUKM() {
    test_ukm_recorder_ = std::make_unique<ukm::TestAutoSetUkmRecorder>();
  }

 private:
  std::unique_ptr<ukm::TestUkmRecorder> test_ukm_recorder_;

  DISALLOW_COPY_AND_ASSIGN(PluginServiceImplTest);
};

TEST_F(PluginServiceImplTest, RecordBrokerUsage) {
  TestBrokerClient client;

  NavigateAndCommit(GURL(kURL1));
  PluginServiceImpl* service = PluginServiceImpl::GetInstance();

  // Internal usage of the broker should not record metrics. Internal usage will
  // not pass a RFH.
  service->OpenChannelToPpapiBroker(0, 0, base::FilePath(), &client);
  EXPECT_FALSE(RecordedBrokerEvent(GURL(kURL1)));

  // Top level frame usage should be recorded.
  int render_process_id = main_rfh()->GetProcess()->GetID();
  int render_frame_id = main_rfh()->GetRoutingID();
  service->OpenChannelToPpapiBroker(render_process_id, render_frame_id,
                                    base::FilePath(), &client);
  EXPECT_TRUE(RecordedBrokerEvent(GURL(kURL1)));
  EXPECT_FALSE(RecordedBrokerEvent(GURL(kURL2)));

  ResetUKM();

  // Iframe usage should be recorded under the top level frame origin.
  RenderFrameHost* child_frame =
      RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  child_frame = NavigationSimulator::NavigateAndCommitFromDocument(GURL(kURL2),
                                                                   child_frame);
  EXPECT_EQ(GURL(kURL2), child_frame->GetLastCommittedURL());
  render_process_id = child_frame->GetProcess()->GetID();
  render_frame_id = child_frame->GetRoutingID();
  service->OpenChannelToPpapiBroker(render_process_id, render_frame_id,
                                    base::FilePath(), &client);
  EXPECT_TRUE(RecordedBrokerEvent(GURL(kURL1)));
  EXPECT_FALSE(RecordedBrokerEvent(GURL(kURL2)));
}

}  // namespace content
