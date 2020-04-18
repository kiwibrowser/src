// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dbus/mock_exported_object.h"

namespace dbus {

MockExportedObject::MockExportedObject(Bus* bus,
                                       const ObjectPath& object_path)
    : ExportedObject(bus, object_path) {
}

MockExportedObject::~MockExportedObject() = default;

}  // namespace dbus
