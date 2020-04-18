// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_usage/tab_id_annotator.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/data_usage/core/data_use.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/previews_state.h"
#include "net/base/network_change_notifier.h"
#include "net/base/request_priority.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::BrowserThread;
using data_usage::DataUse;

namespace chrome_browser_data_usage {

namespace {

class TabIdAnnotatorTest : public ChromeRenderViewHostTestHarness {
 public:
  TabIdAnnotatorTest()
      : ChromeRenderViewHostTestHarness(
            content::TestBrowserThreadBundle::REAL_IO_THREAD) {}

  ~TabIdAnnotatorTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TabIdAnnotatorTest);
};

// Synthesizes a DataUse object with the given |tab_id|.
std::unique_ptr<DataUse> CreateDataUse(SessionID tab_id,
                                       int render_process_id) {
  auto data_use = std::unique_ptr<DataUse>(new DataUse(
      GURL("http://foo.com"), base::TimeTicks(), GURL(), tab_id,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, std::string(), 100, 100));
  return data_use;
}

// Expects that |expected| and |actual| are equal.
void ExpectDataUse(std::unique_ptr<DataUse> expected,
                   std::unique_ptr<DataUse> actual) {
  // Single out the |tab_id| for better debug output in failure cases.
  EXPECT_EQ(expected->tab_id, actual->tab_id);
  EXPECT_EQ(*expected, *actual);
}

// Expects that |expected| and |actual| are equal, then quits |ui_run_loop| on
// the UI thread.
void ExpectDataUseAndQuit(base::RunLoop* ui_run_loop,
                          std::unique_ptr<DataUse> expected,
                          std::unique_ptr<DataUse> actual) {
  DCHECK(ui_run_loop);
  ExpectDataUse(std::move(expected), std::move(actual));

  // This can't use run_loop->QuitClosure here because that uses WeakPtrs, which
  // aren't thread safe.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&base::RunLoop::Quit, base::Unretained(ui_run_loop)));
}

// Tests that for a sample URLRequest, associated with the given
// |render_process_id| and |render_frame_id|, repeatedly annotating DataUse for
// that URLRequest yields the |expected_tab_id|. |ui_run_loop| is the RunLoop on
// the UI thread that should be quit after all the annotations are done.
// Passing in -1 for either or both of |render_process_id| or |render_frame_id|
// indicates that the URLRequest should have no associated ResourceRequestInfo.
void TestAnnotateOnIOThread(base::RunLoop* ui_run_loop,
                            int render_process_id,
                            int render_frame_id,
                            SessionID expected_tab_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(ui_run_loop);

  TabIdAnnotator annotator;
  net::TestURLRequestContext context;
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context.CreateRequest(GURL("http://foo.com"), net::IDLE, &test_delegate,
                            TRAFFIC_ANNOTATION_FOR_TESTS);

  if (render_process_id != -1 && render_frame_id != -1) {
    // The only args that matter here for the ResourceRequestInfo are the
    // |request|, the |render_process_id|, and the |render_frame_id|; arbitrary
    // values are used for all the other args.
    content::ResourceRequestInfo::AllocateForTesting(
        request.get(), content::RESOURCE_TYPE_MAIN_FRAME, nullptr,
        render_process_id, -1, render_frame_id, true, true, true,
        content::PREVIEWS_OFF, nullptr);
  }

  // An invalid tab ID to check that the annotator always sets the tab ID. An
  // arbitrary number is used because SessionID::InvalidValue() means "no tab
  // was found".
  const SessionID kInvalidTabId = SessionID::FromSerializedValue(123456);

  // Annotate two separate DataUse objects to ensure that repeated annotations
  // for the same URLRequest work properly.
  std::unique_ptr<DataUse> first_expected_data_use =
      CreateDataUse(expected_tab_id, render_process_id);
  annotator.Annotate(
      request.get(), CreateDataUse(kInvalidTabId, render_process_id),
      base::Bind(&ExpectDataUse, base::Passed(&first_expected_data_use)));

  // Quit the |ui_run_loop| after the second annotation.
  std::unique_ptr<DataUse> second_expected_data_use =
      CreateDataUse(expected_tab_id, render_process_id);
  annotator.Annotate(request.get(),
                     CreateDataUse(kInvalidTabId, render_process_id),
                     base::Bind(&ExpectDataUseAndQuit, ui_run_loop,
                                base::Passed(&second_expected_data_use)));
}

TEST_F(TabIdAnnotatorTest, AnnotateWithNoRenderFrame) {
  base::RunLoop ui_run_loop;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&TestAnnotateOnIOThread, &ui_run_loop,
                     -1 /* render_process_id */, -1 /* render_frame_id */,
                     SessionID::InvalidValue() /* expected_tab_id */));
  ui_run_loop.Run();
}

TEST_F(TabIdAnnotatorTest, AnnotateWithRenderFrameAndNoTab) {
  base::RunLoop ui_run_loop;
  // |web_contents()| isn't a tab, so it shouldn't have a tab ID.
  EXPECT_FALSE(SessionTabHelper::IdForTab(web_contents()).is_valid());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&TestAnnotateOnIOThread, &ui_run_loop,
                     web_contents()->GetMainFrame()->GetProcess()->GetID(),
                     web_contents()->GetMainFrame()->GetRoutingID(),
                     SessionID::InvalidValue() /* expected_tab_id */));
  ui_run_loop.Run();
}

TEST_F(TabIdAnnotatorTest, AnnotateWithRenderFrameAndTab) {
  base::RunLoop ui_run_loop;
  // Make |web_contents()| into a tab.
  SessionTabHelper::CreateForWebContents(web_contents());
  SessionID expected_tab_id = SessionTabHelper::IdForTab(web_contents());
  // |web_contents()| is a tab, so it should have a tab ID.
  EXPECT_TRUE(expected_tab_id.is_valid());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&TestAnnotateOnIOThread, &ui_run_loop,
                     web_contents()->GetMainFrame()->GetProcess()->GetID(),
                     web_contents()->GetMainFrame()->GetRoutingID(),
                     expected_tab_id));
  ui_run_loop.Run();
}

}  // namespace

}  // namespace chrome_browser_data_usage
