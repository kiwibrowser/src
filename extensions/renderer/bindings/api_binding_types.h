// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_BINDINGS_API_BINDING_TYPES_H_
#define EXTENSIONS_RENDERER_BINDINGS_API_BINDING_TYPES_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "v8/include/v8.h"

namespace extensions {
namespace binding {

// A value indicating an event has no maximum listener count.
extern const int kNoListenerMax;

// Types of changes for event listener registration.
enum class EventListenersChanged {
  HAS_LISTENERS,  // The event had no listeners, and now does.
  NO_LISTENERS,   // The event had listeners, and now does not.
};

// The browser thread that the request should be sent to.
enum class RequestThread {
  UI,
  IO,
};

// Adds an error message to the context's console.
using AddConsoleError =
    base::Callback<void(v8::Local<v8::Context>, const std::string& error)>;

}  // namespace binding
}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_BINDINGS_API_BINDING_TYPES_H_
