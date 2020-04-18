// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_GET_SESSION_NAME_H_
#define COMPONENTS_SYNC_BASE_GET_SESSION_NAME_H_

#include <string>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"

namespace base {
class TaskRunner;
}  // namespace base

namespace syncer {

// Invokes |done_callback| with the session name, a UTF-8 string.
void GetSessionName(
    const scoped_refptr<base::TaskRunner>& task_runner,
    const base::Callback<void(const std::string&)>& done_callback);

std::string GetSessionNameSynchronouslyForTesting();

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_GET_SESSION_NAME_H_
