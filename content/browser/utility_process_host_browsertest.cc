// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/browser/utility_process_host.h"
#include "content/browser/utility_process_host_client.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/bind_interface_helpers.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_service.mojom.h"

namespace content {

class UtilityProcessHostBrowserTest : public ContentBrowserTest {
 public:
  void RunUtilityProcess(bool elevated) {
    base::RunLoop run_loop;
    done_closure_ = run_loop.QuitClosure();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &UtilityProcessHostBrowserTest::RunUtilityProcessOnIOThread,
            base::Unretained(this), elevated));
    run_loop.Run();
  }

 protected:
  void RunUtilityProcessOnIOThread(bool elevated) {
    UtilityProcessHost* host =
        new UtilityProcessHost(/*client=*/nullptr,
                               /*client_task_runner=*/nullptr);
    host->SetName(base::ASCIIToUTF16("TestProcess"));
#if defined(OS_WIN)
    if (elevated)
      host->SetSandboxType(service_manager::SandboxType::
                               SANDBOX_TYPE_NO_SANDBOX_AND_ELEVATED_PRIVILEGES);
#endif
    EXPECT_TRUE(host->Start());

    BindInterface(host, &service_);
    service_->DoSomething(base::BindOnce(
        &UtilityProcessHostBrowserTest::OnSomething, base::Unretained(this)));
  }

  void OnSomething() {
    service_.reset();
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, done_closure_);
  }

  mojom::TestServicePtr service_;
  base::Closure done_closure_;
};

IN_PROC_BROWSER_TEST_F(UtilityProcessHostBrowserTest, LaunchProcess) {
  RunUtilityProcess(false);
}

#if defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(UtilityProcessHostBrowserTest, LaunchElevatedProcess) {
  RunUtilityProcess(true);
}
#endif

}  // namespace content
