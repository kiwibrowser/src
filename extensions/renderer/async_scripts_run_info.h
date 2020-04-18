// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_ASYNC_SCRIPTS_RUN_INFO_H_
#define EXTENSIONS_RENDERER_ASYNC_SCRIPTS_RUN_INFO_H_

#include "base/memory/ref_counted.h"
#include "base/optional.h"

#include "extensions/common/user_script.h"

namespace extensions {

// Collects information about asynchronously injected script runs for a
// run_location.
class AsyncScriptsRunInfo : public base::RefCounted<AsyncScriptsRunInfo> {
 public:
  AsyncScriptsRunInfo(UserScript::RunLocation location);
  void WillExecute(const base::TimeTicks& timestamp);
  void OnCompleted(const base::TimeTicks& timestamp,
                   base::Optional<base::TimeDelta> elapsed);

 private:
  friend class base::RefCounted<AsyncScriptsRunInfo>;
  ~AsyncScriptsRunInfo();

  UserScript::RunLocation run_location_;

  // Time stamp of the last OnCompleted() call.
  base::TimeTicks last_completed_time_;

  DISALLOW_COPY_AND_ASSIGN(AsyncScriptsRunInfo);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_SCRIPTS_RUN_INFO_H_
