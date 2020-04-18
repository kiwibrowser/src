// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_api_frame_id_map.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/common/constants.h"
#include "ipc/ipc_message.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

using FrameDataCallback = extensions::ExtensionApiFrameIdMap::FrameDataCallback;

int ToTestFrameId(int render_process_id, int frame_routing_id) {
  if (render_process_id < 0 && frame_routing_id < 0)
    return ExtensionApiFrameIdMap::kInvalidFrameId;
  // Return a deterministic value (yet different from the input) for testing.
  // To make debugging easier: Ending with 0 = frame ID.
  return render_process_id * 1000 + frame_routing_id * 10;
}

int ToTestParentFrameId(int render_process_id, int frame_routing_id) {
  if (render_process_id < 0 && frame_routing_id < 0)
    return ExtensionApiFrameIdMap::kInvalidFrameId;
  // Return a deterministic value (yet different from the input) for testing.
  // To make debugging easier: Ending with 7 = parent frame ID.
  return render_process_id * 1000 + frame_routing_id * 10 + 7;
}

int ToTestTabId(int render_process_id, int frame_routing_id) {
  if (render_process_id < 0 && frame_routing_id < 0)
    return extension_misc::kUnknownTabId;
  // Return a deterministic value (yet different from the input) for testing.
  // To make debugging easier: Ending with 5 = tab ID.
  return render_process_id * 1000 + frame_routing_id * 10 + 5;
}

int ToTestWindowId(int render_process_id, int frame_routing_id) {
  if (render_process_id < 0 && frame_routing_id < 0)
    return extension_misc::kUnknownWindowId;
  // Return a deterministic value (yet different from the input) for testing.
  // To make debugging easier: Ending with 4 = window ID.
  return render_process_id * 1000 + frame_routing_id * 10 + 4;
}

GURL ToTestLastCommittedMainFrameURL(int render_process_id,
                                     int frame_routing_id) {
  if (render_process_id < 0 && frame_routing_id < 0)
    return GURL();

  // Return a deterministic value (yet different from the input) for testing.
  return GURL(base::StringPrintf("http://%d.com/%d", render_process_id,
                                 frame_routing_id));
}

class TestExtensionApiFrameIdMap : public ExtensionApiFrameIdMap {
 public:
  TestExtensionApiFrameIdMap() {}
  ~TestExtensionApiFrameIdMap() override {}

  int GetInternalSize() { return frame_data_map_.size(); }
  int GetInternalCallbackCount() {
    int count = 0;
    for (auto& it : callbacks_map_)
      count += it.second.callbacks.size();
    return count;
  }

  // These indirections are used because we cannot mock RenderFrameHost with
  // fixed IDs in unit tests.
  // TODO(robwu): Use content/public/test/test_renderer_host.h to mock
  // RenderFrameHosts and update the tests to test against these mocks.
  // After doing that, there is no need for CacheFrameData/RemoveFrameData
  // methods that take a RenderFrameIdKey, so the methods can be merged.
  void SetInternalFrameData(int render_process_id, int frame_routing_id) {
    CacheFrameData(RenderFrameIdKey(render_process_id, frame_routing_id));
  }
  void RemoveInternalFrameData(int render_process_id, int frame_routing_id) {
    RemoveFrameData(RenderFrameIdKey(render_process_id, frame_routing_id));
  }

 private:
  // ExtensionApiFrameIdMap:
  FrameData KeyToValue(const RenderFrameIdKey& key) const override {
    return FrameData(
        ToTestFrameId(key.render_process_id, key.frame_routing_id),
        ToTestParentFrameId(key.render_process_id, key.frame_routing_id),
        ToTestTabId(key.render_process_id, key.frame_routing_id),
        ToTestWindowId(key.render_process_id, key.frame_routing_id),
        ToTestLastCommittedMainFrameURL(key.render_process_id,
                                        key.frame_routing_id));
  }
};

class ExtensionApiFrameIdMapTest : public testing::Test {
 public:
  ExtensionApiFrameIdMapTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

  FrameDataCallback CreateCallback(
      int render_process_id,
      int frame_routing_id,
      const std::string& callback_name_for_testing) {
    return base::Bind(&ExtensionApiFrameIdMapTest::OnCalledCallback,
                      base::Unretained(this), render_process_id,
                      frame_routing_id, callback_name_for_testing);
  }

  void OnCalledCallback(int render_process_id,
                        int frame_routing_id,
                        const std::string& callback_name_for_testing,
                        const ExtensionApiFrameIdMap::FrameData& frame_data) {
    results_.push_back(callback_name_for_testing);

    // If this fails, then the mapping is completely wrong.
    EXPECT_EQ(ToTestFrameId(render_process_id, frame_routing_id),
              frame_data.frame_id);
    EXPECT_EQ(ToTestParentFrameId(render_process_id, frame_routing_id),
              frame_data.parent_frame_id);
    EXPECT_EQ(ToTestTabId(render_process_id, frame_routing_id),
              frame_data.tab_id);
    EXPECT_EQ(ToTestWindowId(render_process_id, frame_routing_id),
              frame_data.window_id);
    EXPECT_EQ(
        ToTestLastCommittedMainFrameURL(render_process_id, frame_routing_id),
        frame_data.last_committed_main_frame_url);
  }

  const std::vector<std::string>& results() { return results_; }
  void ClearResults() { results_.clear(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  // Used to verify the order of callbacks.
  std::vector<std::string> results_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionApiFrameIdMapTest);
};

}  // namespace

TEST_F(ExtensionApiFrameIdMapTest, GetFrameDataOnIO) {
  TestExtensionApiFrameIdMap map;
  EXPECT_EQ(0, map.GetInternalSize());

  // Two identical calls, should be processed at the next message loop.
  map.GetFrameDataOnIO(1, 2, CreateCallback(1, 2, "first"));
  EXPECT_EQ(1, map.GetInternalCallbackCount());
  EXPECT_EQ(0, map.GetInternalSize());

  map.GetFrameDataOnIO(1, 2, CreateCallback(1, 2, "first again"));
  EXPECT_EQ(2, map.GetInternalCallbackCount());
  EXPECT_EQ(0, map.GetInternalSize());

  // First get the frame ID on IO (queued on message loop), then set it on UI.
  // No callbacks should be invoked because the IO thread cannot know that the
  // frame ID was set on the UI thread.
  map.GetFrameDataOnIO(2, 1, CreateCallback(2, 1, "something else"));
  EXPECT_EQ(3, map.GetInternalCallbackCount());
  EXPECT_EQ(0, map.GetInternalSize());

  map.SetInternalFrameData(2, 1);
  EXPECT_EQ(1, map.GetInternalSize());
  EXPECT_EQ(0U, results().size());

  // Run some self-contained test. They should not affect the above callbacks.
  {
    // Callbacks for invalid IDs should immediately be run because it doesn't
    // require a thread hop to determine their invalidity.
    map.GetFrameDataOnIO(-1, MSG_ROUTING_NONE,
                         CreateCallback(-1, MSG_ROUTING_NONE, "invalid IDs"));
    EXPECT_EQ(3, map.GetInternalCallbackCount());  // No change.
    EXPECT_EQ(1, map.GetInternalSize());           // No change.
    ASSERT_EQ(1U, results().size());               // +1
    EXPECT_EQ("invalid IDs", results()[0]);
    ClearResults();
  }

  {
    // First set the frame ID on UI, then get it on IO. Callback should
    // immediately be invoked.
    map.SetInternalFrameData(3, 1);
    EXPECT_EQ(2, map.GetInternalSize());

    map.GetFrameDataOnIO(3, 1, CreateCallback(3, 1, "the only result"));
    EXPECT_EQ(3, map.GetInternalCallbackCount());  // No change.
    EXPECT_EQ(2, map.GetInternalSize());           // +1
    ASSERT_EQ(1U, results().size());               // +1
    EXPECT_EQ("the only result", results()[0]);
    ClearResults();
  }

  {
    // Request the frame ID on IO, set the frame ID (in reality, set on the UI),
    // and request another frame ID. The last query should cause both callbacks
    // to run because the frame ID is known at the time of the call.
    map.GetFrameDataOnIO(7, 2, CreateCallback(7, 2, "queued"));
    EXPECT_EQ(4, map.GetInternalCallbackCount());  // +1

    map.SetInternalFrameData(7, 2);
    EXPECT_EQ(3, map.GetInternalSize());           // +1

    map.GetFrameDataOnIO(7, 2, CreateCallback(7, 2, "not queued"));
    EXPECT_EQ(3, map.GetInternalCallbackCount());  // -1 (first callback ran).
    EXPECT_EQ(3, map.GetInternalSize());           // No change.
    ASSERT_EQ(2U, results().size());               // +2 (both callbacks ran).
    EXPECT_EQ("queued", results()[0]);
    EXPECT_EQ("not queued", results()[1]);
    ClearResults();
  }

  // A call identical to the very first call.
  map.GetFrameDataOnIO(1, 2, CreateCallback(1, 2, "same as first"));
  EXPECT_EQ(4, map.GetInternalCallbackCount());
  EXPECT_EQ(3, map.GetInternalSize());

  // Trigger the queued callbacks.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, map.GetInternalCallbackCount());   // -4 (no queued callbacks).

  EXPECT_EQ(4, map.GetInternalSize());            // +1 (1 new cached frame ID).
  ASSERT_EQ(4U, results().size());                // +4 (callbacks ran).

  // PostTasks are processed in order, so the very first callbacks should be
  // processed. As soon as the first callback is available, all of its callbacks
  // should be run (no deferrals!).
  EXPECT_EQ("first", results()[0]);
  EXPECT_EQ("first again", results()[1]);
  EXPECT_EQ("same as first", results()[2]);
  // This was queued after "first again", but has a different frame ID, so it
  // is received after "same as first".
  EXPECT_EQ("something else", results()[3]);
  ClearResults();

  // Request the frame ID for input that was already looked up. Should complete
  // synchronously.
  map.GetFrameDataOnIO(1, 2, CreateCallback(1, 2, "first and cached"));
  EXPECT_EQ(0, map.GetInternalCallbackCount());  // No change.
  EXPECT_EQ(4, map.GetInternalSize());           // No change.
  ASSERT_EQ(1U, results().size());               // +1 (synchronous callback).
  EXPECT_EQ("first and cached", results()[0]);
  ClearResults();

  // Trigger frame removal and look up frame ID. The frame ID should no longer
  // be available. and GetFrameDataOnIO() should require a thread hop.
  map.RemoveInternalFrameData(1, 2);
  EXPECT_EQ(3, map.GetInternalSize());           // -1
  map.GetFrameDataOnIO(1, 2, CreateCallback(1, 2, "first was removed"));
  EXPECT_EQ(1, map.GetInternalCallbackCount());  // +1
  ASSERT_EQ(0U, results().size());               // No change (queued callback).
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, map.GetInternalCallbackCount());  // -1 (callback not in queue).
  EXPECT_EQ(4, map.GetInternalSize());           // +1 (cached frame ID).
  ASSERT_EQ(1U, results().size());               // +1 (callback ran).
  EXPECT_EQ("first was removed", results()[0]);
}

TEST_F(ExtensionApiFrameIdMapTest, GetCachedFrameDataOnIO) {
  TestExtensionApiFrameIdMap map;
  EXPECT_EQ(0, map.GetInternalSize());

  const int kRenderProcessId = 1;
  const int kFrameRoutingId = 2;

  // Getting cached data on the IO thread should fail if there is no cached
  // data for the entry...
  ExtensionApiFrameIdMap::FrameData data;
  EXPECT_FALSE(
      map.GetCachedFrameDataOnIO(kRenderProcessId, kFrameRoutingId, &data));
  // ... should not queue any callbacks...
  EXPECT_EQ(0, map.GetInternalCallbackCount());
  base::RunLoop().RunUntilIdle();
  // ... and should not add any entries to the map.
  EXPECT_EQ(0, map.GetInternalSize());

  // Getting cached data should succeed if there is a cached entry.
  map.SetInternalFrameData(kRenderProcessId, kFrameRoutingId);
  EXPECT_EQ(1, map.GetInternalSize());
  EXPECT_TRUE(
      map.GetCachedFrameDataOnIO(kRenderProcessId, kFrameRoutingId, &data));
  EXPECT_EQ(0, map.GetInternalCallbackCount());
  EXPECT_EQ(ToTestFrameId(kRenderProcessId, kFrameRoutingId), data.frame_id);
  EXPECT_EQ(ToTestParentFrameId(kRenderProcessId, kFrameRoutingId),
            data.parent_frame_id);
  EXPECT_EQ(ToTestTabId(kRenderProcessId, kFrameRoutingId), data.tab_id);
  EXPECT_EQ(ToTestWindowId(kRenderProcessId, kFrameRoutingId), data.window_id);
  EXPECT_EQ(ToTestLastCommittedMainFrameURL(kRenderProcessId, kFrameRoutingId),
            data.last_committed_main_frame_url);
}

}  // namespace extensions
