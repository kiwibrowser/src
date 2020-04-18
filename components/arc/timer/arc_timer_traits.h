// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_

#include "components/arc/common/timer.mojom.h"
#include "components/arc/timer/create_timer_request.h"

namespace mojo {

template <>
struct EnumTraits<arc::mojom::ClockId, int32_t> {
  static arc::mojom::ClockId ToMojom(int32_t clock_id);
  static bool FromMojom(arc::mojom::ClockId input, int32_t* output);
};

template <>
struct StructTraits<arc::mojom::CreateTimerRequestDataView,
                    arc::CreateTimerRequest> {
  // Due to already defined EnumTraits for |ClockId| the return type is int32_t
  // and not |arc::mojom::ClockId|.
  static arc::mojom::ClockId clock_id(
      const arc::CreateTimerRequest& arc_timer_request);

  static mojo::ScopedHandle expiration_fd(
      arc::CreateTimerRequest& arc_timer_request);

  static bool Read(arc::mojom::CreateTimerRequestDataView input,
                   arc::CreateTimerRequest* output);
};

}  // namespace mojo

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
