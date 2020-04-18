// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_REMOTE_OBJECT_H_
#define CHROMEOS_BINDER_REMOTE_OBJECT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromeos/binder/object.h"
#include "chromeos/chromeos_export.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace binder {

class CommandBroker;

// Object living in a remote process.
class CHROMEOS_EXPORT RemoteObject : public Object {
 public:
  RemoteObject(CommandBroker* command_broker, int32_t handle);

  // Returns the handle of this object.
  int32_t GetHandle() const { return handle_; }

  // Object override:
  Type GetType() const override;
  bool Transact(CommandBroker* command_broker,
                const TransactionData& data,
                std::unique_ptr<TransactionData>* reply) override;

 protected:
  ~RemoteObject() override;

 private:
  base::Closure release_closure_;
  scoped_refptr<base::SingleThreadTaskRunner> release_task_runner_;
  int32_t handle_;

  DISALLOW_COPY_AND_ASSIGN(RemoteObject);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_REMOTE_OBJECT_H_
