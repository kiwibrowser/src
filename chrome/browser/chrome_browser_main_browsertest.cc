// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_browser_main.h"

#include <stddef.h>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/variations_switches.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/base/network_change_notifier_factory.h"

// Friend of ChromeBrowserMainPartsTestApi to poke at internal state.
class ChromeBrowserMainPartsTestApi {
 public:
  explicit ChromeBrowserMainPartsTestApi(ChromeBrowserMainParts* main_parts)
      : main_parts_(main_parts) {}
  ~ChromeBrowserMainPartsTestApi() = default;

  void EnableVariationsServiceInit() {
    main_parts_
        ->should_call_pre_main_loop_start_startup_on_variations_service_ = true;
  }

 private:
  ChromeBrowserMainParts* main_parts_;
  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainPartsTestApi);
};

namespace {

// ChromeBrowserMainExtraParts used to install a MockNetworkChangeNotifier.
class ChromeBrowserMainExtraPartsNetFactoryInstaller
    : public ChromeBrowserMainExtraParts {
 public:
  ChromeBrowserMainExtraPartsNetFactoryInstaller() = default;
  ~ChromeBrowserMainExtraPartsNetFactoryInstaller() override {
    // |network_change_notifier_| needs to be destroyed before |net_installer_|.
    network_change_notifier_.reset();
  }

  net::test::MockNetworkChangeNotifier* network_change_notifier() {
    return network_change_notifier_.get();
  }

  // ChromeBrowserMainExtraParts:
  void PreEarlyInitialization() override {}
  void PostMainMessageLoopStart() override {
    ASSERT_TRUE(net::NetworkChangeNotifier::HasNetworkChangeNotifier());
    net_installer_ =
        std::make_unique<net::NetworkChangeNotifier::DisableForTest>();
    network_change_notifier_ =
        std::make_unique<net::test::MockNetworkChangeNotifier>();
    network_change_notifier_->SetConnectionType(
        net::NetworkChangeNotifier::CONNECTION_NONE);
  }

 private:
  std::unique_ptr<net::test::MockNetworkChangeNotifier>
      network_change_notifier_;
  std::unique_ptr<net::NetworkChangeNotifier::DisableForTest> net_installer_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsNetFactoryInstaller);
};

class ChromeBrowserMainBrowserTest : public InProcessBrowserTest {
 public:
  ChromeBrowserMainBrowserTest() = default;
  ~ChromeBrowserMainBrowserTest() override = default;

 protected:
  // InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Without this (and EnableFetchForTesting() below) VariationsService won't
    // do requests in non-branded builds.
    command_line->AppendSwitchASCII(variations::switches::kVariationsServerURL,
                                    "http://localhost");
  }

  void CreatedBrowserMainParts(
      content::BrowserMainParts* browser_main_parts) override {
    variations::VariationsService::EnableFetchForTesting();
    ChromeBrowserMainParts* chrome_browser_main_parts =
        static_cast<ChromeBrowserMainParts*>(browser_main_parts);
    ChromeBrowserMainPartsTestApi(chrome_browser_main_parts)
        .EnableVariationsServiceInit();
    extra_parts_ = new ChromeBrowserMainExtraPartsNetFactoryInstaller();
    chrome_browser_main_parts->AddParts(extra_parts_);
  }

  ChromeBrowserMainExtraPartsNetFactoryInstaller* extra_parts_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainBrowserTest);
};

// Verifies VariationsService does a request when network status changes from
// none to connected. This is a regression test for https://crbug.com/826930.
IN_PROC_BROWSER_TEST_F(ChromeBrowserMainBrowserTest,
                       VariationsServiceStartsRequestOnNetworkChange) {
  const int initial_request_count =
      g_browser_process->variations_service()->request_count();
  ASSERT_TRUE(extra_parts_);
  extra_parts_->network_change_notifier()->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  // NotifyObserversOfNetworkChangeForTests uses PostTask, so run the loop until
  // idle to ensure VariationsService processes the network change.
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
  const int final_request_count =
      g_browser_process->variations_service()->request_count();
  EXPECT_EQ(initial_request_count + 1, final_request_count);
}

}  // namespace
