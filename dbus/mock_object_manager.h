// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DBUS_MOCK_OBJECT_MANAGER_H_
#define DBUS_MOCK_OBJECT_MANAGER_H_

#include <string>

#include "dbus/message.h"
#include "dbus/object_manager.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace dbus {

// Mock for ObjectManager.
class MockObjectManager : public ObjectManager {
 public:
  MockObjectManager(Bus* bus,
                    const std::string& service_name,
                    const ObjectPath& object_path);

  MOCK_METHOD2(RegisterInterface, void(const std::string&,
                                       Interface*));
  MOCK_METHOD1(UnregisterInterface, void(const std::string&));
  MOCK_METHOD0(GetObjects, std::vector<ObjectPath>());
  MOCK_METHOD1(GetObjectsWithInterface,
               std::vector<ObjectPath>(const std::string&));
  MOCK_METHOD1(GetObjectProxy, ObjectProxy*(const ObjectPath&));
  MOCK_METHOD2(GetProperties, PropertySet*(const ObjectPath&,
                                           const std::string&));

 protected:
  ~MockObjectManager() override;
};

}  // namespace dbus

#endif  // DBUS_MOCK_OBJECT_MANAGER_H_
