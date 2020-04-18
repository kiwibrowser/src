// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_MODAL_WINDOW_CONTROLLER_H_
#define SERVICES_UI_WS_MODAL_WINDOW_CONTROLLER_H_

#include <list>
#include <map>
#include <vector>

#include "services/ui/ws/server_window_drawn_tracker_observer.h"
#include "services/ui/ws/server_window_observer.h"

namespace ui {
namespace ws {

class ServerWindow;
class ServerWindowDrawnTracker;

namespace test {
class ModalWindowControllerTestApi;
}

// See mojom::BlockingContainers for details on this. |min_container| may
// be null.
struct BlockingContainers {
  ServerWindow* system_modal_container = nullptr;
  ServerWindow* min_container = nullptr;
};

// Used to keeps track of system modal windows and check whether windows are
// blocked by modal windows or not to do appropriate retargetting of events.
class ModalWindowController : public ServerWindowObserver,
                              public ServerWindowDrawnTrackerObserver {
 public:
  ModalWindowController();
  ~ModalWindowController() override;

  // See description in mojom::WindowManager::SetBlockingContainers() for
  // details on this.
  void SetBlockingContainers(
      const std::vector<BlockingContainers>& all_containers);

  // Adds a system modal window. The window remains modal to system until it is
  // destroyed. There can exist multiple system modal windows, in which case the
  // one that is visible and added most recently or shown most recently would be
  // the active one.
  void AddSystemModalWindow(ServerWindow* window);

  // Checks whether |window| is blocked by any visible modal window.
  bool IsWindowBlocked(const ServerWindow* window) const;

  // Returns the deepest modal window that is a transient descendants of the
  // top-level window for |window|.
  ServerWindow* GetModalTransient(ServerWindow* window) {
    return const_cast<ServerWindow*>(
        GetModalTransient(static_cast<const ServerWindow*>(window)));
  }
  const ServerWindow* GetModalTransient(const ServerWindow* window) const;

  ServerWindow* GetToplevelWindow(ServerWindow* window) {
    return const_cast<ServerWindow*>(
        GetToplevelWindow(static_cast<const ServerWindow*>(window)));
  }
  const ServerWindow* GetToplevelWindow(const ServerWindow* window) const;

  // Returns the system modal window that is visible and added/shown most
  // recently, if any.
  ServerWindow* GetActiveSystemModalWindow() {
    return const_cast<ServerWindow*>(
        const_cast<const ModalWindowController*>(this)
            ->GetActiveSystemModalWindow());
  }
  const ServerWindow* GetActiveSystemModalWindow() const;

 private:
  friend class test::ModalWindowControllerTestApi;
  class TrackedBlockingContainers;

  // Called when the window associated with a TrackedBlockingContainers is
  // destroyed.
  void DestroyTrackedBlockingContainers(TrackedBlockingContainers* containers);

  // Returns true if the there is a system modal window and |window| is not
  // associated with it, or |window| is not above the minimum container (if one
  // has been set).
  bool IsWindowBlockedBySystemModalOrMinContainer(
      const ServerWindow* window) const;

  // Returns true if |window| is in a container marked for system models.
  bool IsWindowInSystemModalContainer(const ServerWindow* window) const;

  const ServerWindow* GetMinContainer(const ServerWindow* window) const;

  // Removes |window| from the data structures used by this class.
  void RemoveWindow(ServerWindow* window);

  // ServerWindowObserver:
  void OnWindowDestroyed(ServerWindow* window) override;
  void OnWindowModalTypeChanged(ServerWindow* window,
                                ModalType old_modal_type) override;

  // ServerWindowDrawnTrackerObserver:
  void OnDrawnStateChanged(ServerWindow* ancestor,
                           ServerWindow* window,
                           bool is_drawn) override;

  // List of system modal windows in order they are added/shown.
  std::list<ServerWindow*> system_modal_windows_;

  // Drawn trackers for system modal windows.
  std::map<ServerWindow*, std::unique_ptr<ServerWindowDrawnTracker>>
      window_drawn_trackers_;

  std::vector<std::unique_ptr<TrackedBlockingContainers>>
      all_blocking_containers_;

  DISALLOW_COPY_AND_ASSIGN(ModalWindowController);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_MODAL_WINDOW_CONTROLLER_H_
