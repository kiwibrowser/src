// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "extensions/browser/api/device_permissions_prompt.h"
#include "extensions/shell/browser/shell_extensions_api_client.h"
#include "extensions/shell/test/shell_apitest.h"
#include "extensions/test/extension_test_message_listener.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/public/cpp/hid/hid_report_descriptor.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"

using base::ThreadTaskRunnerHandle;
using device::HidReportDescriptor;

const char* const kTestDeviceGuids[] = {"A", "B", "C", "D", "E"};

// These report descriptors define two devices with 8-byte input, output and
// feature reports. The first implements usage page 0xFF00 and has a single
// report without and ID. The second implements usage page 0xFF01 and has a
// single report with ID 1.
const uint8_t kReportDescriptor[] = {0x06, 0x00, 0xFF, 0x08, 0xA1, 0x01, 0x15,
                                     0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95,
                                     0x08, 0x08, 0x81, 0x02, 0x08, 0x91, 0x02,
                                     0x08, 0xB1, 0x02, 0xC0};
const uint8_t kReportDescriptorWithIDs[] = {
    0x06, 0x01, 0xFF, 0x08, 0xA1, 0x01, 0x15, 0x00, 0x26,
    0xFF, 0x00, 0x85, 0x01, 0x75, 0x08, 0x95, 0x08, 0x08,
    0x81, 0x02, 0x08, 0x91, 0x02, 0x08, 0xB1, 0x02, 0xC0};

namespace extensions {

class FakeHidConnectionImpl : public device::mojom::HidConnection {
 public:
  explicit FakeHidConnectionImpl(device::mojom::HidDeviceInfoPtr device)
      : device_(std::move(device)) {}

  ~FakeHidConnectionImpl() override = default;

  // device::mojom::HidConnection implemenation:
  void Read(ReadCallback callback) override {
    const char kResult[] = "This is a HID input report.";
    uint8_t report_id = device_->has_report_id ? 1 : 0;

    std::vector<uint8_t> buffer(kResult, kResult + sizeof(kResult) - 1);

    std::move(callback).Run(true, report_id, buffer);
  }

  void Write(uint8_t report_id,
             const std::vector<uint8_t>& buffer,
             WriteCallback callback) override {
    const char kExpected[] = "o-report";  // 8 bytes
    if (buffer.size() != sizeof(kExpected) - 1) {
      std::move(callback).Run(false);
      return;
    }

    int expected_report_id = device_->has_report_id ? 1 : 0;
    if (report_id != expected_report_id) {
      std::move(callback).Run(false);
      return;
    }

    if (memcmp(buffer.data(), kExpected, sizeof(kExpected) - 1) != 0) {
      std::move(callback).Run(false);
      return;
    }

    std::move(callback).Run(true);
  }

  void GetFeatureReport(uint8_t report_id,
                        GetFeatureReportCallback callback) override {
    uint8_t expected_report_id = device_->has_report_id ? 1 : 0;
    if (report_id != expected_report_id) {
      std::move(callback).Run(false, base::nullopt);
      return;
    }

    const char kResult[] = "This is a HID feature report.";
    std::vector<uint8_t> buffer;
    if (device_->has_report_id)
      buffer.push_back(report_id);
    buffer.insert(buffer.end(), kResult, kResult + sizeof(kResult) - 1);

    std::move(callback).Run(true, buffer);
  }

  void SendFeatureReport(uint8_t report_id,
                         const std::vector<uint8_t>& buffer,
                         SendFeatureReportCallback callback) override {
    const char kExpected[] = "The app is setting this HID feature report.";
    if (buffer.size() != sizeof(kExpected) - 1) {
      std::move(callback).Run(false);
      return;
    }

    int expected_report_id = device_->has_report_id ? 1 : 0;
    if (report_id != expected_report_id) {
      std::move(callback).Run(false);
      return;
    }

    if (memcmp(buffer.data(), kExpected, sizeof(kExpected) - 1) != 0) {
      std::move(callback).Run(false);
      return;
    }

    std::move(callback).Run(true);
  }

 private:
  device::mojom::HidDeviceInfoPtr device_;
};

class FakeHidManager : public device::mojom::HidManager {
 public:
  FakeHidManager() {}
  ~FakeHidManager() override = default;

  void Bind(const std::string& interface_name,
            mojo::ScopedMessagePipeHandle handle,
            const service_manager::BindSourceInfo& source_info) {
    bindings_.AddBinding(this,
                         device::mojom::HidManagerRequest(std::move(handle)));
  }

  // device::mojom::HidManager implementation:
  void GetDevicesAndSetClient(
      device::mojom::HidManagerClientAssociatedPtrInfo client,
      GetDevicesCallback callback) override {
    std::vector<device::mojom::HidDeviceInfoPtr> device_list;
    for (auto& map_entry : devices_)
      device_list.push_back(map_entry.second->Clone());

    std::move(callback).Run(std::move(device_list));

    device::mojom::HidManagerClientAssociatedPtr client_ptr;
    client_ptr.Bind(std::move(client));
    clients_.AddPtr(std::move(client_ptr));
  }

  void GetDevices(GetDevicesCallback callback) override {
    // Clients of HidManager in extensions only use GetDevicesAndSetClient().
    NOTREACHED();
  }

  void Connect(const std::string& device_guid,
               ConnectCallback callback) override {
    if (!base::ContainsKey(devices_, device_guid)) {
      std::move(callback).Run(nullptr);
      return;
    }

    // Strong binds a instance of FakeHidConnctionImpl.
    device::mojom::HidConnectionPtr client;
    mojo::MakeStrongBinding(
        std::make_unique<FakeHidConnectionImpl>(devices_[device_guid]->Clone()),
        mojo::MakeRequest(&client));
    std::move(callback).Run(std::move(client));
  }

  void AddDevice(device::mojom::HidDeviceInfoPtr device) {
    std::string guid = device->guid;
    devices_[guid] = std::move(device);

    device::mojom::HidDeviceInfo* device_info = devices_[guid].get();
    clients_.ForAllPtrs([device_info](device::mojom::HidManagerClient* client) {
      client->DeviceAdded(device_info->Clone());
    });
  }

  void RemoveDevice(const std::string& guid) {
    if (base::ContainsKey(devices_, guid)) {
      device::mojom::HidDeviceInfo* device_info = devices_[guid].get();
      clients_.ForAllPtrs(
          [device_info](device::mojom::HidManagerClient* client) {
            client->DeviceRemoved(device_info->Clone());
          });
      devices_.erase(guid);
    }
  }

 private:
  std::map<std::string, device::mojom::HidDeviceInfoPtr> devices_;
  mojo::AssociatedInterfacePtrSet<device::mojom::HidManagerClient> clients_;
  mojo::BindingSet<device::mojom::HidManager> bindings_;
};

class TestDevicePermissionsPrompt
    : public DevicePermissionsPrompt,
      public DevicePermissionsPrompt::Prompt::Observer {
 public:
  explicit TestDevicePermissionsPrompt(content::WebContents* web_contents)
      : DevicePermissionsPrompt(web_contents) {}

  ~TestDevicePermissionsPrompt() override { prompt()->SetObserver(nullptr); }

  void ShowDialog() override { prompt()->SetObserver(this); }

  void OnDeviceAdded(size_t index, const base::string16& device_name) override {
    OnDevicesChanged();
  }

  void OnDeviceRemoved(size_t index,
                       const base::string16& device_name) override {
    OnDevicesChanged();
  }

 private:
  void OnDevicesChanged() {
    if (prompt()->multiple()) {
      for (size_t i = 0; i < prompt()->GetDeviceCount(); ++i) {
        prompt()->GrantDevicePermission(i);
      }
      prompt()->Dismissed();
    } else {
      for (size_t i = 0; i < prompt()->GetDeviceCount(); ++i) {
        // Always choose the device whose serial number is "A".
        if (prompt()->GetDeviceSerialNumber(i) == base::UTF8ToUTF16("A")) {
          prompt()->GrantDevicePermission(i);
          prompt()->Dismissed();
          return;
        }
      }
    }
  }
};

class TestExtensionsAPIClient : public ShellExtensionsAPIClient {
 public:
  TestExtensionsAPIClient() : ShellExtensionsAPIClient() {}

  std::unique_ptr<DevicePermissionsPrompt> CreateDevicePermissionsPrompt(
      content::WebContents* web_contents) const override {
    return std::make_unique<TestDevicePermissionsPrompt>(web_contents);
  }
};

class HidApiTest : public ShellApiTest {
 public:
  void SetUpOnMainThread() override {
    ShellApiTest::SetUpOnMainThread();

    fake_hid_manager_ = std::make_unique<FakeHidManager>();
    // Because Device Service also runs in this process(browser process), here
    // we can directly set our binder to intercept interface requests against
    // it.
    service_manager::ServiceContext::SetGlobalBinderForTesting(
        device::mojom::kServiceName, device::mojom::HidManager::Name_,
        base::Bind(&FakeHidManager::Bind,
                   base::Unretained(fake_hid_manager_.get())));

    AddDevice(kTestDeviceGuids[0], 0x18D1, 0x58F0, false, "A");
    AddDevice(kTestDeviceGuids[1], 0x18D1, 0x58F0, true, "B");
    AddDevice(kTestDeviceGuids[2], 0x18D1, 0x58F1, false, "C");
  }

  void AddDevice(const std::string& device_guid,
                 int vendor_id,
                 int product_id,
                 bool report_id,
                 std::string serial_number) {
    std::vector<uint8_t> report_descriptor;
    if (report_id) {
      report_descriptor.insert(
          report_descriptor.begin(), kReportDescriptorWithIDs,
          kReportDescriptorWithIDs + sizeof(kReportDescriptorWithIDs));
    } else {
      report_descriptor.insert(report_descriptor.begin(), kReportDescriptor,
                               kReportDescriptor + sizeof(kReportDescriptor));
    }

    std::vector<device::mojom::HidCollectionInfoPtr> collections;
    bool has_report_id;
    size_t max_input_report_size;
    size_t max_output_report_size;
    size_t max_feature_report_size;

    HidReportDescriptor descriptor_parser(report_descriptor);
    descriptor_parser.GetDetails(
        &collections, &has_report_id, &max_input_report_size,
        &max_output_report_size, &max_feature_report_size);

    auto device = device::mojom::HidDeviceInfo::New(
        device_guid, vendor_id, product_id, "Test Device", serial_number,
        device::mojom::HidBusType::kHIDBusTypeUSB, report_descriptor,
        std::move(collections), has_report_id, max_input_report_size,
        max_output_report_size, max_feature_report_size, "");

    fake_hid_manager_->AddDevice(std::move(device));
  }

  FakeHidManager* GetFakeHidManager() { return fake_hid_manager_.get(); }

 protected:
  std::unique_ptr<FakeHidManager> fake_hid_manager_;
};

IN_PROC_BROWSER_TEST_F(HidApiTest, HidApp) {
  ASSERT_TRUE(RunAppTest("api_test/hid/api")) << message_;
}

IN_PROC_BROWSER_TEST_F(HidApiTest, OnDeviceAdded) {
  ExtensionTestMessageListener load_listener("loaded", false);
  ExtensionTestMessageListener result_listener("success", false);
  result_listener.set_failure_message("failure");

  ASSERT_TRUE(LoadApp("api_test/hid/add_event"));
  ASSERT_TRUE(load_listener.WaitUntilSatisfied());

  // Add a blocked device first so that the test will fail if a notification is
  // received.
  AddDevice(kTestDeviceGuids[3], 0x18D1, 0x58F1, false, "A");
  AddDevice(kTestDeviceGuids[4], 0x18D1, 0x58F0, false, "A");
  ASSERT_TRUE(result_listener.WaitUntilSatisfied());
  EXPECT_EQ("success", result_listener.message());
}

IN_PROC_BROWSER_TEST_F(HidApiTest, OnDeviceRemoved) {
  ExtensionTestMessageListener load_listener("loaded", false);
  ExtensionTestMessageListener result_listener("success", false);
  result_listener.set_failure_message("failure");

  ASSERT_TRUE(LoadApp("api_test/hid/remove_event"));
  ASSERT_TRUE(load_listener.WaitUntilSatisfied());

  // Device C was not returned by chrome.hid.getDevices, the app will not get
  // a notification.
  GetFakeHidManager()->RemoveDevice(kTestDeviceGuids[2]);
  // Device A was returned, the app will get a notification.
  GetFakeHidManager()->RemoveDevice(kTestDeviceGuids[0]);
  ASSERT_TRUE(result_listener.WaitUntilSatisfied());
  EXPECT_EQ("success", result_listener.message());
}

IN_PROC_BROWSER_TEST_F(HidApiTest, GetUserSelectedDevices) {
  ExtensionTestMessageListener open_listener("opened_device", false);

  TestExtensionsAPIClient test_api_client;
  ASSERT_TRUE(LoadApp("api_test/hid/get_user_selected_devices"));
  ASSERT_TRUE(open_listener.WaitUntilSatisfied());

  ExtensionTestMessageListener remove_listener("removed", false);
  GetFakeHidManager()->RemoveDevice(kTestDeviceGuids[0]);
  ASSERT_TRUE(remove_listener.WaitUntilSatisfied());

  ExtensionTestMessageListener add_listener("added", false);
  AddDevice(kTestDeviceGuids[0], 0x18D1, 0x58F0, true, "A");
  ASSERT_TRUE(add_listener.WaitUntilSatisfied());
}

}  // namespace extensions
