// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/mus/mus_embedded_frame.h"

#include <map>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "cc/base/switches.h"
#include "components/viz/client/client_layer_tree_frame_sink.h"
#include "components/viz/client/hit_test_data_provider.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "content/renderer/mus/renderer_window_tree_client.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"

namespace content {
namespace {

// Callback from embedding a child frame.
void OnEmbedAck(bool success) {
  DCHECK(success);
}

}  // namespace

MusEmbeddedFrame::~MusEmbeddedFrame() {
  renderer_window_tree_client_->OnEmbeddedFrameDestroyed(this);

  // If the tree changed the old tree the window was created in was dropped and
  // the server implictly cleaned up. As such, no need to do cleanup here (and
  // we don't have a handle to the right WindowTree at this point).
  if (tree_changed_)
    return;

  // If there is |pending_state_| it means we didn't actually create the window
  // yet and there is nothing to do.
  if (pending_state_)
    return;

  window_tree()->DeleteWindow(GetAndAdvanceNextChangeId(), window_id_);
}

void MusEmbeddedFrame::SetWindowBounds(
    const viz::LocalSurfaceId& local_surface_id,
    const gfx::Rect& bounds) {
  if (tree_changed_)
    return;

  if (!window_tree()) {
    DCHECK(pending_state_);
    pending_state_->bounds = bounds;
    pending_state_->local_surface_id = local_surface_id;
    pending_state_->was_set_window_bounds_called = true;
    return;
  }

  window_tree()->SetWindowBounds(GetAndAdvanceNextChangeId(), window_id_,
                                 bounds, local_surface_id);
}

MusEmbeddedFrame::MusEmbeddedFrame(
    RendererWindowTreeClient* renderer_window_tree_client,
    MusEmbeddedFrameDelegate* delegate,
    ui::ClientSpecificId window_id,
    const base::UnguessableToken& token)
    : renderer_window_tree_client_(renderer_window_tree_client),
      delegate_(delegate),
      window_id_(window_id) {
  if (!window_tree()) {
    pending_state_ = std::make_unique<PendingState>();
    pending_state_->token = token;
    return;
  }
  CreateChildWindowAndEmbed(token);
}

void MusEmbeddedFrame::CreateChildWindowAndEmbed(
    const base::UnguessableToken& token) {
  // Set a name for debugging.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  properties[ui::mojom::WindowManager::kName_Property] =
      mojo::ConvertTo<std::vector<uint8_t>>(std::string("RendererFrame"));
  window_tree()->NewWindow(GetAndAdvanceNextChangeId(), window_id_, properties);
  window_tree()->AddWindow(GetAndAdvanceNextChangeId(),
                           renderer_window_tree_client_->root_window_id_,
                           window_id_);
  window_tree()->EmbedUsingToken(window_id_, token, 0,
                                 base::BindOnce(&OnEmbedAck));
}

void MusEmbeddedFrame::OnTreeAvailable() {
  // See header for description, always called before OnTreeWillChange().
  DCHECK(!tree_changed_);
  std::unique_ptr<PendingState> pending_state = std::move(pending_state_);
  CreateChildWindowAndEmbed(pending_state->token);
  if (pending_state->was_set_window_bounds_called)
    SetWindowBounds(pending_state->local_surface_id, pending_state->bounds);
}

void MusEmbeddedFrame::OnTreeWillChange() {
  tree_changed_ = true;
}

uint32_t MusEmbeddedFrame::GetAndAdvanceNextChangeId() {
  return renderer_window_tree_client_->GetAndAdvanceNextChangeId();
}

ui::mojom::WindowTree* MusEmbeddedFrame::window_tree() {
  // Once |tree_changed_| is true the WindowTree this instance used has changed
  // and it no longer makes sense to use it (the original window was deleted).
  return tree_changed_ ? nullptr : renderer_window_tree_client_->tree_.get();
}

MusEmbeddedFrame::PendingState::PendingState() = default;

MusEmbeddedFrame::PendingState::~PendingState() = default;

}  // namespace content
