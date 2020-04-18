// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_layer_tree_host_client.h"

#include "cc/test/fake_layer_tree_frame_sink.h"
#include "cc/trees/layer_tree_host.h"

namespace cc {

FakeLayerTreeHostClient::FakeLayerTreeHostClient() = default;
FakeLayerTreeHostClient::~FakeLayerTreeHostClient() = default;

void FakeLayerTreeHostClient::RequestNewLayerTreeFrameSink() {
  DCHECK(host_);
  host_->SetLayerTreeFrameSink(FakeLayerTreeFrameSink::Create3d());
}

void FakeLayerTreeHostClient::DidFailToInitializeLayerTreeFrameSink() {
  RequestNewLayerTreeFrameSink();
}

}  // namespace cc
