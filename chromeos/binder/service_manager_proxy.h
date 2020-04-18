// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_SERVICE_MANAGER_PROXY_H_
#define CHROMEOS_BINDER_SERVICE_MANAGER_PROXY_H_

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "chromeos/binder/constants.h"
#include "chromeos/chromeos_export.h"

namespace binder {

class CommandBroker;
class Object;

// Proxy of the service manager.
// Use this class to communicate with the service manager process.
class CHROMEOS_EXPORT ServiceManagerProxy {
 public:
  static const char kInterfaceName[];

  // Must match with the values in Android's IServiceManager.h.
  // (https://goo.gl/VcPBKL)
  enum {
    GET_SERVICE_TRANSACTION = kFirstTransactionCode,
    CHECK_SERVICE_TRANSACTION,
    ADD_SERVICE_TRANSACTION,
    LIST_SERVICES_TRANSACTION,
  };

  // Options for AddService().
  enum AddServiceOptions {
    // Allow isolated Android processes to access the service.
    // (See http://goo.gl/H95R6h for isolated process.)
    ALLOW_ISOLATED = 1,
  };

  // Returns the service object if it's available.
  static scoped_refptr<Object> CheckService(CommandBroker* command_broker,
                                            const base::string16& service_name);

  // Registers the given object as a service with the specified name.
  // |options| is a bitmask of AddServiceOptions.
  static bool AddService(CommandBroker* command_broker,
                         const base::string16& service_name,
                         scoped_refptr<Object> object,
                         int options);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_SERVICE_MANAGER_PROXY_H_
