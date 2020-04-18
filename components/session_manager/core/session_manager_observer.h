// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSION_MANAGER_CORE_SESSION_MANAGER_OBSERVER_H_
#define COMPONENTS_SESSION_MANAGER_CORE_SESSION_MANAGER_OBSERVER_H_

#include "base/macros.h"
#include "components/session_manager/session_manager_types.h"

namespace session_manager {

// An observer interface for SessionManager.
// TODO(xiyuan): Use this to replace UserManager::UserSessionStateObserver,
//     http://crbug.com/657149.
class SessionManagerObserver {
 public:
  SessionManagerObserver() = default;

  // Invoked when session state is changed.
  virtual void OnSessionStateChanged() = 0;

 protected:
  virtual ~SessionManagerObserver() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(SessionManagerObserver);
};

}  // namespace session_manager

#endif  // COMPONENTS_SESSION_MANAGER_CORE_SESSION_MANAGER_OBSERVER_H_
