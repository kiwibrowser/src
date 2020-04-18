// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/modal_window_controller.h"

#include "base/stl_util.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_drawn_tracker.h"

namespace ui {
namespace ws {
namespace {

// This mirrors that of RootWindowController.
bool IsWindowAboveContainer(const ServerWindow* window,
                            const ServerWindow* blocking_container) {
  std::vector<const ServerWindow*> target_path;
  std::vector<const ServerWindow*> blocking_path;

  while (window) {
    target_path.push_back(window);
    window = window->parent();
  }

  while (blocking_container) {
    blocking_path.push_back(blocking_container);
    blocking_container = blocking_container->parent();
  }

  // The root window is put at the end so that we compare windows at
  // the same depth.
  while (!blocking_path.empty()) {
    if (target_path.empty())
      return false;

    const ServerWindow* target = target_path.back();
    target_path.pop_back();
    const ServerWindow* blocking = blocking_path.back();
    blocking_path.pop_back();

    // Still on the same path, continue.
    if (target == blocking)
      continue;

    // This can happen only if unparented window is passed because
    // first element must be the same root.
    if (!target->parent() || !blocking->parent())
      return false;

    const ServerWindow* common_parent = target->parent();
    DCHECK_EQ(common_parent, blocking->parent());
    const ServerWindow::Windows& windows = common_parent->children();
    auto blocking_iter = std::find(windows.begin(), windows.end(), blocking);
    // If the target window is above blocking window, the window can handle
    // events.
    return std::find(blocking_iter, windows.end(), target) != windows.end();
  }

  return true;
}

const ServerWindow* GetModalChildForWindowAncestor(const ServerWindow* window) {
  for (const ServerWindow* ancestor = window; ancestor;
       ancestor = ancestor->parent()) {
    for (auto* transient_child : ancestor->transient_children()) {
      if (transient_child->modal_type() != MODAL_TYPE_NONE &&
          transient_child->IsDrawn())
        return transient_child;
    }
  }
  return nullptr;
}

const ServerWindow* GetWindowModalTargetForWindow(const ServerWindow* window) {
  const ServerWindow* modal_window = GetModalChildForWindowAncestor(window);
  if (!modal_window)
    return window;
  return GetWindowModalTargetForWindow(modal_window);
}

// Returns true if |transient| is a valid modal window. |target_window| is
// the window being considered.
bool IsModalTransientChild(const ServerWindow* transient,
                           const ServerWindow* target_window) {
  return transient->IsDrawn() &&
         (transient->modal_type() == MODAL_TYPE_WINDOW ||
          transient->modal_type() == MODAL_TYPE_SYSTEM ||
          (transient->modal_type() == MODAL_TYPE_CHILD &&
           (transient->GetChildModalParent() &&
            transient->GetChildModalParent()->Contains(target_window))));
}

// Returns the deepest modal window starting at |activatable|. |target_window|
// is the window being considered.
const ServerWindow* GetModalTransientChild(const ServerWindow* activatable,
                                           const ServerWindow* target_window) {
  for (const ServerWindow* transient : activatable->transient_children()) {
    if (IsModalTransientChild(transient, target_window)) {
      if (transient->transient_children().empty())
        return transient;

      const ServerWindow* modal_child =
          GetModalTransientChild(transient, target_window);
      return modal_child ? modal_child : transient;
    }
  }
  return nullptr;
}

}  // namespace

class ModalWindowController::TrackedBlockingContainers
    : public ServerWindowObserver {
 public:
  TrackedBlockingContainers(ModalWindowController* modal_window_controller,
                            ServerWindow* system_modal_container,
                            ServerWindow* min_container)
      : modal_window_controller_(modal_window_controller),
        system_modal_container_(system_modal_container),
        min_container_(min_container) {
    DCHECK(system_modal_container_);
    system_modal_container_->AddObserver(this);
    if (min_container_)
      min_container_->AddObserver(this);
  }

  ~TrackedBlockingContainers() override {
    if (system_modal_container_)
      system_modal_container_->RemoveObserver(this);
    if (min_container_)
      min_container_->RemoveObserver(this);
  }

  bool IsInDisplayWithRoot(const ServerWindow* root) const {
    return root->Contains(system_modal_container_);
  }

  ServerWindow* system_modal_container() { return system_modal_container_; }

  ServerWindow* min_container() { return min_container_; }

 private:
  void Destroy() {
    modal_window_controller_->DestroyTrackedBlockingContainers(this);
  }

  // ServerWindowObserver:
  void OnWindowDestroying(ServerWindow* window) override {
    if (window == min_container_) {
      min_container_->RemoveObserver(this);
      min_container_ = nullptr;
      if (!system_modal_container_)
        Destroy();
    } else if (window == system_modal_container_) {
      system_modal_container_->RemoveObserver(this);
      system_modal_container_ = nullptr;
      // The |system_modal_container_| should always be valid.
      Destroy();
    }
  }

  ModalWindowController* modal_window_controller_;
  ServerWindow* system_modal_container_;
  ServerWindow* min_container_;

  DISALLOW_COPY_AND_ASSIGN(TrackedBlockingContainers);
};

ModalWindowController::ModalWindowController() {}

ModalWindowController::~ModalWindowController() {
  for (auto it = system_modal_windows_.begin();
       it != system_modal_windows_.end(); it++) {
    (*it)->RemoveObserver(this);
  }
}

void ModalWindowController::SetBlockingContainers(
    const std::vector<BlockingContainers>& all_blocking_containers) {
  all_blocking_containers_.clear();

  for (const BlockingContainers& containers : all_blocking_containers) {
    all_blocking_containers_.push_back(
        std::make_unique<TrackedBlockingContainers>(
            this, containers.system_modal_container, containers.min_container));
  }
}

void ModalWindowController::AddSystemModalWindow(ServerWindow* window) {
  DCHECK(window);
  DCHECK(!base::ContainsValue(system_modal_windows_, window));
  DCHECK_EQ(MODAL_TYPE_SYSTEM, window->modal_type());

  system_modal_windows_.push_back(window);
  window_drawn_trackers_.insert(make_pair(
      window, std::make_unique<ServerWindowDrawnTracker>(window, this)));
  window->AddObserver(this);
}

void ModalWindowController::DestroyTrackedBlockingContainers(
    TrackedBlockingContainers* containers) {
  for (auto iter = all_blocking_containers_.begin();
       iter != all_blocking_containers_.end(); ++iter) {
    if (iter->get() == containers) {
      all_blocking_containers_.erase(iter);
      return;
    }
  }
  NOTREACHED();
}

bool ModalWindowController::IsWindowBlocked(const ServerWindow* window) const {
  if (!window || !window->IsDrawn())
    return true;

  return GetModalTransient(window) ||
         IsWindowBlockedBySystemModalOrMinContainer(window);
}

const ServerWindow* ModalWindowController::GetModalTransient(
    const ServerWindow* window) const {
  if (!window)
    return nullptr;

  // We always want to check the for the transient child of the toplevel window.
  const ServerWindow* toplevel = GetToplevelWindow(window);
  if (!toplevel)
    return nullptr;

  return GetModalTransientChild(toplevel, window);
}

const ServerWindow* ModalWindowController::GetToplevelWindow(
    const ServerWindow* window) const {
  const ServerWindow* last = nullptr;
  for (const ServerWindow *w = window; w; last = w, w = w->parent()) {
    if (w->is_activation_parent())
      return last;
  }
  return nullptr;
}

const ServerWindow* ModalWindowController::GetActiveSystemModalWindow() const {
  for (auto it = system_modal_windows_.rbegin();
       it != system_modal_windows_.rend(); it++) {
    ServerWindow* modal = *it;
    if (modal->IsDrawn() && IsWindowInSystemModalContainer(modal))
      return modal;
  }
  return nullptr;
}

bool ModalWindowController::IsWindowBlockedBySystemModalOrMinContainer(
    const ServerWindow* window) const {
  const ServerWindow* system_modal_window = GetActiveSystemModalWindow();
  const ServerWindow* min_container = nullptr;
  if (system_modal_window) {
    // If there is a system modal window, then |window| must be part of the
    // system modal window.
    const bool is_part_of_active_modal =
        system_modal_window->Contains(window) ||
        window->HasTransientAncestor(system_modal_window);
    if (!is_part_of_active_modal)
      return true;

    // When there is a system modal window the |min_container| becomes the
    // system modal container.
    min_container = system_modal_window->parent();
  } else {
    min_container = GetMinContainer(window);
  }
  return min_container && !IsWindowAboveContainer(window, min_container);
}

bool ModalWindowController::IsWindowInSystemModalContainer(
    const ServerWindow* window) const {
  DCHECK(window->IsDrawn());
  const ServerWindow* root = window->GetRootForDrawn();
  DCHECK(root);
  for (auto& blocking_containers : all_blocking_containers_) {
    if (blocking_containers->IsInDisplayWithRoot(root))
      return window->parent() == blocking_containers->system_modal_container();
  }
  // This means the window manager didn't set the blocking containers, assume
  // the window is in a valid system modal container.
  return true;
}

const ServerWindow* ModalWindowController::GetMinContainer(
    const ServerWindow* window) const {
  DCHECK(window->IsDrawn());
  const ServerWindow* root = window->GetRootForDrawn();
  DCHECK(root);
  for (auto& blocking_containers : all_blocking_containers_) {
    if (blocking_containers->IsInDisplayWithRoot(root))
      return blocking_containers->min_container();
  }
  return nullptr;
}

void ModalWindowController::RemoveWindow(ServerWindow* window) {
  window->RemoveObserver(this);
  auto it = std::find(system_modal_windows_.begin(),
                      system_modal_windows_.end(), window);
  DCHECK(it != system_modal_windows_.end());
  system_modal_windows_.erase(it);
  window_drawn_trackers_.erase(window);
}

void ModalWindowController::OnWindowDestroyed(ServerWindow* window) {
  RemoveWindow(window);
}

void ModalWindowController::OnWindowModalTypeChanged(ServerWindow* window,
                                                     ModalType old_modal_type) {
  DCHECK_EQ(MODAL_TYPE_SYSTEM, old_modal_type);
  DCHECK_NE(MODAL_TYPE_SYSTEM, window->modal_type());
  RemoveWindow(window);
}

void ModalWindowController::OnDrawnStateChanged(ServerWindow* ancestor,
                                                ServerWindow* window,
                                                bool is_drawn) {
  if (!is_drawn)
    return;

  // Move the most recently shown window to the end of the list.
  auto it = std::find(system_modal_windows_.begin(),
                      system_modal_windows_.end(), window);
  DCHECK(it != system_modal_windows_.end());
  system_modal_windows_.splice(system_modal_windows_.end(),
                               system_modal_windows_, it);
}

}  // namespace ws
}  // namespace ui
