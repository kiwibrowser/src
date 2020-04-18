// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_TEST_HELPER_H_
#define SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_TEST_HELPER_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/optional.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "services/ui/ws2/ids.h"
#include "ui/gfx/geometry/rect.h"

namespace aura {
class Window;
}

namespace gfx {
class Insets;
}

namespace ui {

namespace mojom {
class WindowTree;

enum class EventTargetingPolicy;
}  // namespace mojom

namespace ws2 {

class Embedding;
class WindowServiceClient;

// Used for accessing private members of WindowServiceClient in tests.
class WindowServiceClientTestHelper {
 public:
  explicit WindowServiceClientTestHelper(
      WindowServiceClient* window_service_client);
  ~WindowServiceClientTestHelper();

  mojom::WindowTree* window_tree();

  mojom::WindowDataPtr WindowToWindowData(aura::Window* window);

  aura::Window* NewWindow(
      Id transport_window_id,
      base::flat_map<std::string, std::vector<uint8_t>> properties = {});
  void DeleteWindow(aura::Window* window);
  aura::Window* NewTopLevelWindow(
      Id transport_window_id,
      base::flat_map<std::string, std::vector<uint8_t>> properties = {});
  bool SetCapture(aura::Window* window);
  bool ReleaseCapture(aura::Window* window);
  bool SetWindowBounds(aura::Window* window, const gfx::Rect& bounds);
  // Same as SetWindowBounds(), but called in such a way that the ack
  // (OnChangeCompleted()) is called on the client.
  void SetWindowBoundsWithAck(aura::Window* window,
                              const gfx::Rect& bounds,
                              uint32_t change_id = 1);
  void SetClientArea(
      aura::Window* window,
      const gfx::Insets& insets,
      base::Optional<std::vector<gfx::Rect>> additional_client_areas =
          base::Optional<std::vector<gfx::Rect>>());
  void SetWindowProperty(aura::Window* window,
                         const std::string& name,
                         const std::vector<uint8_t>& value,
                         uint32_t change_id = 1);

  // Creates a new embedding. On success the new Embedding is returned. The
  // returned Embedding is owned by the ClientWindow for |window|.
  Embedding* Embed(aura::Window* window,
                   mojom::WindowTreeClientPtr client_ptr,
                   mojom::WindowTreeClient* client,
                   uint32_t embed_flags = 0);
  void SetEventTargetingPolicy(aura::Window* window,
                               mojom::EventTargetingPolicy policy);
  bool SetFocus(aura::Window* window);
  void SetCanFocus(aura::Window* window, bool can_focus);

  Id TransportIdForWindow(aura::Window* window);

  void DestroyEmbedding(Embedding* embedding);

 private:
  ClientWindowId ClientWindowIdForWindow(aura::Window* window);

  WindowServiceClient* window_service_client_;

  DISALLOW_COPY_AND_ASSIGN(WindowServiceClientTestHelper);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_TEST_HELPER_H_
