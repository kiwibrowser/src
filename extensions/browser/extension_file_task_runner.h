// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_FILE_TASK_RUNNER_H_
#define EXTENSIONS_BROWSER_EXTENSION_FILE_TASK_RUNNER_H_

#include "base/memory/ref_counted.h"

namespace base {
class SequencedTaskRunner;
}

namespace extensions {

// Returns the singleton instance of the task runner to be used for
// extension-related tasks that read, modify, or delete files. All these tasks
// must be posted to this task runner, even if it is only reading the file,
// since other tasks may be modifying it.
scoped_refptr<base::SequencedTaskRunner> GetExtensionFileTaskRunner();

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_FILE_TASK_RUNNER_H_
