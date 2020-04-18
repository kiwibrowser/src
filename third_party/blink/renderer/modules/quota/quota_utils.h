// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_QUOTA_QUOTA_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_QUOTA_QUOTA_UTILS_H_

#include "third_party/blink/public/mojom/quota/quota_dispatcher_host.mojom-blink.h"

namespace blink {

class ExecutionContext;

void ConnectToQuotaDispatcherHost(ExecutionContext*,
                                  mojom::blink::QuotaDispatcherHostRequest);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_QUOTA_QUOTA_UTILS_H_
