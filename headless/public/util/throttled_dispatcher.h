// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_THROTTLED_DISPATCHER_H_
#define HEADLESS_PUBLIC_UTIL_THROTTLED_DISPATCHER_H_

#include <vector>

#include "base/single_thread_task_runner.h"
#include "headless/public/util/expedited_dispatcher.h"

namespace headless {

class ManagedDispatchURLRequestJob;

// Similar to ExpeditedDispatcher except network fetches can be paused.
class HEADLESS_EXPORT ThrottledDispatcher : public ExpeditedDispatcher {
 public:
  explicit ThrottledDispatcher(
      scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner);
  ~ThrottledDispatcher() override;

  void PauseRequests();
  void ResumeRequests();

  // UrlRequestDispatcher implementation:
  void DataReady(ManagedDispatchURLRequestJob* job) override;
  void JobDeleted(ManagedDispatchURLRequestJob* job) override;

 private:
  // Protects all members below.
  base::Lock lock_;
  bool requests_paused_;
  std::vector<ManagedDispatchURLRequestJob*> paused_jobs_;

  DISALLOW_COPY_AND_ASSIGN(ThrottledDispatcher);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_EXPEDITED_DISPATCHER_H_
