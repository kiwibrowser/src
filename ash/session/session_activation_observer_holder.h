// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SESSION_SESSION_ACTIVATION_OBSERVER_HOLDER_H_
#define ASH_SESSION_SESSION_ACTIVATION_OBSERVER_HOLDER_H_

#include <map>
#include <memory>

#include "ash/public/interfaces/session_controller.mojom.h"
#include "base/macros.h"
#include "components/account_id/account_id.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace ash {

class SessionActivationObserverHolder {
 public:
  SessionActivationObserverHolder();
  ~SessionActivationObserverHolder();

  void AddSessionActivationObserverForAccountId(
      const AccountId& account_id,
      mojom::SessionActivationObserverPtr observer);

  void NotifyActiveSessionChanged(const AccountId& from, const AccountId& to);

  void NotifyLockStateChanged(bool locked);

 private:
  using ObserverSet = mojo::InterfacePtrSet<mojom::SessionActivationObserver>;
  std::map<AccountId, std::unique_ptr<ObserverSet>> observer_map_;

  DISALLOW_COPY_AND_ASSIGN(SessionActivationObserverHolder);
};

}  // namespace ash

#endif  // ASH_SESSION_SESSION_ACTIVATION_OBSERVER_HOLDER_H_
