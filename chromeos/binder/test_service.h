// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_TEST_SERVICE_H_
#define CHROMEOS_BINDER_TEST_SERVICE_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chromeos/binder/constants.h"
#include "chromeos/binder/ipc_thread.h"

namespace binder {

// Service for testing.
// Opens binder driver on its own thread and provides a service.
// Note: Although this service runs in the same process as the test code, the
// binder driver thinks this service is in a separate process because it owns a
// separate binder driver FD itself.
class TestService {
 public:
  enum {
    INCREMENT_INT_TRANSACTION = kFirstTransactionCode,
    GET_FD_TRANSACTION,
    WAIT_TRANSACTION,    // Waits for SIGNAL_TRANSACTION.
    SIGNAL_TRANSACTION,  // Signals a waiting thread.
  };

  TestService();
  ~TestService();

  // The name of this service.
  const base::string16& service_name() const { return service_name_; }

  // Starts the service and waits for it to complete initialization.
  // Returns true on success.
  bool StartAndWait();

  // Stops this service.
  void Stop();

  // Returns the contents of the file returned by GET_FD_TRANSACTION.
  static std::string GetFileContents();

 private:
  class TestObject;

  // Initializes the service on the service thread.
  // |result| will be set to true on success.
  void Initialize(bool* result);

  base::string16 service_name_;
  MainIpcThread main_thread_;
  SubIpcThread sub_thread_;

  DISALLOW_COPY_AND_ASSIGN(TestService);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_TEST_SERVICE_H_
