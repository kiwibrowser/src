// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/session/session_activation_observer_holder.h"

#include <utility>

#include "base/stl_util.h"

namespace ash {

SessionActivationObserverHolder::SessionActivationObserverHolder() = default;

SessionActivationObserverHolder::~SessionActivationObserverHolder() = default;

void SessionActivationObserverHolder ::AddSessionActivationObserverForAccountId(
    const AccountId& account_id,
    mojom::SessionActivationObserverPtr observer) {
  if (!account_id.is_valid())
    return;
  auto& observers = observer_map_[account_id];
  if (!observers)
    observers = std::make_unique<ObserverSet>();
  observers->AddPtr(std::move(observer));
}

void SessionActivationObserverHolder::NotifyActiveSessionChanged(
    const AccountId& from,
    const AccountId& to) {
  auto it = observer_map_.find(from);
  if (it != observer_map_.end()) {
    it->second->ForAllPtrs([](auto* ptr) { ptr->OnSessionActivated(false); });
  }

  it = observer_map_.find(to);
  if (it != observer_map_.end()) {
    it->second->ForAllPtrs([](auto* ptr) { ptr->OnSessionActivated(true); });
  }

  base::EraseIf(observer_map_, [](auto& item) { return item.second->empty(); });
}

void SessionActivationObserverHolder::NotifyLockStateChanged(bool locked) {
  for (const auto& it : observer_map_) {
    it.second->ForAllPtrs(
        [locked](auto* ptr) { ptr->OnLockStateChanged(locked); });
  }

  base::EraseIf(observer_map_, [](auto& item) { return item.second->empty(); });
}

}  // namespace ash
