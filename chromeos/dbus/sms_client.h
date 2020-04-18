// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SMS_CLIENT_H_
#define CHROMEOS_DBUS_SMS_CLIENT_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"

namespace base {
class DictionaryValue;
}

namespace dbus {
class ObjectPath;
}

namespace chromeos {

// SMSMessageClient is used to communicate with the
// org.freedesktop.ModemManager1.SMS service.  All methods should be
// called from the origin thread (UI thread) which initializes the
// DBusThreadManager instance.
class CHROMEOS_EXPORT SMSClient : public DBusClient {
 public:
  using GetAllCallback =
      base::OnceCallback<void(const base::DictionaryValue& sms)>;

  static const char kSMSPropertyState[];
  static const char kSMSPropertyNumber[];
  static const char kSMSPropertyText[];
  static const char kSMSPropertyTimestamp[];

  ~SMSClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static SMSClient* Create();

  // Calls GetAll method.  |callback| is called after the method call succeeds.
  virtual void GetAll(const std::string& service_name,
                      const dbus::ObjectPath& object_path,
                      GetAllCallback callback) = 0;

 protected:
  // Create() should be used instead.
  SMSClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(SMSClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SMS_CLIENT_H_
