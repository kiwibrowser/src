// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_ASYNC_EVENT_DISPATCHER_LOOKUP_H_
#define SERVICES_UI_WS_ASYNC_EVENT_DISPATCHER_LOOKUP_H_

#include "services/ui/common/types.h"

namespace ui {
namespace ws {

class AsyncEventDispatcher;

// Looks up an AsyncEventDispatcher by id. This is used so that event processing
// related code does not have a dependency on WindowTree or WindowService.
class AsyncEventDispatcherLookup {
 public:
  virtual AsyncEventDispatcher* GetAsyncEventDispatcherById(
      ClientSpecificId id) = 0;
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_ASYNC_EVENT_DISPATCHER_LOOKUP_H_
