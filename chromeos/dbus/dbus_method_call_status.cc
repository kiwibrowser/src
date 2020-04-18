// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/dbus_method_call_status.h"

#include "base/bind.h"

namespace chromeos {

VoidDBusMethodCallback EmptyVoidDBusMethodCallback() {
  return base::BindOnce([](bool result) {
    // Do nothing.
  });
}

}  // namespace chromeos
