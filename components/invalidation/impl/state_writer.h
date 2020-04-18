// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Simple interface for something that persists state.

#ifndef COMPONENTS_INVALIDATION_IMPL_STATE_WRITER_H_
#define COMPONENTS_INVALIDATION_IMPL_STATE_WRITER_H_

#include <string>

#include "components/invalidation/public/invalidation_export.h"

namespace syncer {

class INVALIDATION_EXPORT StateWriter {
 public:
  virtual ~StateWriter() {}

  virtual void WriteState(const std::string& state) = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_STATE_WRITER_H_
