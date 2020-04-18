// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_ACCELERATORS_MEDIA_KEYS_LISTENER_H_
#define UI_BASE_ACCELERATORS_MEDIA_KEYS_LISTENER_H_

#include <memory>

#include "base/callback.h"
#include "ui/base/ui_base_export.h"

namespace ui {

class Accelerator;

// Create MediaKeyListener to receive accelerators on media keys.
class UI_BASE_EXPORT MediaKeysListener {
 public:
  enum class Scope {
    kGlobal,   // Listener works whenever application in focus or not.
    kFocused,  // Listener only works whan application has focus.
  };

  enum class MediaKeysHandleResult {
    kIgnore,  // Ignore the key and continue propagation to other system apps.
    kSuppressPropagation,  // Handled. Prevent propagation to other system
                           // apps.
  };

  // Media keys accelerators receiver.
  class UI_BASE_EXPORT Delegate {
   public:
    virtual ~Delegate();

    // Called on media key event.
    // Return result - whether event is handled and propagation of event should
    // be suppressed.
    virtual MediaKeysHandleResult OnMediaKeysAccelerator(
        const Accelerator& accelerator) = 0;
  };

  // Can return nullptr if media keys listening is not implemented.
  // Currently implemented only on mac.
  static std::unique_ptr<MediaKeysListener> Create(Delegate* delegate,
                                                   Scope scope);

  virtual ~MediaKeysListener();

  // Start receiving media keys events.
  virtual void StartWatchingMediaKeys() = 0;
  // Stop receiving media keys events.
  virtual void StopWatchingMediaKeys() = 0;
  // Whether listener started receiving media keys events.
  virtual bool IsWatchingMediaKeys() const = 0;
};

}  // namespace ui

#endif  // UI_BASE_ACCELERATORS_MEDIA_KEYS_LISTENER_H_
