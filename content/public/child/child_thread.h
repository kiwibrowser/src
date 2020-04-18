// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_CHILD_CHILD_THREAD_H_
#define CONTENT_PUBLIC_CHILD_CHILD_THREAD_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "ipc/ipc_sender.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_MACOSX)
#include "mojo/public/cpp/system/buffer.h"
#endif

namespace base {
class SingleThreadTaskRunner;
struct UserMetricsAction;
}

namespace service_manager {
class Connector;
}

namespace content {

class ServiceManagerConnection;

// An abstract base class that contains logic shared between most child
// processes of the embedder.
class CONTENT_EXPORT ChildThread : public IPC::Sender {
 public:
  // Returns the one child thread for this process.  Note that this can only be
  // accessed when running on the child thread itself.
  static ChildThread* Get();

  ~ChildThread() override {}

  // Sends over a base::UserMetricsAction to be recorded by user metrics as
  // an action. Once a new user metric is added, run
  //   tools/metrics/actions/extract_actions.py
  // to add the metric to actions.xml, then update the <owner>s and
  // <description> sections. Make sure to include the actions.xml file when you
  // upload your code for review!
  //
  // WARNING: When using base::UserMetricsAction, base::UserMetricsAction
  // and a string literal parameter must be on the same line, e.g.
  //   RenderThread::Get()->RecordAction(
  //       base::UserMetricsAction("my extremely long action name"));
  // because otherwise our processing scripts won't pick up on new actions.
  virtual void RecordAction(const base::UserMetricsAction& action) = 0;

  // Sends over a string to be recorded by user metrics as a computed action.
  // When you use this you need to also update the rules for extracting known
  // actions in chrome/tools/extract_actions.py.
  virtual void RecordComputedAction(const std::string& action) = 0;

  // Returns the ServiceManagerConnection for the thread (from which a
  // service_manager::Connector can be obtained).
  virtual ServiceManagerConnection* GetServiceManagerConnection() = 0;

  // Returns a connector that can be used to bind interfaces exposed by other
  // services.
  virtual service_manager::Connector* GetConnector() = 0;

  virtual scoped_refptr<base::SingleThreadTaskRunner> GetIOTaskRunner() = 0;

  // Tells the child process that a field trial was activated.
  virtual void SetFieldTrialGroup(const std::string& trial_name,
                                  const std::string& group_name) = 0;

#if defined(OS_WIN)
  // Request that the given font be loaded by the browser so it's cached by the
  // OS. Please see ChildProcessHost::PreCacheFont for details.
  virtual void PreCacheFont(const LOGFONT& log_font) = 0;

  // Release cached font.
  virtual void ReleaseCachedFonts() = 0;
#elif defined(OS_MACOSX)
  // Load specified font into shared memory.
  virtual bool LoadFont(const base::string16& font_name,
                        float font_point_size,
                        mojo::ScopedSharedBufferHandle* out_font_data,
                        uint32_t* out_font_id) = 0;
#endif
};

}  // namespace content

#endif  // CONTENT_PUBLIC_CHILD_CHILD_THREAD_H_
