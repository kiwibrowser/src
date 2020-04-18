// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_BEGIN_FRAME_ARGS_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_BEGIN_FRAME_ARGS_STRUCT_TRAITS_H_

#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "services/viz/public/interfaces/compositing/begin_frame_args.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<viz::mojom::BeginFrameArgsDataView, viz::BeginFrameArgs> {
  static base::TimeTicks frame_time(const viz::BeginFrameArgs& args) {
    return args.frame_time;
  }

  static base::TimeTicks deadline(const viz::BeginFrameArgs& args) {
    return args.deadline;
  }

  static base::TimeDelta interval(const viz::BeginFrameArgs& args) {
    return args.interval;
  }

  static uint64_t sequence_number(const viz::BeginFrameArgs& args) {
    return args.sequence_number;
  }

  static uint64_t source_id(const viz::BeginFrameArgs& args) {
    return args.source_id;
  }

  static viz::mojom::BeginFrameArgsType type(const viz::BeginFrameArgs& args) {
    return static_cast<viz::mojom::BeginFrameArgsType>(args.type);
  }

  static bool on_critical_path(const viz::BeginFrameArgs& args) {
    return args.on_critical_path;
  }

  static bool animate_only(const viz::BeginFrameArgs& args) {
    return args.animate_only;
  }

  static bool Read(viz::mojom::BeginFrameArgsDataView data,
                   viz::BeginFrameArgs* out);
};

template <>
struct StructTraits<viz::mojom::BeginFrameAckDataView, viz::BeginFrameAck> {
  static uint64_t sequence_number(const viz::BeginFrameAck& ack) {
    return ack.sequence_number;
  }

  static uint64_t source_id(const viz::BeginFrameAck& ack) {
    return ack.source_id;
  }

  static bool has_damage(const viz::BeginFrameAck& ack) {
    return ack.has_damage;
  }

  static bool Read(viz::mojom::BeginFrameAckDataView data,
                   viz::BeginFrameAck* out);
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_BEGIN_FRAME_ARGS_STRUCT_TRAITS_H_
