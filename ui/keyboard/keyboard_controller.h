// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_CONTROLLER_H_
#define UI_KEYBOARD_KEYBOARD_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "ui/aura/window_observer.h"
#include "ui/base/ime/input_method_keyboard_controller.h"
#include "ui/base/ime/input_method_observer.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/keyboard/container_behavior.h"
#include "ui/keyboard/container_type.h"
#include "ui/keyboard/display_util.h"
#include "ui/keyboard/keyboard_event_filter.h"
#include "ui/keyboard/keyboard_export.h"
#include "ui/keyboard/keyboard_layout_delegate.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/keyboard/notification_manager.h"
#include "ui/keyboard/queued_container_type.h"
#include "ui/keyboard/queued_display_change.h"

namespace aura {
class Window;
}
namespace ui {
class InputMethod;
class TextInputClient;
}

namespace keyboard {

class CallbackAnimationObserver;
class KeyboardControllerObserver;
class KeyboardUI;

// Represents the current state of the keyboard managed by the controller.
// Don't change the numeric value of the members because they are used in UMA
// - VirtualKeyboard.ControllerStateTransition.
// - VirtualKeyboard.LingeringIntermediateState
enum class KeyboardControllerState {
  UNKNOWN = 0,
  // Keyboard has never been shown.
  INITIAL = 1,
  // Waiting for an extension to be loaded. Will move to HIDDEN if this is
  // loading pre-emptively, otherwise will move to SHOWN.
  LOADING_EXTENSION = 2,
  // Keyboard is shown.
  SHOWN = 4,
  // Keyboard is still shown, but will move to HIDING in a short period, or if
  // an input element gets focused again, will move to SHOWN.
  WILL_HIDE = 5,
  // Keyboard is hidden, but has shown at least once.
  HIDDEN = 7,
  COUNT,
};

// Provides control of the virtual keyboard, including providing a container
// and controlling visibility.
class KEYBOARD_EXPORT KeyboardController
    : public ui::InputMethodObserver,
      public aura::WindowObserver,
      public ui::InputMethodKeyboardController {
 public:
  // Different ways to hide the keyboard.
  enum HideReason {
    // System initiated.
    HIDE_REASON_AUTOMATIC,
    // User initiated.
    HIDE_REASON_MANUAL,
  };

  KeyboardController(std::unique_ptr<KeyboardUI> ui,
                     KeyboardLayoutDelegate* delegate);
  ~KeyboardController() override;

  // Returns the container for the keyboard, which is owned by
  // KeyboardController. Creates the container if it's not already created.
  aura::Window* GetContainerWindow();

  // Same as GetContainerWindow except that this function doesn't create the
  // window.
  aura::Window* GetContainerWindowWithoutCreationForTest();

  // Whether the container window for the keyboard has been initialized.
  bool keyboard_container_initialized() const { return container_ != nullptr; }

  // Reloads the content of the keyboard. No-op if the keyboard content is not
  // loaded yet.
  void Reload();

  // Notifies the observer for contents bounds changed.
  void NotifyContentsBoundsChanging(const gfx::Rect& new_bounds);

  // Management of the observer list.
  void AddObserver(KeyboardControllerObserver* observer);
  bool HasObserver(KeyboardControllerObserver* observer) const;
  void RemoveObserver(KeyboardControllerObserver* observer);

  KeyboardUI* ui() { return ui_.get(); }

  void set_keyboard_locked(bool lock) { keyboard_locked_ = lock; }

  bool keyboard_locked() const { return keyboard_locked_; }

  // Immediately starts hiding animation of virtual keyboard and notifies
  // observers bounds change. This method forcibly sets keyboard_locked_
  // false while closing the keyboard.
  void HideKeyboard(HideReason reason);

  // Hides virtual keyboard if it's shown and not locked. This request can be
  // canceled by calling ShowKeyboard() or focusing another text input field
  // within certain period. This method is no-op if it's called when virtual
  // keyboard is hidden or it's locked.
  void MaybeHideKeyboard();

  // Force the keyboard to show up if not showing and lock the keyboard if
  // |lock| is true.
  void ShowKeyboard(bool lock);

  // Loads the keyboard UI contents in the background, but does not display
  // the keyboard.
  void LoadKeyboardUiInBackground();

  // Force the keyboard to show up in the specific display if not showing and
  // lock the keyboard
  void ShowKeyboardInDisplay(const display::Display& display);

  // Sets the active keyboard controller. KeyboardController takes ownership of
  // the instance. Calling ResetIntance with a new instance destroys the
  // previous one. May be called with nullptr to clear the instance.
  static void ResetInstance(KeyboardController* controller);

  // Retrieve the active keyboard controller.
  static KeyboardController* GetInstance();

  // Returns true if keyboard is in SHOWN or SHOWING state.
  bool keyboard_visible() const;

  // Returns true if keyboard window has been created.
  bool IsKeyboardWindowCreated();

  // Returns the bounds in screen for the visible portion of the keyboard. An
  // empty rectangle will get returned when the keyboard is hidden.
  const gfx::Rect& visual_bounds_in_screen() const {
    return visual_bounds_in_screen_;
  }

  // Returns the current bounds that affect the workspace layout. If the
  // keyboard is not shown or if the keyboard mode should not affect the usable
  // region of the screen, an empty rectangle will be returned.
  gfx::Rect GetWorkspaceOccludedBounds() const;

  // Returns the current bounds that affect the window layout of the various
  // lock screens.
  gfx::Rect GetKeyboardLockScreenOffsetBounds() const;

  KeyboardControllerState GetStateForTest() const { return state_; }

  ContainerType GetActiveContainerType() const {
    return container_behavior_->GetType();
  }

  gfx::Rect AdjustSetBoundsRequest(const gfx::Rect& display_bounds,
                                   const gfx::Rect& requested_bounds) const;

  // Returns true if overscroll is currently allowed by the active keyboard
  // container behavior.
  bool IsOverscrollAllowed() const;

  // Handle mouse and touch events on the keyboard. The effects of this method
  // will not stop propagation to the keyboard extension.
  bool HandlePointerEvent(const ui::LocatedEvent& event);

  // Moves an already loaded keyboard.
  void MoveKeyboard(const gfx::Rect& new_bounds);

  // Sets the active container type. If the keyboard is currently shown, this
  // will trigger a hide animation and a subsequent show animation. Otherwise
  // the ContainerBehavior change is synchronous.
  void SetContainerType(ContainerType type,
                        base::Optional<gfx::Rect> target_bounds,
                        base::OnceCallback<void(bool)> callback);

  // Sets floating keyboard draggable rect.
  bool SetDraggableArea(const gfx::Rect& rect);

  void MoveToDisplayWithTransition(display::Display display,
                                   gfx::Rect new_bounds_in_local);

  // InputMethodKeyboardController overrides.
  bool DisplayVirtualKeyboard() override;
  void DismissVirtualKeyboard() override;
  void AddObserver(
      ui::InputMethodKeyboardControllerObserver* observer) override;
  void RemoveObserver(
      ui::InputMethodKeyboardControllerObserver* observer) override;
  bool IsKeyboardVisible() override;

 private:
  // For access to Observer methods for simulation.
  friend class KeyboardControllerTest;

  // For access to SetContainerBounds.
  friend class KeyboardLayoutManager;

  // For access to NotifyKeyboardConfigChanged
  friend bool keyboard::UpdateKeyboardConfig(
      const keyboard::KeyboardConfig& config);

  // aura::WindowObserver overrides
  void OnWindowHierarchyChanged(const HierarchyChangeParams& params) override;
  void OnWindowAddedToRootWindow(aura::Window* window) override;
  void OnWindowRemovingFromRootWindow(aura::Window* window,
                                      aura::Window* new_root) override;
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;

  // InputMethodObserver overrides
  void OnBlur() override {}
  void OnCaretBoundsChanged(const ui::TextInputClient* client) override {}
  void OnFocus() override {}
  void OnInputMethodDestroyed(const ui::InputMethod* input_method) override {}
  void OnTextInputStateChanged(const ui::TextInputClient* client) override;
  void OnShowImeIfNeeded() override;

  // Sets the bounds of the container window. Shows the keyboard if contents
  // is first loaded and show_on_content_update_ is true. Called by
  // KeyboardLayoutManager.
  void SetContainerBounds(const gfx::Rect& new_bounds,
                          const bool contents_loaded);

  // Show virtual keyboard immediately with animation.
  void ShowKeyboardInternal(const display::Display& display);
  void PopulateKeyboardContent(const display::Display& display,
                               bool show_keyboard);

  // Returns true if keyboard is scheduled to hide.
  bool WillHideKeyboard() const;

  // Called when the hide animation finished.
  void HideAnimationFinished();
  // Called when the show animation finished.
  void ShowAnimationFinished();

  void NotifyKeyboardBoundsChangingAndEnsureCaretInWorkArea();

  // Called when the keyboard mode is set or the keyboard is moved to another
  // display.
  void AdjustKeyboardBounds();

  // Notifies keyboard config change to the observers.
  // Only called from |UpdateKeyboardConfig| in keyboard_util.
  void NotifyKeyboardConfigChanged();

  // Validates the state transition. Called from ChangeState.
  void CheckStateTransition(KeyboardControllerState prev,
                            KeyboardControllerState next);

  // Changes the current state and validates the transition.
  void ChangeState(KeyboardControllerState state);

  // Reports error histogram in case lingering in an intermediate state.
  void ReportLingeringState();

  // Shows the keyboard if the last time the keyboard was hidden was a small
  // time ago.
  void ShowKeyboardIfWithinTransientBlurThreshold();

  void SetContainerBehaviorInternal(ContainerType type);

  std::unique_ptr<KeyboardUI> ui_;
  KeyboardLayoutDelegate* layout_delegate_;
  std::unique_ptr<aura::Window> container_;
  // CallbackAnimationObserver should destructed before container_ because it
  // uses container_'s animator.
  std::unique_ptr<CallbackAnimationObserver> animation_observer_;

  // Current active visual behavior for the keyboard container.
  std::unique_ptr<ContainerBehavior> container_behavior_;

  std::unique_ptr<QueuedContainerType> queued_container_type_;
  std::unique_ptr<QueuedDisplayChange> queued_display_change_;

  // If true, show the keyboard window when keyboard UI content updates.
  bool show_on_content_update_;

  // If true, the keyboard is always visible even if no window has input focus.
  bool keyboard_locked_;
  KeyboardEventFilter event_filter_;

  base::ObserverList<KeyboardControllerObserver> observer_list_;

  // The bounds in screen for the visible portion of the keyboard.
  // If the contents window is visible, this should be the same size as the
  // contents window. If not, this should be empty.
  gfx::Rect visual_bounds_in_screen_;

  KeyboardControllerState state_;

  NotificationManager notification_manager_;

  base::Time time_of_last_blur_ = base::Time::UnixEpoch();

  DisplayUtil display_util_;

  static KeyboardController* instance_;

  base::WeakPtrFactory<KeyboardController> weak_factory_report_lingering_state_;
  base::WeakPtrFactory<KeyboardController> weak_factory_will_hide_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardController);
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_CONTROLLER_H_
