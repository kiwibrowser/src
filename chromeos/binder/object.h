// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_OBJECT_H_
#define CHROMEOS_BINDER_OBJECT_H_

#include <memory>

#include "base/memory/ref_counted.h"

namespace binder {

class CommandBroker;
class TransactionData;

// An object to perform a transaction.
class Object : public base::RefCountedThreadSafe<Object> {
 public:
  // Type of an object.
  enum Type {
    TYPE_LOCAL,   // This object lives in this process.
    TYPE_REMOTE,  // This object lives in a remote process.
  };

  // Returns the type of this object.
  virtual Type GetType() const = 0;

  // Performs a transaction.
  virtual bool Transact(CommandBroker* command_broker,
                        const TransactionData& data,
                        std::unique_ptr<TransactionData>* reply) = 0;

 protected:
  friend class base::RefCountedThreadSafe<Object>;
  virtual ~Object() {}
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_OBJECT_H_
