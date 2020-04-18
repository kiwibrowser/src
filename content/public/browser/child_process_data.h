// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_CHILD_PROCESS_DATA_H_
#define CONTENT_PUBLIC_BROWSER_CHILD_PROCESS_DATA_H_

#include "base/process/process.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"

namespace content {

// Holds information about a child process.
struct ChildProcessData {
  // The type of the process. See the content::ProcessType enum for the
  // well-known process types.
  int process_type;

  // The name of the process.  i.e. for plugins it might be Flash, while for
  // for workers it might be the domain that it's from.
  base::string16 name;

  // The unique identifier for this child process. This identifier is NOT a
  // process ID, and will be unique for all types of child process for
  // one run of the browser.
  int id;

  // The handle to the process. May have value kNullProcessHandle if no process
  // exists - either because it hasn't been started yet or it's running in the
  // current process.
  base::ProcessHandle handle;

  explicit ChildProcessData(int process_type)
      : process_type(process_type),
        id(0),
        handle(base::kNullProcessHandle) {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_CHILD_PROCESS_DATA_H_
