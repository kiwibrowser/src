// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_session.h"

#include "components/arc/arc_session_impl.h"

namespace arc {

ArcSession::ArcSession() = default;
ArcSession::~ArcSession() = default;

void ArcSession::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void ArcSession::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

// static
std::unique_ptr<ArcSession> ArcSession::Create(
    ArcBridgeService* arc_bridge_service) {
  return std::make_unique<ArcSessionImpl>(
      ArcSessionImpl::CreateDelegate(arc_bridge_service));
}

}  // namespace arc
