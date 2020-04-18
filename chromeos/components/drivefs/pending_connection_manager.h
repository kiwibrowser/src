// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_DRIVEFS_PENDING_CONNECTION_MANAGER_H_
#define CHROMEOS_COMPONENTS_DRIVEFS_PENDING_CONNECTION_MANAGER_H_

#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/component_export.h"
#include "base/containers/flat_map.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/unguessable_token.h"
#include "chromeos/chromeos_export.h"

namespace drivefs {

class PendingConnectionManagerTest;

class COMPONENT_EXPORT(DRIVEFS) PendingConnectionManager {
 public:
  using OpenIpcChannelCallback = base::OnceCallback<void(base::ScopedFD)>;

  static PendingConnectionManager& Get();

  bool OpenIpcChannel(const std::string& identity, base::ScopedFD ipc_channel);

  void ExpectOpenIpcChannel(base::UnguessableToken token,
                            OpenIpcChannelCallback handler);
  void CancelExpectedOpenIpcChannel(base::UnguessableToken token);

 private:
  friend class base::NoDestructor<PendingConnectionManager>;
  friend class PendingConnectionManagerTest;

  PendingConnectionManager();
  ~PendingConnectionManager();

  base::flat_map<std::string, OpenIpcChannelCallback>
      open_ipc_channel_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(PendingConnectionManager);
};

}  // namespace drivefs

#endif  // CHROMEOS_COMPONENTS_DRIVEFS_PENDING_CONNECTION_MANAGER_H_
