// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/instance_handle.h"

#include "ppapi/cpp/instance.h"

namespace pp {

InstanceHandle::InstanceHandle(Instance* instance)
    : pp_instance_(instance->pp_instance()) {
}

}  // namespace pp
