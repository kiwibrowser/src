// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_FAKE_BLUETOOTH_CHOOSER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_FAKE_BLUETOOTH_CHOOSER_H_

#include <memory>

#include "content/public/browser/bluetooth_chooser.h"
#include "content/shell/common/layout_test/fake_bluetooth_chooser.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

// Implementation of FakeBluetoothChooser in
// src/content/shell/common/layout_test/fake_bluetooth_chooser.mojom
// to provide a method of controlling the Bluetooth chooser during a test.
// Serves as a Bluetooth chooser factory for choosers that can be manually
// controlled through the Mojo API. Only one instance of this class will exist
// while the chooser is active.
//
// The implementation details for FakeBluetoothChooser can be found in the Web
// Bluetooth Test Scanning design document.
// https://docs.google.com/document/d/1XFl_4ZAgO8ddM6U53A9AfUuZeWgJnlYD5wtbXqEpzeg
//
// Intended to only be used through the FakeBluetoothChooser Mojo interface.
class FakeBluetoothChooser : public mojom::FakeBluetoothChooser,
                             public BluetoothChooser {
 public:
  // Resets the test scan duration to timeout immediately.
  ~FakeBluetoothChooser() override;

  // LayoutTestContentBrowserClient will create an instance of this class when a
  // request is bound. It will maintain ownership of the instance temporarily
  // until the chooser is opened. When the chooser is opened, ownership of this
  // instance will shift to the caller of
  // WebContentsDelegate::RunBluetoothChooser.
  static std::unique_ptr<FakeBluetoothChooser> Create(
      mojom::FakeBluetoothChooserRequest request);

  // Sets the EventHandler that will handle events produced by the chooser.
  void SetEventHandler(const EventHandler& event_handler);

  // mojom::FakeBluetoothChooser overrides:

  void WaitForEvents(uint32_t num_of_events,
                     WaitForEventsCallback callback) override;
  void SelectPeripheral(const std::string& peripheral_address,
                        SelectPeripheralCallback callback) override;
  void Cancel(CancelCallback callback) override;
  void Rescan(RescanCallback callback) override;

  // BluetoothChooser overrides:

  void SetAdapterPresence(AdapterPresence presence) override;
  void ShowDiscoveryState(DiscoveryState state) override;
  void AddOrUpdateDevice(const std::string& device_id,
                         bool should_update_name,
                         const base::string16& device_name,
                         bool is_gatt_connected,
                         bool is_paired,
                         int signal_strength_level) override;

 private:
  explicit FakeBluetoothChooser(mojom::FakeBluetoothChooserRequest request);

  // Stores the callback function that handles chooser events.
  EventHandler event_handler_;

  mojo::Binding<mojom::FakeBluetoothChooser> binding_;

  DISALLOW_COPY_AND_ASSIGN(FakeBluetoothChooser);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_FAKE_BLUETOOTH_CHOOSER_H_
