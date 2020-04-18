// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_PUBLIC_ACK_HANDLER_H_
#define COMPONENTS_INVALIDATION_PUBLIC_ACK_HANDLER_H_

#include <vector>

#include "components/invalidation/public/invalidation_export.h"

namespace invalidation {
class ObjectId;
}  // namespace invalidation

namespace syncer {

class AckHandle;

// An interface for classes that keep track of invalidation acknowledgements.
//
// We don't expect to support more than one "real" implementation of AckHandler,
// but this interface is very useful for testing and implementation hiding.
class INVALIDATION_EXPORT AckHandler {
 public:
  AckHandler();
  virtual ~AckHandler() = 0;

  // Record the local acknowledgement of an invalidation identified by |handle|.
  virtual void Acknowledge(const invalidation::ObjectId& id,
                           const AckHandle& handle) = 0;

  // Record the drop of an invalidation identified by |handle|.
  virtual void Drop(const invalidation::ObjectId& id,
                    const AckHandle& handle) = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_PUBLIC_ACK_HANDLER_H_
