// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CHROME_KEYBOARD_UI_H_
#define CHROME_BROWSER_UI_ASH_CHROME_KEYBOARD_UI_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/media_stream_request.h"
#include "ui/aura/window_observer.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/keyboard/keyboard_controller_observer.h"
#include "ui/keyboard/keyboard_ui.h"

namespace {
class WindowBoundsChangeObserver;
}
namespace aura {
class Window;
}
namespace gfx {
class Rect;
}
namespace content {
class BrowserContext;
class WebContents;
}  // namespace content
namespace ui {
class InputMethod;
class Shadow;
}

// Subclass of KeyboardUI. It is used by KeyboardController to get
// access to the virtual keyboard window and setup Chrome extension functions.
class ChromeKeyboardUI : public keyboard::KeyboardUI,
                         public aura::WindowObserver,
                         public content::WebContentsObserver {
 public:
  class TestApi {
   public:
    // Use an empty |url| to clear the override.
    static void SetOverrideVirtualKeyboardUrl(const GURL& url);

   private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(TestApi);
  };

  explicit ChromeKeyboardUI(content::BrowserContext* context);
  ~ChromeKeyboardUI() override;

  // Requests the audio input from microphone for speech input.
  void RequestAudioInput(content::WebContents* web_contents,
                         const content::MediaStreamRequest& request,
                         const content::MediaResponseCallback& callback);

  // Called when a window being observed changes bounds, to update its insets.
  void UpdateInsetsForWindow(aura::Window* window);

  // Overridden from KeyboardUI:
  aura::Window* GetContentsWindow() override;
  bool HasContentsWindow() const override;
  bool ShouldWindowOverscroll(aura::Window* window) const override;
  void ReloadKeyboardIfNeeded() override;
  void InitInsets(const gfx::Rect& new_bounds) override;
  void ResetInsets() override;

 protected:
  // aura::WindowObserver overrides:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowDestroyed(aura::Window* window) override;
  void OnWindowParentChanged(aura::Window* window,
                             aura::Window* parent) override;

  content::BrowserContext* browser_context() { return browser_context_; }

  const aura::Window* GetKeyboardRootWindow() const;

  virtual std::unique_ptr<content::WebContents> CreateWebContents();

 private:
  friend class TestApi;

  // Loads the web contents for the given |url|.
  void LoadContents(const GURL& url);

  // Gets the virtual keyboard URL (either the default URL or IME override URL).
  const GURL& GetVirtualKeyboardUrl();

  // Determines whether a particular window should have insets for overscroll.
  bool ShouldEnableInsets(aura::Window* window);

  // Adds an observer for tracking changes to a window size or
  // position while the keyboard is displayed. Any window repositioning
  // invalidates insets for overscrolling.
  void AddBoundsChangedObserver(aura::Window* window);

  // Sets shadow around the keyboard. If shadow has not been created yet,
  // this method creates it.
  void SetShadowAroundKeyboard();

  // The implementation can choose to setup the WebContents before the virtual
  // keyboard page is loaded (e.g. install a WebContentsObserver).
  // SetupWebContents() is called right after creating the WebContents, before
  // loading the keyboard page.
  void SetupWebContents(content::WebContents* contents);

  // Overridden from KeyboardUI:
  ui::InputMethod* GetInputMethod() override;
  void SetController(keyboard::KeyboardController* controller) override;
  void ShowKeyboardContainer(aura::Window* container) override;

  // content::WebContentsObserver overrides
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;

  // The BrowserContext to use for creating the WebContents hosting the
  // keyboard.
  content::BrowserContext* const browser_context_;

  const GURL default_url_;

  std::unique_ptr<content::WebContents> keyboard_contents_;
  std::unique_ptr<ui::Shadow> shadow_;

  std::unique_ptr<keyboard::KeyboardControllerObserver> observer_;
  std::unique_ptr<WindowBoundsChangeObserver> window_bounds_observer_;

  DISALLOW_COPY_AND_ASSIGN(ChromeKeyboardUI);
};

#endif  // CHROME_BROWSER_UI_ASH_CHROME_KEYBOARD_UI_H_
