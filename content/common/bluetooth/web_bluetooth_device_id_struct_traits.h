// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_DEVICE_ID_STRUCT_TRAITS_H_
#define CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_DEVICE_ID_STRUCT_TRAITS_H_

#include <string>

#include "content/common/bluetooth/web_bluetooth_device_id.h"
#include "third_party/blink/public/platform/modules/bluetooth/web_bluetooth.mojom.h"

namespace mojo {

template <>
struct StructTraits<blink::mojom::WebBluetoothDeviceIdDataView,
                    content::WebBluetoothDeviceId> {
  static const std::string& device_id(
      const content::WebBluetoothDeviceId& device_id) {
    return device_id.str();
  }

  static bool Read(blink::mojom::WebBluetoothDeviceIdDataView input,
                   content::WebBluetoothDeviceId* output) {
    std::string result;

    if (!input.ReadDeviceId(&result))
      return false;
    if (!content::WebBluetoothDeviceId::IsValid(result))
      return false;

    *output = content::WebBluetoothDeviceId(std::move(result));
    return true;
  }
};

}  // namespace mojo

#endif  // CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_DEVICE_ID_STRUCT_TRAITS_H_
