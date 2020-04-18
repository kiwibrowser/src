// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/service_manager_proxy.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/binder/command_broker.h"
#include "chromeos/binder/object.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/binder/transaction_data_reader.h"
#include "chromeos/binder/writable_transaction_data.h"

namespace binder {

// static
const char ServiceManagerProxy::kInterfaceName[] = "android.os.IServiceManager";

// static
scoped_refptr<Object> ServiceManagerProxy::CheckService(
    CommandBroker* command_broker,
    const base::string16& service_name) {
  // TODO(hashimoto): Propagate the current thread's strict mode policy
  // (crbug.com/575574).
  const int kStrictModePolicy = 0;
  WritableTransactionData data;
  data.SetCode(CHECK_SERVICE_TRANSACTION);
  data.WriteInterfaceToken(base::ASCIIToUTF16(kInterfaceName),
                           kStrictModePolicy);
  data.WriteString16(service_name);
  std::unique_ptr<TransactionData> reply;
  if (!command_broker->Transact(kContextManagerHandle, data, &reply) ||
      !reply || reply->HasStatus()) {
    LOG(ERROR) << "Failed to get the service.";
    return scoped_refptr<Object>();
  }
  TransactionDataReader reader(*reply);
  return reader.ReadObject(command_broker);
}

// static
bool ServiceManagerProxy::AddService(CommandBroker* command_broker,
                                     const base::string16& service_name,
                                     scoped_refptr<Object> object,
                                     int options) {
  // TODO(hashimoto): Propagate the current thread's strict mode policy
  // (crbug.com/575574).
  const int kStrictModePolicy = 0;
  WritableTransactionData data;
  data.SetCode(ADD_SERVICE_TRANSACTION);
  data.WriteInterfaceToken(base::ASCIIToUTF16(kInterfaceName),
                           kStrictModePolicy);
  data.WriteString16(service_name);
  data.WriteObject(object);
  data.WriteInt32((options & ALLOW_ISOLATED) ? 1 : 0);
  std::unique_ptr<TransactionData> reply;
  if (!command_broker->Transact(kContextManagerHandle, data, &reply) ||
      !reply || reply->HasStatus()) {
    LOG(ERROR) << "Failed to add the service.";
    return false;
  }
  return true;
}

}  // namespace binder
