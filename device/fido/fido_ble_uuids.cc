// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_uuids.h"

#include "build/build_config.h"

namespace device {

const char kFidoServiceUUID[] = "0000fffd-0000-1000-8000-00805f9b34fb";
const char kFidoControlPointUUID[] = "f1d0fff1-deaa-ecee-b42f-c9ba7ed623bb";
const char kFidoStatusUUID[] = "f1d0fff2-deaa-ecee-b42f-c9ba7ed623bb";
const char kFidoControlPointLengthUUID[] =
    "f1d0fff3-deaa-ecee-b42f-c9ba7ed623bb";
const char kFidoServiceRevisionUUID[] = "00002a28-0000-1000-8000-00805f9b34fb";
const char kFidoServiceRevisionBitfieldUUID[] =
    "f1d0fff4-deaa-ecee-b42f-c9ba7ed623bb";

#if defined(OS_MACOSX)
const char kCableAdvertisementUUID[] = "fde2";
#else
const char kCableAdvertisementUUID[] = "0000fde2-0000-1000-8000-00805f9b34fb";
#endif

}  // namespace device
