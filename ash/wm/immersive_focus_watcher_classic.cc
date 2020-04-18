// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/immersive_focus_watcher_classic.h"

#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ui/aura/window.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/transient_window_manager.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/activation_client.h"

namespace ash {
namespace {

// Returns the BubbleDialogDelegateView corresponding to |maybe_bubble| if
// |maybe_bubble| is a bubble.
views::BubbleDialogDelegateView* AsBubbleDialogDelegate(
    aura::Window* maybe_bubble) {
  if (!maybe_bubble)
    return nullptr;
  views::Widget* widget = views::Widget::GetWidgetForNativeView(maybe_bubble);
  if (!widget)
    return nullptr;
  return widget->widget_delegate()->AsBubbleDialogDelegate();
}

views::View* GetAnchorView(aura::Window* maybe_bubble) {
  views::BubbleDialogDelegateView* bubble_dialog =
      AsBubbleDialogDelegate(maybe_bubble);
  return bubble_dialog ? bubble_dialog->GetAnchorView() : nullptr;
}

// Returns true if |maybe_transient| is a transient child of |toplevel|.
bool IsWindowTransientChildOf(aura::Window* maybe_transient,
                              aura::Window* toplevel) {
  if (!maybe_transient || !toplevel)
    return false;

  for (aura::Window* window = maybe_transient; window;
       window = ::wm::GetTransientParent(window)) {
    if (window == toplevel)
      return true;
  }
  return false;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

// Class which keeps the top-of-window views revealed as long as one of the
// bubbles it is observing is visible. The logic to keep the top-of-window
// views revealed based on the visibility of bubbles anchored to
// children of |ImmersiveFullscreenController::top_container_| is separate from
// the logic related to |ImmersiveFullscreenController::focus_revealed_lock_|
// so that bubbles which are not activatable and bubbles which do not close
// upon deactivation also keep the top-of-window views revealed for the
// duration of their visibility.
class ImmersiveFocusWatcherClassic::BubbleObserver
    : public aura::WindowObserver {
 public:
  explicit BubbleObserver(ImmersiveFullscreenController* controller);
  ~BubbleObserver() override;

  // Start / stop observing changes to |bubble|'s visibility.
  void StartObserving(aura::Window* bubble);
  void StopObserving(aura::Window* bubble);

 private:
  // Updates |revealed_lock_| based on whether any of |bubbles_| is visible.
  void UpdateRevealedLock();

  // aura::WindowObserver overrides:
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override;
  void OnWindowDestroying(aura::Window* window) override;

  ImmersiveFullscreenController* controller_;

  std::set<aura::Window*> bubbles_;

  // Lock which keeps the top-of-window views revealed based on whether any of
  // |bubbles_| is visible.
  std::unique_ptr<ImmersiveRevealedLock> revealed_lock_;

  DISALLOW_COPY_AND_ASSIGN(BubbleObserver);
};

ImmersiveFocusWatcherClassic::BubbleObserver::BubbleObserver(
    ImmersiveFullscreenController* controller)
    : controller_(controller) {}

ImmersiveFocusWatcherClassic::BubbleObserver::~BubbleObserver() {
  for (aura::Window* bubble : bubbles_)
    bubble->RemoveObserver(this);
}

void ImmersiveFocusWatcherClassic::BubbleObserver::StartObserving(
    aura::Window* bubble) {
  if (bubbles_.insert(bubble).second) {
    bubble->AddObserver(this);
    UpdateRevealedLock();
  }
}

void ImmersiveFocusWatcherClassic::BubbleObserver::StopObserving(
    aura::Window* bubble) {
  if (bubbles_.erase(bubble)) {
    bubble->RemoveObserver(this);
    UpdateRevealedLock();
  }
}

void ImmersiveFocusWatcherClassic::BubbleObserver::UpdateRevealedLock() {
  bool has_visible_bubble = false;
  for (aura::Window* bubble : bubbles_) {
    if (bubble->IsVisible()) {
      has_visible_bubble = true;
      break;
    }
  }

  bool was_revealed = controller_->IsRevealed();
  if (has_visible_bubble) {
    if (!revealed_lock_.get()) {
      // Reveal the top-of-window views without animating because it looks
      // weird for the top-of-window views to animate and the bubble not to
      // animate along with the top-of-window views.
      revealed_lock_.reset(controller_->GetRevealedLock(
          ImmersiveFullscreenController::ANIMATE_REVEAL_NO));
    }
  } else {
    revealed_lock_.reset();
  }

  if (!was_revealed && revealed_lock_.get()) {
    // Currently, there is no nice way for bubbles to reposition themselves
    // whenever the anchor view moves. Tell the bubbles to reposition themselves
    // explicitly instead. The hidden bubbles are also repositioned because
    // BubbleDialogDelegateView does not reposition its widget as a result of a
    // visibility change.
    for (aura::Window* bubble : bubbles_)
      AsBubbleDialogDelegate(bubble)->OnAnchorBoundsChanged();
  }
}

void ImmersiveFocusWatcherClassic::BubbleObserver::OnWindowVisibilityChanged(
    aura::Window*,
    bool visible) {
  UpdateRevealedLock();
}

void ImmersiveFocusWatcherClassic::BubbleObserver::OnWindowDestroying(
    aura::Window* window) {
  StopObserving(window);
}

ImmersiveFocusWatcherClassic::ImmersiveFocusWatcherClassic(
    ImmersiveFullscreenController* controller)
    : immersive_fullscreen_controller_(controller) {
  GetWidget()->GetFocusManager()->AddFocusChangeListener(this);
  GetWidget()->AddObserver(this);
  ::wm::TransientWindowManager::GetOrCreate(GetWidgetWindow())
      ->AddObserver(this);
  RecreateBubbleObserver();
}

ImmersiveFocusWatcherClassic::~ImmersiveFocusWatcherClassic() {
  ::wm::TransientWindowManager::GetOrCreate(GetWidgetWindow())
      ->RemoveObserver(this);
  GetWidget()->GetFocusManager()->RemoveFocusChangeListener(this);
  GetWidget()->RemoveObserver(this);
}

void ImmersiveFocusWatcherClassic::UpdateFocusRevealedLock() {
  views::Widget* widget = GetWidget();
  views::View* top_container =
      immersive_fullscreen_controller_->top_container();
  bool hold_lock = false;
  if (widget->IsActive()) {
    views::View* focused_view = widget->GetFocusManager()->GetFocusedView();
    if (top_container->Contains(focused_view))
      hold_lock = true;
  } else {
    aura::Window* native_window = widget->GetNativeWindow();
    aura::Window* active_window =
        ::wm::GetActivationClient(native_window->GetRootWindow())
            ->GetActiveWindow();
    if (GetAnchorView(active_window)) {
      // BubbleObserver will already have locked the top-of-window views if the
      // bubble is anchored to a child of |top_container|. Don't acquire
      // |lock_| here for the sake of simplicity.
      // Note: Instead of checking for the existence of the |anchor_view|,
      // the existence of the |anchor_widget| is performed to avoid the case
      // where the view is already gone (and the widget is still running).
    } else {
      // The currently active window is not |native_window| and it is not a
      // bubble with an anchor view. The top-of-window views should be revealed
      // if:
      // 1) The active window is a transient child of |native_window|.
      // 2) The top-of-window views are already revealed. This restriction
      //    prevents a transient window opened by the web contents while the
      //    top-of-window views are hidden from from initiating a reveal.
      // The top-of-window views will stay revealed till |native_window| is
      // reactivated.
      if (immersive_fullscreen_controller_->IsRevealed() &&
          IsWindowTransientChildOf(active_window, native_window)) {
        hold_lock = true;
      }
    }
  }

  if (hold_lock) {
    if (!lock_.get()) {
      lock_.reset(immersive_fullscreen_controller_->GetRevealedLock(
          ImmersiveFullscreenController::ANIMATE_REVEAL_YES));
    }
  } else {
    lock_.reset();
  }
}

void ImmersiveFocusWatcherClassic::ReleaseLock() {
  lock_.reset();
}

views::Widget* ImmersiveFocusWatcherClassic::GetWidget() {
  return immersive_fullscreen_controller_->widget();
}

aura::Window* ImmersiveFocusWatcherClassic::GetWidgetWindow() {
  return GetWidget()->GetNativeWindow();
}

void ImmersiveFocusWatcherClassic::RecreateBubbleObserver() {
  bubble_observer_.reset(new BubbleObserver(immersive_fullscreen_controller_));
  const std::vector<aura::Window*> transient_children =
      ::wm::GetTransientChildren(GetWidgetWindow());
  for (size_t i = 0; i < transient_children.size(); ++i) {
    aura::Window* transient_child = transient_children[i];
    views::View* anchor_view = GetAnchorView(transient_child);
    if (anchor_view &&
        immersive_fullscreen_controller_->top_container()->Contains(
            anchor_view))
      bubble_observer_->StartObserving(transient_child);
  }
}

void ImmersiveFocusWatcherClassic::OnWillChangeFocus(
    views::View* focused_before,
    views::View* focused_now) {}

void ImmersiveFocusWatcherClassic::OnDidChangeFocus(views::View* focused_before,
                                                    views::View* focused_now) {
  UpdateFocusRevealedLock();
}

void ImmersiveFocusWatcherClassic::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {
  UpdateFocusRevealedLock();
}

void ImmersiveFocusWatcherClassic::OnTransientChildAdded(
    aura::Window* window,
    aura::Window* transient) {
  views::View* anchor = GetAnchorView(transient);
  if (anchor &&
      immersive_fullscreen_controller_->top_container()->Contains(anchor)) {
    // Observe the aura::Window because the BubbleDelegateView may not be
    // parented to the widget's root view yet so |bubble_delegate->GetWidget()|
    // may still return NULL.
    bubble_observer_->StartObserving(transient);
  }
}

void ImmersiveFocusWatcherClassic::OnTransientChildRemoved(
    aura::Window* window,
    aura::Window* transient) {
  bubble_observer_->StopObserving(transient);
}

}  // namespace ash
