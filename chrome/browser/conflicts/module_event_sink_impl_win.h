// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MODULE_EVENT_SINK_IMPL_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MODULE_EVENT_SINK_IMPL_WIN_H_

#include <stdint.h>

#include "base/callback_forward.h"
#include "base/process/process.h"
#include "chrome/common/conflicts/module_event_sink_win.mojom.h"
#include "content/public/common/process_type.h"

class ModuleDatabase;

// Implementation of the mojom::ModuleEventSink interface. This is the endpoint
// in the browser process. This redirects calls to the singleton ModuleDatabase
// object.
class ModuleEventSinkImpl : public mojom::ModuleEventSink {
 public:
  // Callback for retrieving the handle associated with a process. This is used
  // by "Create" to get a handle to the remote process.
  using GetProcessCallback = base::Callback<base::Process()>;

  // Creates a service endpoint that forwards notifications from the remote
  // |process| of the provided |process_type| to the provided |module_database|.
  // The |module_database| must outlive this object.
  ModuleEventSinkImpl(base::Process process,
                      content::ProcessType process_type,
                      ModuleDatabase* module_database);
  ~ModuleEventSinkImpl() override;

  // Factory function for use with service_manager::InterfaceRegistry. This
  // creates a concrete implementation of mojom::ModuleDatabase interface in the
  // current process, for the remote process represented by the provided
  // |request|. This should only be called on the UI thread.
  static void Create(GetProcessCallback get_process,
                     content::ProcessType process_type,
                     ModuleDatabase* module_database,
                     mojom::ModuleEventSinkRequest request);

  // mojom::ModuleEventSink implementation:
  void OnModuleEvent(mojom::ModuleEventType event_type,
                     uint64_t load_address) override;

 private:
  friend class ModuleEventSinkImplTest;

  // A handle to the process on the other side of the pipe.
  base::Process process_;

  // The module database this forwards events to. The |module_database| must
  // outlive this object.
  ModuleDatabase* module_database_;

  // The process ID of the remote process on the other end of the pipe. This is
  // forwarded along to the ModuleDatabase for each call.
  content::ProcessType process_type_;

  DISALLOW_COPY_AND_ASSIGN(ModuleEventSinkImpl);
};

#endif  // CHROME_BROWSER_CONFLICTS_MODULE_EVENT_SINK_IMPL_WIN_H_
