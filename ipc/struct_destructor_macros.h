// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_STRUCT_DESTRUCTOR_MACROS_H_
#define IPC_STRUCT_DESTRUCTOR_MACROS_H_

// Null out all the macros that need nulling.
#include "ipc/ipc_message_null_macros.h"

// Set up so next include will generate destructors.
#undef IPC_STRUCT_BEGIN_WITH_PARENT
#define IPC_STRUCT_BEGIN_WITH_PARENT(struct_name, parent) \
  struct_name::~struct_name() {}

#endif  // IPC_STRUCT_DESTRUCTOR_MACROS_H_

