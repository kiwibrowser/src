// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dbus/mock_object_manager.h"

namespace dbus {

MockObjectManager::MockObjectManager(Bus* bus,
                                     const std::string& service_name,
                                     const ObjectPath& object_path)
    : ObjectManager(bus, service_name, object_path) {
}

MockObjectManager::~MockObjectManager() = default;

}  // namespace dbus
