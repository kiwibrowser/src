// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_DATA_TYPE_ENCRYPTION_HANDLER_H_
#define COMPONENTS_SYNC_DRIVER_DATA_TYPE_ENCRYPTION_HANDLER_H_

#include "components/sync/base/model_type.h"

namespace syncer {

// The DataTypeEncryptionHandler provides the status of datatype encryption.
class DataTypeEncryptionHandler {
 public:
  DataTypeEncryptionHandler();
  virtual ~DataTypeEncryptionHandler();

  // Returns true if a passphrase is required for encryption to proceed, false
  // otherwise.
  virtual bool IsPassphraseRequired() const = 0;

  // Returns the current set of encrypted data types.
  virtual ModelTypeSet GetEncryptedDataTypes() const = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_DATA_TYPE_ENCRYPTION_HANDLER_H_
