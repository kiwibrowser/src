// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_PORTS_USER_DATA_H_
#define MOJO_EDK_SYSTEM_PORTS_USER_DATA_H_

#include "base/memory/ref_counted.h"

namespace mojo {
namespace edk {
namespace ports {

class UserData : public base::RefCountedThreadSafe<UserData> {
 protected:
  friend class base::RefCountedThreadSafe<UserData>;

  virtual ~UserData() {}
};

}  // namespace ports
}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_PORTS_USER_DATA_H_
