// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_usage/tab_id_provider.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chrome_browser_data_usage {

namespace {

const SessionID kTabId = SessionID::FromSerializedValue(10);

class TabIdProviderTest : public testing::Test {
 public:
  TabIdProviderTest()
      : task_runner_(base::ThreadTaskRunnerHandle::Get()),
        tab_id_getter_call_count_(0) {}

  ~TabIdProviderTest() override {}

  base::OnceCallback<SessionID(void)> TabIdGetterCallback() {
    return base::BindOnce(&TabIdProviderTest::GetTabInfo,
                          base::Unretained(this));
  }

  base::TaskRunner* task_runner() { return task_runner_.get(); }
  base::RunLoop* run_loop() { return &run_loop_; }
  int tab_id_getter_call_count() const { return tab_id_getter_call_count_; }

 private:
  SessionID GetTabInfo() {
    ++tab_id_getter_call_count_;
    return kTabId;
  }

  base::MessageLoop message_loop_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::RunLoop run_loop_;
  int tab_id_getter_call_count_;

  DISALLOW_COPY_AND_ASSIGN(TabIdProviderTest);
};

// Copies |tab_id| into |capture|.
void CaptureTabId(SessionID* capture, SessionID tab_info) {
  *capture = tab_info;
}

TEST_F(TabIdProviderTest, ProvideTabId) {
  TabIdProvider provider(task_runner(), FROM_HERE, TabIdGetterCallback());

  SessionID tab_id = SessionID::InvalidValue();
  provider.ProvideTabId(base::BindOnce(&CaptureTabId, &tab_id));
  run_loop()->RunUntilIdle();

  EXPECT_EQ(1, tab_id_getter_call_count());
  EXPECT_EQ(kTabId, tab_id);
}

TEST_F(TabIdProviderTest, ProvideTabIdPiggyback) {
  TabIdProvider provider(task_runner(), FROM_HERE, TabIdGetterCallback());

  // First, ask for the first tab ID to kick things off.
  SessionID first_tab_id = SessionID::InvalidValue();
  provider.ProvideTabId(base::BindOnce(&CaptureTabId, &first_tab_id));

  // The first tab ID callback should still be pending, with the tab ID not
  // available yet, so this second callback should piggyback off of the first
  // callback.
  SessionID piggyback_tab_id = SessionID::InvalidValue();
  provider.ProvideTabId(base::BindOnce(&CaptureTabId, &piggyback_tab_id));

  run_loop()->RunUntilIdle();

  // The tab ID getter callback should only have been called once.
  EXPECT_EQ(1, tab_id_getter_call_count());
  EXPECT_EQ(kTabId, first_tab_id);
  EXPECT_EQ(kTabId, piggyback_tab_id);
}

TEST_F(TabIdProviderTest, ProvideTabIdCacheHit) {
  TabIdProvider provider(task_runner(), FROM_HERE, TabIdGetterCallback());

  // First, ask for the first tab ID to kick things off.
  SessionID first_tab_id = SessionID::InvalidValue();
  provider.ProvideTabId(base::BindOnce(&CaptureTabId, &first_tab_id));

  // Wait for the first tab ID callback to finish.
  run_loop()->RunUntilIdle();

  EXPECT_EQ(1, tab_id_getter_call_count());
  EXPECT_EQ(kTabId, first_tab_id);

  // Ask for another tab ID, which should be satisfied by the cached tab ID from
  // the first callback.
  SessionID cache_hit_tab_id = SessionID::InvalidValue();
  provider.ProvideTabId(base::BindOnce(&CaptureTabId, &cache_hit_tab_id));

  // This cache hit callback should run synchronously, without causing the tab
  // ID getter callback to run again.
  EXPECT_EQ(1, tab_id_getter_call_count());
  EXPECT_EQ(kTabId, cache_hit_tab_id);
}

TEST_F(TabIdProviderTest, ProvideTabIdAfterProviderDestroyed) {
  std::unique_ptr<TabIdProvider> provider(
      new TabIdProvider(task_runner(), FROM_HERE, TabIdGetterCallback()));

  // Ask for two tab IDs.
  SessionID first_tab_id = SessionID::InvalidValue(),
            second_tab_id = SessionID::InvalidValue();
  provider->ProvideTabId(base::BindOnce(&CaptureTabId, &first_tab_id));
  provider->ProvideTabId(base::BindOnce(&CaptureTabId, &second_tab_id));

  // Then, destroy the |provider| before the tab ID getter callback or any other
  // callbacks are run.
  provider.reset();

  // The callbacks should still complete successfully later even though the
  // |provider| no longer exists.
  run_loop()->RunUntilIdle();
  EXPECT_EQ(1, tab_id_getter_call_count());
  EXPECT_EQ(kTabId, first_tab_id);
  EXPECT_EQ(kTabId, second_tab_id);
}

}  // namespace

}  // namespace chrome_browser_data_usage
