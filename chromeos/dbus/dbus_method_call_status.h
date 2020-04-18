// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_DBUS_METHOD_CALL_STATUS_H_
#define CHROMEOS_DBUS_DBUS_METHOD_CALL_STATUS_H_

// TODO(hidehiko): Rename this file to dbus_callback.h, when we fully
// get rid of DBusMethodCallStatus enum defined below.

#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "chromeos/chromeos_export.h"

namespace dbus {

class ObjectPath;

}  // namespace dbus

namespace chromeos {

// DEPRECATED. Migrating to DBusMethodCallback style.
// TODO(hidehiko): Remove this when migration is completed.
// An enum to describe whether or not a DBus method call succeeded.
enum DBusMethodCallStatus {
  DBUS_METHOD_CALL_FAILURE,
  DBUS_METHOD_CALL_SUCCESS,
};

// Callback to handle response of methods with result.
// If the method returns multiple values, std::tuple<...> will be used.
// In case of error, nullopt should be passed.
template <typename ResultType>
using DBusMethodCallback =
    base::OnceCallback<void(base::Optional<ResultType> result)>;

// Callback to handle response of methods without result.
// |result| is true if the method call is successfully completed, otherwise
// false.
using VoidDBusMethodCallback = base::OnceCallback<void(bool result)>;

// TODO(crbug.com/739622): Use OnceCallback in following definition, too.

// A callback to handle responses of methods returning a ObjectPath value that
// doesn't get call status.
typedef base::Callback<void(const dbus::ObjectPath& result)> ObjectPathCallback;

// Called when service becomes available.
using WaitForServiceToBeAvailableCallback =
    base::OnceCallback<void(bool service_is_available)>;

// Returns an empty callback that does nothing.
CHROMEOS_EXPORT VoidDBusMethodCallback EmptyVoidDBusMethodCallback();

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_DBUS_METHOD_CALL_STATUS_H_
