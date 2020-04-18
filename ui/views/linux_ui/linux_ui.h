// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_LINUX_UI_LINUX_UI_H_
#define UI_VIEWS_LINUX_UI_LINUX_UI_H_

#include <string>

#include "base/callback.h"
#include "build/buildflag.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ime/linux/linux_input_method_context_factory.h"
#include "ui/base/ime/linux/text_edit_key_bindings_delegate_auralinux.h"
#include "ui/gfx/linux_font_delegate.h"
#include "ui/shell_dialogs/shell_dialog_linux.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/features.h"
#include "ui/views/linux_ui/status_icon_linux.h"
#include "ui/views/views_export.h"

// The main entrypoint into Linux toolkit specific code. GTK code should only
// be executed behind this interface.

namespace aura {
class Window;
}

namespace base {
class TimeDelta;
}

namespace color_utils {
struct HSL;
}

namespace gfx {
class Image;
}

namespace ui {
class NativeTheme;
}

namespace views {
class Border;
class DeviceScaleFactorObserver;
class LabelButton;
class LabelButtonBorder;
class WindowButtonOrderObserver;

#if BUILDFLAG(ENABLE_NATIVE_WINDOW_NAV_BUTTONS)
class NavButtonProvider;
#endif

// Adapter class with targets to render like different toolkits. Set by any
// project that wants to do linux desktop native rendering.
//
// TODO(erg): We're hardcoding GTK2, when we'll need to have backends for (at
// minimum) GTK2 and GTK3. LinuxUI::instance() should actually be a very
// complex method that pokes around with dlopen against a libuigtk2.so, a
// liuigtk3.so, etc.
class VIEWS_EXPORT LinuxUI : public ui::LinuxInputMethodContextFactory,
                             public gfx::LinuxFontDelegate,
                             public ui::ShellDialogLinux,
                             public ui::TextEditKeyBindingsDelegateAuraLinux {
 public:
  // Describes the window management actions that could be taken in response to
  // a middle click in the non client area.
  enum NonClientWindowFrameAction {
    WINDOW_FRAME_ACTION_NONE,
    WINDOW_FRAME_ACTION_LOWER,
    WINDOW_FRAME_ACTION_MINIMIZE,
    WINDOW_FRAME_ACTION_TOGGLE_MAXIMIZE,
    WINDOW_FRAME_ACTION_MENU,
  };

  // The types of clicks that might invoke a NonClientWindowFrameAction.
  enum NonClientWindowFrameActionSourceType {
    WINDOW_FRAME_ACTION_SOURCE_DOUBLE_CLICK = 0,
    WINDOW_FRAME_ACTION_SOURCE_MIDDLE_CLICK,
    WINDOW_FRAME_ACTION_SOURCE_RIGHT_CLICK,

    WINDOW_FRAME_ACTION_SOURCE_LAST
  };

  typedef base::Callback<ui::NativeTheme*(aura::Window* window)>
      NativeThemeGetter;

  ~LinuxUI() override {}

  // Sets the dynamically loaded singleton that draws the desktop native UI.
  static void SetInstance(LinuxUI* instance);

  // Returns a LinuxUI instance for the toolkit used in the user's desktop
  // environment.
  //
  // Can return NULL, in case no toolkit has been set. (For example, if we're
  // running with the "--ash" flag.)
  static LinuxUI* instance();

  virtual void Initialize() = 0;
  virtual bool GetTint(int id, color_utils::HSL* tint) const = 0;
  virtual bool GetColor(int id, SkColor* color) const = 0;

  // Returns the preferences that we pass to WebKit.
  virtual SkColor GetFocusRingColor() const = 0;
  virtual SkColor GetThumbActiveColor() const = 0;
  virtual SkColor GetThumbInactiveColor() const = 0;
  virtual SkColor GetTrackColor() const = 0;
  virtual SkColor GetActiveSelectionBgColor() const = 0;
  virtual SkColor GetActiveSelectionFgColor() const = 0;
  virtual SkColor GetInactiveSelectionBgColor() const = 0;
  virtual SkColor GetInactiveSelectionFgColor() const = 0;
  virtual base::TimeDelta GetCursorBlinkInterval() const = 0;

  // Returns a NativeTheme that will provide system colors and draw system
  // style widgets.
  virtual ui::NativeTheme* GetNativeTheme(aura::Window* window) const = 0;

  // Used to set an override NativeTheme.
  virtual void SetNativeThemeOverride(const NativeThemeGetter& callback) = 0;

  // Returns whether we should be using the native theme provided by this
  // object by default.
  virtual bool GetDefaultUsesSystemTheme() const = 0;

  // Sets visual properties in the desktop environment related to download
  // progress, if available.
  virtual void SetDownloadCount(int count) const = 0;
  virtual void SetProgressFraction(float percentage) const = 0;

  // Checks for platform support for status icons.
  virtual bool IsStatusIconSupported() const = 0;

  // Create a native status icon.
  virtual std::unique_ptr<StatusIconLinux> CreateLinuxStatusIcon(
      const gfx::ImageSkia& image,
      const base::string16& tool_tip) const = 0;

  // Returns the icon for a given content type from the icon theme.
  // TODO(davidben): Add an observer for the theme changing, so we can drop the
  // caches.
  virtual gfx::Image GetIconForContentType(
      const std::string& content_type, int size) const = 0;

  // Builds a Border which paints the native button style.
  virtual std::unique_ptr<Border> CreateNativeBorder(
      views::LabelButton* owning_button,
      std::unique_ptr<views::LabelButtonBorder> border) = 0;

  // Notifies the observer about changes about how window buttons should be
  // laid out. If the order is anything other than the default min,max,close on
  // the right, will immediately send a button change event to the observer.
  virtual void AddWindowButtonOrderObserver(
      WindowButtonOrderObserver* observer) = 0;

  // Removes the observer from the LinuxUI's list.
  virtual void RemoveWindowButtonOrderObserver(
      WindowButtonOrderObserver* observer) = 0;

  // What action we should take when the user clicks on the non-client area.
  // |source| describes the type of click.
  virtual NonClientWindowFrameAction GetNonClientWindowFrameAction(
      NonClientWindowFrameActionSourceType source) = 0;

  // Notifies the window manager that start up has completed.
  // Normally Chromium opens a new window on startup and GTK does this
  // automatically. In case Chromium does not open a new window on startup,
  // e.g. an existing browser window already exists, this should be called.
  virtual void NotifyWindowManagerStartupComplete() = 0;

  // Updates the device scale factor so that the default font size can be
  // recalculated.
  virtual void UpdateDeviceScaleFactor() = 0;

  // Determines the device scale factor of the primary screen.
  virtual float GetDeviceScaleFactor() const = 0;

  // Registers |observer| to be notified about changes to the device
  // scale factor.
  virtual void AddDeviceScaleFactorObserver(
      DeviceScaleFactorObserver* observer) = 0;

  // Unregisters |observer| from receiving changes to the device scale
  // factor.
  virtual void RemoveDeviceScaleFactorObserver(
      DeviceScaleFactorObserver* observer) = 0;

  // Only used on GTK to indicate if the dark GTK theme variant is
  // preferred.
  virtual bool PreferDarkTheme() const = 0;

#if BUILDFLAG(ENABLE_NATIVE_WINDOW_NAV_BUTTONS)
  // Returns a new NavButtonProvider, or nullptr if the underlying
  // toolkit does not support drawing client-side navigation buttons.
  virtual std::unique_ptr<NavButtonProvider> CreateNavButtonProvider() = 0;
#endif

  // Returns a map of KeyboardEvent code to KeyboardEvent key values.
  virtual base::flat_map<std::string, std::string> GetKeyboardLayoutMap() = 0;
};

}  // namespace views

#endif  // UI_VIEWS_LINUX_UI_LINUX_UI_H_
