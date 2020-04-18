// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_IO_THREAD_EXTENSION_MESSAGE_FILTER_H_
#define EXTENSIONS_BROWSER_IO_THREAD_EXTENSION_MESSAGE_FILTER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_message_filter.h"

struct ExtensionHostMsg_Request_Params;

namespace content {
class BrowserContext;
}

namespace extensions {

class InfoMap;

// This class filters out incoming extension-specific IPC messages from the
// renderer process. It is created on the UI thread, but handles messages on the
// IO thread and is destroyed there.
class IOThreadExtensionMessageFilter : public content::BrowserMessageFilter {
 public:
  IOThreadExtensionMessageFilter(int render_process_id,
                                 content::BrowserContext* context);

  int render_process_id() { return render_process_id_; }

 private:
  friend class base::DeleteHelper<IOThreadExtensionMessageFilter>;
  friend class content::BrowserThread;

  ~IOThreadExtensionMessageFilter() override;

  // content::BrowserMessageFilter implementation.
  void OnDestruct() const override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // Message handlers on the IO thread.
  void OnExtensionGenerateUniqueID(int* unique_id);
  void OnExtensionRequestForIOThread(
      int routing_id,
      const ExtensionHostMsg_Request_Params& params);

  const int render_process_id_;

  // The browser context as a void pointer, for use as an identifier on the IO
  // thread.
  void* browser_context_id_;

  scoped_refptr<extensions::InfoMap> extension_info_map_;

  // Weak pointers produced by this factory are bound to the IO thread.
  base::WeakPtrFactory<IOThreadExtensionMessageFilter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(IOThreadExtensionMessageFilter);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_IO_THREAD_EXTENSION_MESSAGE_FILTER_H_
