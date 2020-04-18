// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/usb/usb_chooser_context_factory.h"
#include "chrome/browser/usb/usb_chooser_controller.h"
#include "chrome/browser/usb/web_usb_chooser_service.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "device/base/mock_device_client.h"
#include "device/usb/mock_usb_device.h"
#include "device/usb/mock_usb_service.h"
#include "device/usb/public/mojom/chooser_service.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"

using content::RenderFrameHost;
using device::MockDeviceClient;
using device::MockUsbDevice;

namespace {

class FakeChooserView : public ChooserController::View {
 public:
  explicit FakeChooserView(std::unique_ptr<ChooserController> controller)
      : controller_(std::move(controller)) {
    controller_->set_view(this);
  }

  ~FakeChooserView() override { controller_->set_view(nullptr); }

  void OnOptionsInitialized() override {
    if (controller_->NumOptions())
      controller_->Select({0});
    else
      controller_->Cancel();
    delete this;
  }

  void OnOptionAdded(size_t index) override { NOTREACHED(); }
  void OnOptionRemoved(size_t index) override { NOTREACHED(); }
  void OnOptionUpdated(size_t index) override { NOTREACHED(); }
  void OnAdapterEnabledChanged(bool enabled) override { NOTREACHED(); }
  void OnRefreshStateChanged(bool refreshing) override { NOTREACHED(); }

 private:
  std::unique_ptr<ChooserController> controller_;

  DISALLOW_COPY_AND_ASSIGN(FakeChooserView);
};

class FakeChooserService : public WebUsbChooserService {
 public:
  explicit FakeChooserService(RenderFrameHost* render_frame_host)
      : WebUsbChooserService(render_frame_host) {}

  ~FakeChooserService() override {}

  void ShowChooser(std::unique_ptr<UsbChooserController> controller) override {
    new FakeChooserView(std::move(controller));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeChooserService);
};

class TestContentBrowserClient : public ChromeContentBrowserClient {
 public:
  TestContentBrowserClient() {}
  ~TestContentBrowserClient() override {}

  // ChromeContentBrowserClient:
  void CreateUsbChooserService(
      content::RenderFrameHost* render_frame_host,
      device::mojom::UsbChooserServiceRequest request) override {
    mojo::MakeStrongBinding(
        std::make_unique<FakeChooserService>(render_frame_host),
        std::move(request));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestContentBrowserClient);
};

class WebUsbTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    embedded_test_server()->ServeFilesFromSourceDirectory("content/test/data");
    ASSERT_TRUE(embedded_test_server()->Start());
    device_client_.reset(new MockDeviceClient());
    AddMockDevice("123456");

    // Force the UsbChooserContext to be created before the test begins. This
    // ensures that it is created before any instances of DeviceManagerImpl and
    // thus may expose ordering bugs not normally encountered.
    UsbChooserContextFactory::GetForProfile(browser()->profile());

    original_content_browser_client_ =
        content::SetBrowserClientForTesting(&test_content_browser_client_);

    GURL url = embedded_test_server()->GetURL("localhost", "/simple_page.html");
    ui_test_utils::NavigateToURL(browser(), url);
    origin_ = url.GetOrigin();

    RenderFrameHost* render_frame_host =
        browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame();
    EXPECT_EQ(origin_, render_frame_host->GetLastCommittedOrigin().GetURL());
  }

  void TearDown() override {
    content::SetBrowserClientForTesting(original_content_browser_client_);
  }

  void AddMockDevice(const std::string& serial_number) {
    DCHECK(!mock_device_);
    mock_device_ = base::MakeRefCounted<MockUsbDevice>(
        0, 0, "Test Manufacturer", "Test Device", serial_number);
    device_client_->usb_service()->AddDevice(mock_device_);
  }

  void RemoveMockDevice() {
    DCHECK(mock_device_);
    device_client_->usb_service()->RemoveDevice(mock_device_);
    mock_device_ = nullptr;
  }

  const GURL& origin() { return origin_; }

 private:
  std::unique_ptr<MockDeviceClient> device_client_;
  scoped_refptr<MockUsbDevice> mock_device_;
  TestContentBrowserClient test_content_browser_client_;
  content::ContentBrowserClient* original_content_browser_client_;
  GURL origin_;
};

IN_PROC_BROWSER_TEST_F(WebUsbTest, RequestAndGetDevices) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  int int_result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractInt(
      web_contents,
      "navigator.usb.getDevices()"
      "    .then(devices => {"
      "        domAutomationController.send(devices.length);"
      "    });",
      &int_result));
  EXPECT_EQ(0, int_result);

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "navigator.usb.requestDevice({ filters: [ { vendorId: 0 } ] })"
      "    .then(device => {"
      "        domAutomationController.send(device.serialNumber);"
      "    });",
      &result));
  EXPECT_EQ("123456", result);

  EXPECT_TRUE(content::ExecuteScriptAndExtractInt(
      web_contents,
      "navigator.usb.getDevices()"
      "    .then(devices => {"
      "        domAutomationController.send(devices.length);"
      "    });",
      &int_result));
  EXPECT_EQ(1, int_result);
}

IN_PROC_BROWSER_TEST_F(WebUsbTest, RequestDeviceWithGuardBlocked) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  auto* map =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  map->SetContentSettingDefaultScope(origin(), origin(),
                                     CONTENT_SETTINGS_TYPE_USB_GUARD,
                                     std::string(), CONTENT_SETTING_BLOCK);

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "navigator.usb.requestDevice({ filters: [ { vendorId: 0 } ] })"
      "    .then(device => {"
      "      domAutomationController.send('failed');"
      "    }, error => {"
      "      domAutomationController.send(error.name + ': ' + error.message);"
      "    });",
      &result));
  EXPECT_EQ("NotFoundError: No device selected.", result);
}

IN_PROC_BROWSER_TEST_F(WebUsbTest, AddRemoveDevice) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "navigator.usb.requestDevice({ filters: [ { vendorId: 0 } ] })"
      "    .then(device => {"
      "        domAutomationController.send(device.serialNumber);"
      "    });"

      "var deviceAdded = null;"
      "navigator.usb.addEventListener('connect', e => {"
      "    deviceAdded = e.device;"
      "});"

      "var deviceRemoved = null;"
      "navigator.usb.addEventListener('disconnect', e => {"
      "    deviceRemoved = e.device;"
      "});",
      &result));
  EXPECT_EQ("123456", result);

  RemoveMockDevice();
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "if (deviceRemoved === null) {"
      "  domAutomationController.send('null');"
      "} else {"
      "  domAutomationController.send(deviceRemoved.serialNumber);"
      "}",
      &result));
  EXPECT_EQ("123456", result);

  AddMockDevice("123456");
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "if (deviceAdded === null) {"
      "  domAutomationController.send('null');"
      "} else {"
      "  domAutomationController.send(deviceAdded.serialNumber);"
      "}",
      &result));
  EXPECT_EQ("123456", result);
}

IN_PROC_BROWSER_TEST_F(WebUsbTest, AddRemoveDeviceEphemeral) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Replace the default mock device with one that has no serial number.
  RemoveMockDevice();
  AddMockDevice("");

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "navigator.usb.requestDevice({ filters: [ { vendorId: 0 } ] })"
      "    .then(device => {"
      "        domAutomationController.send(device.serialNumber);"
      "    });"

      "var deviceRemoved = null;"
      "navigator.usb.addEventListener('disconnect', e => {"
      "    deviceRemoved = e.device;"
      "});",
      &result));
  EXPECT_EQ("", result);

  RemoveMockDevice();
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "if (deviceRemoved === null) {"
      "  domAutomationController.send('null');"
      "} else {"
      "  domAutomationController.send(deviceRemoved.serialNumber);"
      "}",
      &result));
  EXPECT_EQ("", result);
}

}  // namespace
