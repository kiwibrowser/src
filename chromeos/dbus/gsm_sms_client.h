// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_GSM_SMS_CLIENT_H_
#define CHROMEOS_DBUS_GSM_SMS_CLIENT_H_

#include <stdint.h>

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace dbus {
class ObjectPath;
}

namespace chromeos {

// GsmSMSClient is used to communicate with the
// org.freedesktop.ModemManager.Modem.Gsm.SMS service.
// All methods should be called from the origin thread (UI thread) which
// initializes the DBusThreadManager instance.
class CHROMEOS_EXPORT GsmSMSClient : public DBusClient {
 public:
  typedef base::Callback<void(uint32_t index, bool complete)>
      SmsReceivedHandler;

  ~GsmSMSClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static GsmSMSClient* Create();

  // Sets SmsReceived signal handler.
  virtual void SetSmsReceivedHandler(const std::string& service_name,
                                     const dbus::ObjectPath& object_path,
                                     const SmsReceivedHandler& handler) = 0;

  // Resets SmsReceived signal handler.
  virtual void ResetSmsReceivedHandler(const std::string& service_name,
                                       const dbus::ObjectPath& object_path) = 0;

  // Calls Delete method.  |callback| is called on method call completion.
  virtual void Delete(const std::string& service_name,
                      const dbus::ObjectPath& object_path,
                      uint32_t index,
                      VoidDBusMethodCallback callback) = 0;

  // Calls Get method.  |callback| is called on method call completion.
  virtual void Get(const std::string& service_name,
                   const dbus::ObjectPath& object_path,
                   uint32_t index,
                   DBusMethodCallback<base::DictionaryValue> callback) = 0;

  // Calls List method.  |callback| is called on method call completion.
  virtual void List(const std::string& service_name,
                    const dbus::ObjectPath& object_path,
                    DBusMethodCallback<base::ListValue> callback) = 0;

  // Requests a check for new messages. In shill this does nothing. The
  // stub implementation uses it to generate a sequence of test messages.
  virtual void RequestUpdate(const std::string& service_name,
                             const dbus::ObjectPath& object_path) = 0;

 protected:
  friend class GsmSMSClientTest;

  // Create() should be used instead.
  GsmSMSClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(GsmSMSClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_GSM_SMS_CLIENT_H_
