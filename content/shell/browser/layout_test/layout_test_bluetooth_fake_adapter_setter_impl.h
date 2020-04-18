// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BLUETOOTH_FAKE_ADAPTER_SETTER_IMPL_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BLUETOOTH_FAKE_ADAPTER_SETTER_IMPL_H_

#include "base/macros.h"
#include "content/shell/common/layout_test/layout_test_bluetooth_fake_adapter_setter.mojom.h"

namespace content {

class LayoutTestBluetoothFakeAdapterSetterImpl
    : public mojom::LayoutTestBluetoothFakeAdapterSetter {
 public:
  LayoutTestBluetoothFakeAdapterSetterImpl();
  ~LayoutTestBluetoothFakeAdapterSetterImpl() override;

  static void Create(
      mojom::LayoutTestBluetoothFakeAdapterSetterRequest request);

 private:
  void Set(const std::string& adapter_name, SetCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(LayoutTestBluetoothFakeAdapterSetterImpl);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_BLUETOOTH_FAKE_ADAPTER_SETTER_IMPL_H_
