// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_change_notifier_factory_chromeos.h"

#include "chromeos/network/network_change_notifier_chromeos.h"

namespace chromeos {

namespace {

NetworkChangeNotifierChromeos* g_network_change_notifier_chromeos = NULL;

}  // namespace

net::NetworkChangeNotifier*
NetworkChangeNotifierFactoryChromeos::CreateInstance() {
  DCHECK(!g_network_change_notifier_chromeos);
  g_network_change_notifier_chromeos = new NetworkChangeNotifierChromeos();
  return g_network_change_notifier_chromeos;
}

// static
NetworkChangeNotifierChromeos*
NetworkChangeNotifierFactoryChromeos::GetInstance() {
  return g_network_change_notifier_chromeos;
}

}  // namespace chromeos
