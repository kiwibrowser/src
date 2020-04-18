// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_JS_SYNC_JS_CONTROLLER_H_
#define COMPONENTS_SYNC_JS_SYNC_JS_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/sync/base/weak_handle.h"
#include "components/sync/js/js_controller.h"
#include "components/sync/js/js_event_handler.h"

namespace syncer {

class JsBackend;

// A class that mediates between the sync JsEventHandlers and the sync
// JsBackend.
class SyncJsController : public JsController,
                         public JsEventHandler,
                         public base::SupportsWeakPtr<SyncJsController> {
 public:
  SyncJsController();

  ~SyncJsController() override;

  // Sets the backend to route all messages to (if initialized).
  // Sends any queued-up messages if |backend| is initialized.
  void AttachJsBackend(const WeakHandle<JsBackend>& js_backend);

  // JsController implementation.
  void AddJsEventHandler(JsEventHandler* event_handler) override;
  void RemoveJsEventHandler(JsEventHandler* event_handler) override;

  // JsEventHandler implementation.
  void HandleJsEvent(const std::string& name,
                     const JsEventDetails& details) override;

 private:
  // Sets |js_backend_|'s event handler depending on how many
  // underlying event handlers we have.
  void UpdateBackendEventHandler();

  WeakHandle<JsBackend> js_backend_;
  base::ObserverList<JsEventHandler> js_event_handlers_;

  DISALLOW_COPY_AND_ASSIGN(SyncJsController);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_JS_SYNC_JS_CONTROLLER_H_
