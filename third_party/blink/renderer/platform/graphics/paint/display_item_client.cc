// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/display_item_client.h"

#if DCHECK_IS_ON()
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#endif

namespace blink {

DisplayItemClient::CacheGenerationOrInvalidationReason::ValueType
    DisplayItemClient::CacheGenerationOrInvalidationReason::next_generation_ =
        kFirstValidGeneration;

#if DCHECK_IS_ON()

HashSet<const DisplayItemClient*>* g_live_display_item_clients = nullptr;

DisplayItemClient::DisplayItemClient() {
  if (!g_live_display_item_clients)
    g_live_display_item_clients = new HashSet<const DisplayItemClient*>();
  g_live_display_item_clients->insert(this);
}

DisplayItemClient::~DisplayItemClient() {
  g_live_display_item_clients->erase(this);
}

bool DisplayItemClient::IsAlive() const {
  return g_live_display_item_clients &&
         g_live_display_item_clients->Contains(this);
}

String DisplayItemClient::SafeDebugName(const DisplayItemClient& client,
                                        bool known_to_be_safe) {
  if (known_to_be_safe) {
    DCHECK(client.IsAlive());
    return client.DebugName();
  }

  // If the caller is not sure, we must ensure the client is alive, and it's
  // not a destroyed client at the same address of a new client.
  if (client.IsAlive() && !client.IsJustCreated())
    return client.DebugName();

  return "DEAD";
}

#endif  // DCHECK_IS_ON()

}  // namespace blink
