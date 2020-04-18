// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/viz/public/cpp/compositing/begin_frame_args_struct_traits.h"

#include "mojo/public/cpp/base/time_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<viz::mojom::BeginFrameArgsDataView, viz::BeginFrameArgs>::
    Read(viz::mojom::BeginFrameArgsDataView data, viz::BeginFrameArgs* out) {
  if (!data.ReadFrameTime(&out->frame_time) ||
      !data.ReadDeadline(&out->deadline) ||
      !data.ReadInterval(&out->interval)) {
    return false;
  }
  out->source_id = data.source_id();
  out->sequence_number = data.sequence_number();
  // TODO(eseckler): Use EnumTraits for |type|.
  out->type = static_cast<viz::BeginFrameArgs::BeginFrameArgsType>(data.type());
  out->on_critical_path = data.on_critical_path();
  out->animate_only = data.animate_only();
  return true;
}

// static
bool StructTraits<viz::mojom::BeginFrameAckDataView, viz::BeginFrameAck>::Read(
    viz::mojom::BeginFrameAckDataView data,
    viz::BeginFrameAck* out) {
  if (data.sequence_number() < viz::BeginFrameArgs::kStartingFrameNumber)
    return false;
  out->source_id = data.source_id();
  out->sequence_number = data.sequence_number();
  out->has_damage = data.has_damage();
  return true;
}

}  // namespace mojo
