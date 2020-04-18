// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_RENDERER_PROCESS_TYPE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_RENDERER_PROCESS_TYPE_H_

namespace blink {
namespace scheduler {

enum class RendererProcessType {
  kExtensionRenderer,
  kRenderer,
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_RENDERER_PROCESS_TYPE_H_
