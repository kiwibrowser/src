// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_WAYLAND_SERVER_H_
#define COMPONENTS_EXO_WAYLAND_SERVER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "components/exo/wayland/scoped_wl.h"
#include "ui/display/display_observer.h"

struct wl_global;

namespace exo {
class Display;

namespace wayland {

// This class is a thin wrapper around a Wayland display server. All Wayland
// requests are dispatched into the given Exosphere display.
class Server : public display::DisplayObserver {
 public:
  // Class that represent a wayland output. Tied to a specific display ID
  // and associated with a global.
  class Output {
   public:
    explicit Output(int64_t id);
    ~Output();

    int64_t id() const { return id_; }
    void set_global(wl_global* global) { global_ = global; }

   private:
    const int64_t id_;
    wl_global* global_ = nullptr;

    DISALLOW_COPY_AND_ASSIGN(Output);
  };

  explicit Server(Display* display);
  ~Server() override;

  // Creates a Wayland display server that clients can connect to using the
  // default socket name.
  static std::unique_ptr<Server> Create(Display* display);

  // This adds a Unix socket to the Wayland display server which can be used
  // by clients to connect to the display server.
  bool AddSocket(const std::string name);

  // Returns the file descriptor associated with the server.
  int GetFileDescriptor() const;

  // This function dispatches events. This must be called on a thread for
  // which it's safe to access the Exosphere display that this server was
  // created for. The |timeout| argument specifies the amount of time that
  // Dispatch() should block waiting for the file descriptor to become ready.
  void Dispatch(base::TimeDelta timeout);

  // Send all buffered events to the clients.
  void Flush();

  // Overridden from display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;

 private:
  Display* const display_;
  std::unique_ptr<wl_display, WlDisplayDeleter> wl_display_;
  base::flat_map<int64_t, std::unique_ptr<Output>> outputs_;

  DISALLOW_COPY_AND_ASSIGN(Server);
};

}  // namespace wayland
}  // namespace exo

#endif  // COMPONENTS_EXO_WAYLAND_SERVER_H_
