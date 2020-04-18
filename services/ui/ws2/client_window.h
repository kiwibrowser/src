// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_CLIENT_WINDOW_H_
#define SERVICES_UI_WS2_CLIENT_WINDOW_H_

#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "services/ui/ws2/ids.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"

namespace aura {
class Window;
}

namespace ui {

class EventHandler;

namespace ws2 {

class Embedding;
class WindowServiceClient;

// Tracks any state associated with an aura::Window for the WindowService.
// ClientWindow is created for every window created at the request of a client,
// including the root window of ClientRoots.
class COMPONENT_EXPORT(WINDOW_SERVICE) ClientWindow {
 public:
  ~ClientWindow();

  // Creates a new ClientWindow. The lifetime of the ClientWindow is tied to
  // that of the Window (the Window ends up owning the ClientWindow).
  // |is_top_level| is true if the window represents a top-level window.
  static ClientWindow* Create(aura::Window* window,
                              WindowServiceClient* client,
                              const viz::FrameSinkId& frame_sink_id,
                              bool is_top_level);

  aura::Window* window() { return window_; }

  void set_can_focus(bool value) { can_focus_ = value; }
  bool can_focus() const { return can_focus_; }

  // Returns the ClientWindow associated with a window, null if not created yet.
  static ClientWindow* GetMayBeNull(aura::Window* window) {
    return const_cast<ClientWindow*>(
        GetMayBeNull(const_cast<const aura::Window*>(window)));
  }
  static const ClientWindow* GetMayBeNull(const aura::Window* window);

  WindowServiceClient* owning_window_service_client() {
    return owning_window_service_client_;
  }
  const WindowServiceClient* owning_window_service_client() const {
    return owning_window_service_client_;
  }

  WindowServiceClient* embedded_window_service_client();
  const WindowServiceClient* embedded_window_service_client() const;

  void set_frame_sink_id(const viz::FrameSinkId& frame_sink_id) {
    frame_sink_id_ = frame_sink_id;
  }
  const viz::FrameSinkId& frame_sink_id() const { return frame_sink_id_; }

  const std::vector<gfx::Rect>& additional_client_areas() const {
    return additional_client_areas_;
  }
  const gfx::Insets& client_area() const { return client_area_; }
  void SetClientArea(const gfx::Insets& insets,
                     const std::vector<gfx::Rect>& additional_client_areas);

  void set_capture_owner(WindowServiceClient* owner) { capture_owner_ = owner; }
  WindowServiceClient* capture_owner() const { return capture_owner_; }

  void set_focus_owner(WindowServiceClient* owner) { focus_owner_ = owner; }
  WindowServiceClient* focus_owner() const { return focus_owner_; }

  // Returns true if the window has an embedding, and the owning client
  // intercepts events that would normally target descendants.
  bool DoesOwnerInterceptEvents() const;

  // Returns true if this window has a client embedded in it.
  bool HasEmbedding() const { return embedding_.get() != nullptr; }
  void SetEmbedding(std::unique_ptr<Embedding> embedding);
  Embedding* embedding() { return embedding_.get(); }

  // Returns true if the window is a top-level window and there is at least some
  // non-client area.
  bool HasNonClientArea() const;

  bool IsTopLevel() const;

  void AttachCompositorFrameSink(
      viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
      viz::mojom::CompositorFrameSinkClientPtr client);

 private:
  friend class ClientWindowTestHelper;

  ClientWindow(aura::Window*,
               WindowServiceClient* client,
               const viz::FrameSinkId& frame_sink_id,
               bool is_top_level);

  // Forwards to TopLevelEventHandler, see it for details.
  // NOTE: this is only applicable to top-levels.
  bool IsInPointerPressedForTesting();

  aura::Window* window_;

  // Client that created the window. Null if the window was not created at the
  // request of a client. Generally this is null for first level embedding,
  // otherwise non-null. A first level embedding is one where local code
  // calls InitForEmbed() on a Window not associated with any other clients.
  WindowServiceClient* owning_window_service_client_;

  // Non-null if there is an embedding in this window.
  std::unique_ptr<Embedding> embedding_;

  // This is initially the id supplied by the client (for locally created
  // windows it is kWindowServerClientId for the high part and low part an ever
  // increasing number). If the window is used as the embed root, then it
  // changes to high part = id of client being embedded in and low part 0. If
  // used as a top-level, it's changed to the id passed by the client
  // requesting the top-level.
  viz::FrameSinkId frame_sink_id_;

  // Together |client_area_| and |additional_client_areas_| are used to specify
  // the client area. See SetClientArea() in mojom for details.
  gfx::Insets client_area_;
  std::vector<gfx::Rect> additional_client_areas_;

  std::unique_ptr<ui::EventHandler> event_handler_;

  // When a window has capture there are two possible clients that can get the
  // events, either the embedder or the embedded client. When |window_| has
  // capture this indicates which client gets the events. If null and |window_|
  // has capture, then events are not sent to a client and not handled by the
  // WindowService (meaning ui/events and aura's event processing continues).
  // For example, a mouse press in the non-client area of a top-level results
  // in views setting capture.
  WindowServiceClient* capture_owner_ = nullptr;

  // This serves the same purpose as |capture_owner_|, but is used for focus.
  // See |capture_owner_| for details.
  WindowServiceClient* focus_owner_ = nullptr;

  // True if the window is allowed to receive focus.
  // TODO(sky): provide way for client code to make use of this. It's needed
  // when client code creates the top-level. http://crbug.com/837703.
  bool can_focus_ = true;

  DISALLOW_COPY_AND_ASSIGN(ClientWindow);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_CLIENT_WINDOW_H_
