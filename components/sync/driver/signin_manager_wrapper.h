// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SIGNIN_MANAGER_WRAPPER_H_
#define COMPONENTS_SYNC_DRIVER_SIGNIN_MANAGER_WRAPPER_H_

#include "base/macros.h"

class SigninManagerBase;

namespace identity {
class IdentityManager;
}

// TODO(crbug.com/825190): Get rid of this class after ProfileSyncService
// doesn't use SigninManager anymore.
class SigninManagerWrapper {
 public:
  explicit SigninManagerWrapper(identity::IdentityManager* identity_manager,
                                SigninManagerBase* signin_manager);
  ~SigninManagerWrapper();

  // Return the original IdentityManager object that was passed in.
  identity::IdentityManager* GetIdentityManager();

  // Return the original SigninManagerBase object that was passed in.
  SigninManagerBase* GetSigninManager();

 private:
  identity::IdentityManager* identity_manager_;
  SigninManagerBase* signin_manager_;

  DISALLOW_COPY_AND_ASSIGN(SigninManagerWrapper);
};

#endif  // COMPONENTS_SYNC_DRIVER_SIGNIN_MANAGER_WRAPPER_H_
