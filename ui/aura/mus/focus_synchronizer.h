// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_FOCUS_SYNCHRONIZER_H_
#define UI_AURA_MUS_FOCUS_SYNCHRONIZER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/aura/aura_export.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/mus/focus_synchronizer_observer.h"
#include "ui/aura/window_observer.h"

namespace ui {
namespace mojom {
class WindowTree;
}
}

namespace aura {

class FocusSynchronizerDelegate;
class WindowMus;

namespace client {
class FocusClient;
}

// FocusSynchronizer is responsible for keeping focus in sync between aura
// and the mus server. FocusSynchronizer may be configured in two distinct
// ways:
// . SetSingletonFocusClient(). Use this when a single FocusClient is shared
//   among all windows and never changes.
// . SetActiveFocusClient(). Use this when there may be more than one
//   FocusClient.
class AURA_EXPORT FocusSynchronizer : public client::FocusChangeObserver,
                                      public WindowObserver {
 public:
  FocusSynchronizer(FocusSynchronizerDelegate* delegate,
                    ui::mojom::WindowTree* window_tree);
  ~FocusSynchronizer() override;

  client::FocusClient* active_focus_client() { return active_focus_client_; }
  Window* active_focus_client_root() { return active_focus_client_root_; }
  WindowMus* focused_window() { return focused_window_; }

  // Add and remove observers to the FocusSynchronizer to get notified when the
  // |active_focus_client_| and the |active_focus_client_root_| change.
  void AddObserver(FocusSynchronizerObserver* observer);
  void RemoveObserver(FocusSynchronizerObserver* observer);

  // Called when the server side wants to change focus to |window|.
  void SetFocusFromServer(WindowMus* window);

  // Called when the focused window is destroyed.
  void OnFocusedWindowDestroyed();

  // Used when the focus client is shared among all windows. See class
  // description for details.
  void SetSingletonFocusClient(client::FocusClient* focus_client);
  bool is_singleton_focus_client() const { return is_singleton_focus_client_; }

  // Sets the active FocusClient and the window the FocusClient is associated
  // with. |focus_client_root| is not necessarily the window that actually has
  // focus.
  void SetActiveFocusClient(client::FocusClient* focus_client,
                            Window* focus_client_root);

 private:
  // Called internally to set |active_focus_client_| and update observer.
  void SetActiveFocusClientInternal(client::FocusClient* focus_client);

  // Called internally to set |focused_window_| and update the server.
  void SetFocusedWindow(WindowMus* window);

  // Called internally when notified that |active_focus_client_| and
  // |active_focus_client_root_| have been changed.
  void OnActiveFocusClientChanged(client::FocusClient* focus_client,
                                  Window* focus_client_root);

  // client::FocusChangeObserver:
  void OnWindowFocused(Window* gained_focus, Window* lost_focus) override;

  // WindowObserver:
  void OnWindowDestroying(Window* window) override;
  void OnWindowPropertyChanged(Window* window,
                               const void* key,
                               intptr_t old) override;

  FocusSynchronizerDelegate* delegate_;
  ui::mojom::WindowTree* window_tree_;

  base::ObserverList<FocusSynchronizerObserver> observers_;

  bool setting_focus_ = false;
  WindowMus* window_setting_focus_to_ = nullptr;

  bool is_singleton_focus_client_ = false;

  client::FocusClient* active_focus_client_ = nullptr;
  // The window that |active_focus_client_| is associated with.
  Window* active_focus_client_root_ = nullptr;
  // The window that actually has focus.
  WindowMus* focused_window_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(FocusSynchronizer);
};

}  // namespace aura

#endif  // UI_AURA_MUS_FOCUS_SYNCHRONIZER_H_
