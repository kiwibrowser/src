// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/io_thread.h"

#include <map>
#include <memory>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/certificate_transparency/features.h"
#include "components/certificate_transparency/tree_state_tracker.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/filename_util.h"
#include "net/base/host_port_pair.h"
#include "net/cert/ct_verifier.h"
#include "net/http/http_auth_preferences.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/simple_connection_listener.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

// URLFetcherDelegate that expects a request to hang.
class HangingURLFetcherDelegate : public net::URLFetcherDelegate {
 public:
  HangingURLFetcherDelegate() {}
  ~HangingURLFetcherDelegate() override {}

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    ADD_FAILURE() << "This request should never complete.";
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HangingURLFetcherDelegate);
};

// URLFetcherDelegate that can wait for a request to succeed.
class TestURLFetcherDelegate : public net::URLFetcherDelegate {
 public:
  TestURLFetcherDelegate() {}
  ~TestURLFetcherDelegate() override {}

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    run_loop_.Quit();
  }

  void WaitForCompletion() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TestURLFetcherDelegate);
};

// Runs a task on the IOThread and waits for it to complete.
void RunOnIOThreadBlocking(base::OnceClosure task) {
  base::RunLoop run_loop;
  content::BrowserThread::PostTaskAndReply(content::BrowserThread::IO,
                                           FROM_HERE, std::move(task),
                                           run_loop.QuitClosure());
  run_loop.Run();
}

void CheckCnameLookup(IOThread* io_thread, bool expected) {
  EXPECT_EQ(expected,
            io_thread->globals()
                ->http_auth_preferences->NegotiateDisableCnameLookup());
}

void CheckNegotiateEnablePort(IOThread* io_thread, bool expected) {
  EXPECT_EQ(expected,
            io_thread->globals()->http_auth_preferences->NegotiateEnablePort());
}

void CheckCanUseDefaultCredentials(IOThread* io_thread,
                                   bool expected,
                                   const GURL& url) {
  EXPECT_EQ(
      expected,
      io_thread->globals()->http_auth_preferences->CanUseDefaultCredentials(
          url));
}

void CheckCanDelegate(IOThread* io_thread, bool expected, const GURL& url) {
  EXPECT_EQ(expected,
            io_thread->globals()->http_auth_preferences->CanDelegate(url));
}

void CheckEffectiveConnectionType(IOThread* io_thread,
                                  net::EffectiveConnectionType expected) {
  EXPECT_EQ(expected, io_thread->globals()
                          ->system_request_context->network_quality_estimator()
                          ->GetEffectiveConnectionType());
}

class IOThreadBrowserTest : public InProcessBrowserTest {
 public:
  IOThreadBrowserTest() {}
  ~IOThreadBrowserTest() override {}

  void SetUp() override {
    // Must start listening (And get a port for the proxy) before calling
    // SetUp(). Use two phase EmbeddedTestServer setup for proxy tests.
    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    embedded_test_server()->StartAcceptingConnections();
  }

  void TearDown() override {
    // Need to stop this before |connection_listener_| is destroyed.
    EXPECT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
    InProcessBrowserTest::TearDown();
  }
};

// This test uses the kDisableAuthNegotiateCnameLookup to check that
// the HttpAuthPreferences are correctly initialized and running on the
// IO thread. The other preferences are tested by the HttpAuthPreferences
// unit tests.
IN_PROC_BROWSER_TEST_F(IOThreadBrowserTest, UpdateNegotiateDisableCnameLookup) {
  g_browser_process->local_state()->SetBoolean(
      prefs::kDisableAuthNegotiateCnameLookup, false);
  RunOnIOThreadBlocking(
      base::Bind(&CheckCnameLookup,
                 base::Unretained(g_browser_process->io_thread()), false));
  g_browser_process->local_state()->SetBoolean(
      prefs::kDisableAuthNegotiateCnameLookup, true);
  RunOnIOThreadBlocking(
      base::Bind(&CheckCnameLookup,
                 base::Unretained(g_browser_process->io_thread()), true));
}

IN_PROC_BROWSER_TEST_F(IOThreadBrowserTest, UpdateEnableAuthNegotiatePort) {
  g_browser_process->local_state()->SetBoolean(prefs::kEnableAuthNegotiatePort,
                                               false);
  RunOnIOThreadBlocking(
      base::Bind(&CheckNegotiateEnablePort,
                 base::Unretained(g_browser_process->io_thread()), false));
  g_browser_process->local_state()->SetBoolean(prefs::kEnableAuthNegotiatePort,
                                               true);
  RunOnIOThreadBlocking(
      base::Bind(&CheckNegotiateEnablePort,
                 base::Unretained(g_browser_process->io_thread()), true));
}

IN_PROC_BROWSER_TEST_F(IOThreadBrowserTest, UpdateServerWhitelist) {
  GURL url("http://test.example.com");

  g_browser_process->local_state()->SetString(prefs::kAuthServerWhitelist,
                                              "xxx");
  RunOnIOThreadBlocking(
      base::Bind(&CheckCanUseDefaultCredentials,
                 base::Unretained(g_browser_process->io_thread()), false, url));

  g_browser_process->local_state()->SetString(prefs::kAuthServerWhitelist, "*");
  RunOnIOThreadBlocking(
      base::Bind(&CheckCanUseDefaultCredentials,
                 base::Unretained(g_browser_process->io_thread()), true, url));
}

IN_PROC_BROWSER_TEST_F(IOThreadBrowserTest, UpdateDelegateWhitelist) {
  GURL url("http://test.example.com");

  g_browser_process->local_state()->SetString(
      prefs::kAuthNegotiateDelegateWhitelist, "");
  RunOnIOThreadBlocking(
      base::Bind(&CheckCanDelegate,
                 base::Unretained(g_browser_process->io_thread()), false, url));

  g_browser_process->local_state()->SetString(
      prefs::kAuthNegotiateDelegateWhitelist, "*");
  RunOnIOThreadBlocking(
      base::Bind(&CheckCanDelegate,
                 base::Unretained(g_browser_process->io_thread()), true, url));
}

class IOThreadEctCommandLineBrowserTest : public IOThreadBrowserTest {
 public:
  IOThreadEctCommandLineBrowserTest() {}
  ~IOThreadEctCommandLineBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII("--force-effective-connection-type",
                                    "Slow-2G");
  }
};

IN_PROC_BROWSER_TEST_F(IOThreadEctCommandLineBrowserTest,
                       ForceECTFromCommandLine) {
  RunOnIOThreadBlocking(
      base::Bind(&CheckEffectiveConnectionType,
                 base::Unretained(g_browser_process->io_thread()),
                 net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G));
}

class IOThreadEctFieldTrialBrowserTest : public IOThreadBrowserTest {
 public:
  IOThreadEctFieldTrialBrowserTest() {}
  ~IOThreadEctFieldTrialBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    variations::testing::ClearAllVariationParams();
    std::map<std::string, std::string> variation_params;
    variation_params["force_effective_connection_type"] = "2G";
    ASSERT_TRUE(variations::AssociateVariationParams(
        "NetworkQualityEstimator", "Enabled", variation_params));
    ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
        "NetworkQualityEstimator", "Enabled"));
  }
};

IN_PROC_BROWSER_TEST_F(IOThreadEctFieldTrialBrowserTest,
                       ForceECTUsingFieldTrial) {
  RunOnIOThreadBlocking(
      base::Bind(&CheckEffectiveConnectionType,
                 base::Unretained(g_browser_process->io_thread()),
                 net::EFFECTIVE_CONNECTION_TYPE_2G));
}

class IOThreadEctFieldTrialAndCommandLineBrowserTest
    : public IOThreadEctFieldTrialBrowserTest {
 public:
  IOThreadEctFieldTrialAndCommandLineBrowserTest() {}
  ~IOThreadEctFieldTrialAndCommandLineBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IOThreadEctFieldTrialBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII("--force-effective-connection-type",
                                    "Slow-2G");
  }
};

IN_PROC_BROWSER_TEST_F(IOThreadEctFieldTrialAndCommandLineBrowserTest,
                       ECTFromCommandLineOverridesFieldTrial) {
  RunOnIOThreadBlocking(
      base::Bind(&CheckEffectiveConnectionType,
                 base::Unretained(g_browser_process->io_thread()),
                 net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G));
}

class IOThreadBrowserTestWithHangingPacRequest : public IOThreadBrowserTest {
 public:
  IOThreadBrowserTestWithHangingPacRequest() {}
  ~IOThreadBrowserTestWithHangingPacRequest() override {}

  void SetUpOnMainThread() override {
    // This must be created after the main message loop has been set up.
    // Waits for one connection.  Additional connections are fine.
    connection_listener_ =
        std::make_unique<net::test_server::SimpleConnectionListener>(
            1, net::test_server::SimpleConnectionListener::
                   ALLOW_ADDITIONAL_CONNECTIONS);

    embedded_test_server()->SetConnectionListener(connection_listener_.get());

    IOThreadBrowserTest::SetUpOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kProxyPacUrl, embedded_test_server()->GetURL("/hung").spec());
  }

 protected:
  std::unique_ptr<net::test_server::SimpleConnectionListener>
      connection_listener_;
};

// Make sure that the SystemURLRequestContext is shut down correctly when
// there's an in-progress PAC script fetch.
IN_PROC_BROWSER_TEST_F(IOThreadBrowserTestWithHangingPacRequest, Shutdown) {
  // Request that should hang while trying to request the PAC script.
  // Enough requests are created on startup that this probably isn't needed, but
  // best to be safe.
  HangingURLFetcherDelegate hanging_fetcher_delegate;
  std::unique_ptr<net::URLFetcher> hanging_fetcher = net::URLFetcher::Create(
      GURL("http://blah/"), net::URLFetcher::GET, &hanging_fetcher_delegate);
  hanging_fetcher->SetRequestContext(
      g_browser_process->io_thread()->system_url_request_context_getter());
  hanging_fetcher->Start();

  connection_listener_->WaitForConnections();
}

}  // namespace
