// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_MUS_PROPERTY_MIRROR_H_
#define UI_VIEWS_MUS_MUS_PROPERTY_MIRROR_H_

#include "ui/views/mus/mus_export.h"

namespace aura {
class Window;
}

namespace views {

// Facilitates copying mus client window properties to their mash frame windows.
class VIEWS_MUS_EXPORT MusPropertyMirror {
 public:
  virtual ~MusPropertyMirror() {}

  // Called when a property with the given |key| has changed for |window|.
  // |window| is what mus clients get when calling |widget->GetNativeWindow()|.
  // |root_window| is the top-level window representing the widget that is owned
  // by the window manager and that the window manager observes for changes.
  // Various ash features rely on property values of mus clients' root windows.
  virtual void MirrorPropertyFromWidgetWindowToRootWindow(
      aura::Window* window,
      aura::Window* root_window,
      const void* key) = 0;
};

}  // namespace views

#endif  // UI_VIEWS_MUS_MUS_PROPERTY_MIRROR_H_
