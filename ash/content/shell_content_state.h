// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_CONTENT_SHELL_CONTENT_STATE_H_
#define ASH_CONTENT_SHELL_CONTENT_STATE_H_

#include "ash/content/ash_with_content_export.h"
#include "ash/content/screen_orientation_delegate_chromeos.h"
#include "ash/public/cpp/session_types.h"
#include "base/macros.h"


namespace aura {
class Window;
}

namespace content {
class BrowserContext;
}

namespace ash {

class ASH_WITH_CONTENT_EXPORT ShellContentState {
 public:
  static void SetInstance(ShellContentState* state);
  static ShellContentState* GetInstance();
  static void DestroyInstance();

  // Provides the embedder a way to return an active browser context for the
  // current user scenario. Default implementation here returns nullptr.
  virtual content::BrowserContext* GetActiveBrowserContext() = 0;

  // Returns the browser context for the user given by |index|.
  virtual content::BrowserContext* GetBrowserContextByIndex(
      UserIndex index) = 0;

  // Returns the browser context associated with the window.
  virtual content::BrowserContext* GetBrowserContextForWindow(
      aura::Window* window) = 0;

  // Returns the browser context on which the window is currently shown. NULL
  // means the window will be shown for every user.
  virtual content::BrowserContext* GetUserPresentingBrowserContextForWindow(
      aura::Window* window) = 0;

 protected:
  ShellContentState();
  virtual ~ShellContentState();

  ScreenOrientationDelegateChromeos orientation_delegate_;

 private:
  static ShellContentState* instance_;

  DISALLOW_COPY_AND_ASSIGN(ShellContentState);
};

}  // namespace ash

#endif  // ASH_CONTENT_SHELL_CONTENT_STATE_H_
