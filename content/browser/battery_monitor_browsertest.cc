// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/mojom/battery_monitor.mojom.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace content {

namespace {

class MockBatteryMonitor : public device::mojom::BatteryMonitor {
 public:
  MockBatteryMonitor() : binding_(this) {}
  ~MockBatteryMonitor() override = default;

  void Bind(const std::string& interface_name,
            mojo::ScopedMessagePipeHandle handle,
            const service_manager::BindSourceInfo& source_info) {
    DCHECK(!binding_.is_bound());
    binding_.Bind(device::mojom::BatteryMonitorRequest(std::move(handle)));
  }

  void DidChange(const device::mojom::BatteryStatus& battery_status) {
    status_ = battery_status;
    status_to_report_ = true;

    if (!callback_.is_null())
      ReportStatus();
  }

 private:
  // mojom::BatteryMonitor methods:
  void QueryNextStatus(QueryNextStatusCallback callback) override {
    if (!callback_.is_null()) {
      DVLOG(1) << "Overlapped call to QueryNextStatus!";
      binding_.Close();
      return;
    }
    callback_ = std::move(callback);

    if (status_to_report_)
      ReportStatus();
  }

  void ReportStatus() {
    std::move(callback_).Run(status_.Clone());
    status_to_report_ = false;
  }

  QueryNextStatusCallback callback_;
  device::mojom::BatteryStatus status_;
  bool status_to_report_ = false;
  mojo::Binding<device::mojom::BatteryMonitor> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockBatteryMonitor);
};

class BatteryMonitorTest : public ContentBrowserTest {
 public:
  BatteryMonitorTest() = default;

  void SetUpOnMainThread() override {
    mock_battery_monitor_ = std::make_unique<MockBatteryMonitor>();
    // Because Device Service also runs in this process(browser process), here
    // we can directly set our binder to intercept interface requests against
    // it.
    service_manager::ServiceContext::SetGlobalBinderForTesting(
        device::mojom::kServiceName, device::mojom::BatteryMonitor::Name_,
        base::Bind(&MockBatteryMonitor::Bind,
                   base::Unretained(mock_battery_monitor_.get())));
  }

 protected:
  MockBatteryMonitor* mock_battery_monitor() {
    return mock_battery_monitor_.get();
  }

 private:
  std::unique_ptr<MockBatteryMonitor> mock_battery_monitor_;

  DISALLOW_COPY_AND_ASSIGN(BatteryMonitorTest);
};

IN_PROC_BROWSER_TEST_F(BatteryMonitorTest, NavigatorGetBatteryInfo) {
  // From JavaScript request a promise for the battery status information and
  // once it resolves check the values and navigate to #pass.
  device::mojom::BatteryStatus status;
  status.charging = true;
  status.charging_time = 100;
  status.discharging_time = std::numeric_limits<double>::infinity();
  status.level = 0.5;
  mock_battery_monitor()->DidChange(status);

  GURL test_url = GetTestUrl("battery_monitor",
                             "battery_status_promise_resolution_test.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 2);
  EXPECT_EQ("pass", shell()->web_contents()->GetLastCommittedURL().ref());
}

IN_PROC_BROWSER_TEST_F(BatteryMonitorTest, NavigatorGetBatteryListenChange) {
  // From JavaScript request a promise for the battery status information.
  // Once it resolves add an event listener for battery level change. Set
  // battery level to 0.6 and invoke update. Check that the event listener
  // is invoked with the correct value for level and navigate to #pass.
  device::mojom::BatteryStatus status;
  mock_battery_monitor()->DidChange(status);

  TestNavigationObserver same_tab_observer(shell()->web_contents(), 2);
  GURL test_url =
      GetTestUrl("battery_monitor", "battery_status_event_listener_test.html");
  shell()->LoadURL(test_url);
  same_tab_observer.Wait();
  EXPECT_EQ("resolved", shell()->web_contents()->GetLastCommittedURL().ref());

  TestNavigationObserver same_tab_observer2(shell()->web_contents(), 1);
  status.level = 0.6;
  mock_battery_monitor()->DidChange(status);
  same_tab_observer2.Wait();
  EXPECT_EQ("pass", shell()->web_contents()->GetLastCommittedURL().ref());
}

}  //  namespace

}  //  namespace content
