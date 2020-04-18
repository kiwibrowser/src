// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <wayland-server.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/ozone/platform/wayland/fake_server.h"
#include "ui/ozone/platform/wayland/wayland_test.h"
#include "ui/ozone/public/clipboard_delegate.h"

namespace ui {

// This class mocks how a real clipboard/ozone client would
// hook to ClipboardDelegate, with one difference: real clients
// have no access to the WaylandConnection instance like this
// MockClipboardClient impl does. Instead, clients and ozone gets
// plumbbed up by calling the appropriated Ozone API,
// OzonePlatform::GetClipboardDelegate.
class MockClipboardClient {
 public:
  MockClipboardClient(WaylandConnection* connection) {
    DCHECK(connection);
    // See comment above for reasoning to access the WaylandConnection
    // directly from here.
    delegate_ = connection->GetClipboardDelegate();

    DCHECK(delegate_);
  }
  ~MockClipboardClient() = default;

  // Fill the clipboard backing store with sample data.
  void SetData(const std::string& utf8_text,
               const std::string& mime_type,
               ClipboardDelegate::OfferDataClosure callback) {
    // This mimics how Mus' ClipboardImpl writes data to the DataMap.
    std::vector<char> object_map(utf8_text.begin(), utf8_text.end());
    char* object_data = &object_map.front();
    data_types_[mime_type] =
        std::vector<uint8_t>(object_data, object_data + object_map.size());

    delegate_->OfferClipboardData(data_types_, std::move(callback));
  }

  void ReadData(const std::string& mime_type,
                ClipboardDelegate::RequestDataClosure callback) {
    delegate_->RequestClipboardData(mime_type, &data_types_,
                                    std::move(callback));
  }

  bool IsSelectionOwner() { return delegate_->IsSelectionOwner(); }

 private:
  ClipboardDelegate* delegate_ = nullptr;
  ClipboardDelegate::DataMap data_types_;

  DISALLOW_COPY_AND_ASSIGN(MockClipboardClient);
};

class WaylandDataDeviceManagerTest : public WaylandTest {
 public:
  WaylandDataDeviceManagerTest() {}

  void SetUp() override {
    WaylandTest::SetUp();

    Sync();

    data_device_manager_ = server_.data_device_manager();
    DCHECK(data_device_manager_);

    clipboard_client_.reset(new MockClipboardClient(connection_.get()));
  }

 protected:
  wl::MockDataDeviceManager* data_device_manager_;
  std::unique_ptr<MockClipboardClient> clipboard_client_;

  DISALLOW_COPY_AND_ASSIGN(WaylandDataDeviceManagerTest);
};

TEST_P(WaylandDataDeviceManagerTest, WriteToClipboard) {
  // The client writes data to the clipboard ...
  auto callback = base::BindOnce([]() {});
  clipboard_client_->SetData(wl::kSampleClipboardText, wl::kTextMimeTypeUtf8,
                             std::move(callback));
  Sync();

  // ... and the server reads it.
  data_device_manager_->data_source()->ReadData(
      base::BindOnce([](const std::vector<uint8_t>& data) {
        std::string string_data(data.begin(), data.end());
        EXPECT_EQ(wl::kSampleClipboardText, string_data);
      }));
  Sync();
}

TEST_P(WaylandDataDeviceManagerTest, ReadFromClibpard) {
  // TODO: implement this in terms of an actual wl_surface that gets
  // focused and compositor sends data_device data to it.
  auto* data_offer = data_device_manager_->data_device()->OnDataOffer();
  data_offer->OnOffer(wl::kTextMimeTypeUtf8);
  data_device_manager_->data_device()->OnSelection(*data_offer);
  Sync();

  // The client requests to reading clipboard data from the server.
  // The Server writes in some sample data, and we check it matches
  // expectation.
  auto callback =
      base::BindOnce([](const base::Optional<std::vector<uint8_t>>& data) {
        std::string string_data = std::string(data->begin(), data->end());
        EXPECT_EQ(wl::kSampleClipboardText, string_data);
      });
  clipboard_client_->ReadData(wl::kTextMimeTypeUtf8, std::move(callback));
  Sync();
}

TEST_P(WaylandDataDeviceManagerTest, IsSelectionOwner) {
  auto callback = base::BindOnce([]() {});
  clipboard_client_->SetData(wl::kSampleClipboardText, wl::kTextMimeTypeUtf8,
                             std::move(callback));
  Sync();
  ASSERT_TRUE(clipboard_client_->IsSelectionOwner());

  // The compositor sends OnCancelled whenever another application
  // on the system sets a new selection. It means we are not the application
  // that owns the current selection data.
  data_device_manager_->data_source()->OnCancelled();
  Sync();

  ASSERT_FALSE(clipboard_client_->IsSelectionOwner());
}

INSTANTIATE_TEST_CASE_P(XdgVersionV5Test,
                        WaylandDataDeviceManagerTest,
                        ::testing::Values(kXdgShellV5));

INSTANTIATE_TEST_CASE_P(XdgVersionV6Test,
                        WaylandDataDeviceManagerTest,
                        ::testing::Values(kXdgShellV6));

}  // namespace ui
