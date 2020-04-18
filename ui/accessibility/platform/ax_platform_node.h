// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_NODE_H_
#define UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_NODE_H_

#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/observer_list.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_export.h"
#include "ui/accessibility/ax_mode_observer.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

class AXPlatformNodeDelegate;

// AXPlatformNode is the abstract base class for an implementation of
// native accessibility APIs on supported platforms (e.g. Windows, Mac OS X).
// An object that wants to be accessible can derive from AXPlatformNodeDelegate
// and then call AXPlatformNode::Create. The delegate implementation should
// own the AXPlatformNode instance (or otherwise manage its lifecycle).
class AX_EXPORT AXPlatformNode {
 public:
  using NativeWindowHandlerCallback =
      base::RepeatingCallback<AXPlatformNode*(gfx::NativeWindow)>;

  // Create an appropriate platform-specific instance. The delegate owns the
  // AXPlatformNode instance (or manages its lifecycle in some other way).
  static AXPlatformNode* Create(AXPlatformNodeDelegate* delegate);

  // Cast a gfx::NativeViewAccessible to an AXPlatformNode if it is one,
  // or return nullptr if it's not an instance of this class.
  static AXPlatformNode* FromNativeViewAccessible(
      gfx::NativeViewAccessible accessible);

  // Return the AXPlatformNode at the root of the tree for a native window.
  static AXPlatformNode* FromNativeWindow(gfx::NativeWindow native_window);

  // Provide a function that returns the AXPlatformNode at the root of the
  // tree for a native window.
  static void RegisterNativeWindowHandler(NativeWindowHandlerCallback handler);

  // Register and unregister to receive notifications about AXMode changes
  // for this node.
  static void AddAXModeObserver(AXModeObserver* observer);
  static void RemoveAXModeObserver(AXModeObserver* observer);

  // Helper static function to notify all global observers about
  // the addition of an AXMode flag.
  static void NotifyAddAXModeFlags(AXMode mode_flags);

  static void OnAutofillShown();
  static void OnAutofillHidden();
  static bool IsAutofillShown();

  // Call Destroy rather than deleting this, because the subclass may
  // use reference counting.
  virtual void Destroy();

  // Get the platform-specific accessible object type for this instance.
  // On some platforms this is just a type cast, on others it may be a
  // wrapper object or handle.
  virtual gfx::NativeViewAccessible GetNativeViewAccessible() = 0;

  // Fire a platform-specific notification that an event has occurred on
  // this object.
  virtual void NotifyAccessibilityEvent(ax::mojom::Event event_type) = 0;

  // Return this object's delegate.
  virtual AXPlatformNodeDelegate* GetDelegate() const = 0;

  // Return the unique ID
  int32_t GetUniqueId() const;

 protected:
  AXPlatformNode();
  virtual ~AXPlatformNode();

 private:
  // Global ObserverList for AXMode changes.
  static base::LazyInstance<base::ObserverList<AXModeObserver>>::Leaky
      ax_mode_observers_;

  static base::LazyInstance<NativeWindowHandlerCallback>::Leaky
      native_window_handler_;

  static bool is_autofill_shown_;

  DISALLOW_COPY_AND_ASSIGN(AXPlatformNode);
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_NODE_H_
