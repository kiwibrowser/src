// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_MEMORY_CHILD_MEMORY_COORDINATOR_IMPL_ANDROID_H_
#define CONTENT_CHILD_MEMORY_CHILD_MEMORY_COORDINATOR_IMPL_ANDROID_H_

#include "content/child/memory/child_memory_coordinator_impl.h"

namespace content {

class CONTENT_EXPORT ChildMemoryCoordinatorImplAndroid
    : public ChildMemoryCoordinatorImpl {
 public:
  ChildMemoryCoordinatorImplAndroid(mojom::MemoryCoordinatorHandlePtr parent,
                                    ChildMemoryCoordinatorDelegate* delegate);
  ~ChildMemoryCoordinatorImplAndroid() override;

  // TODO(bashi): Add JNI bindings to listen to OnTrimMemory()

  // Android's ComponentCallback2.onTrimMemory() implementation.
  void OnTrimMemory(int level);

 private:
  DISALLOW_COPY_AND_ASSIGN(ChildMemoryCoordinatorImplAndroid);
};

}  // namespace content

#endif  // CONTENT_CHILD_MEMORY_CHILD_MEMORY_COORDINATOR_IMPL_ANDROID_H_
