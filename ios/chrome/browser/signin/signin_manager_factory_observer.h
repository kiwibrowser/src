// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_FACTORY_OBSERVER_H_
#define IOS_CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_FACTORY_OBSERVER_H_

#include "base/macros.h"

class SigninManager;

// Observer for SigninManagerFactory.
class SigninManagerFactoryObserver {
 public:
  SigninManagerFactoryObserver() {}
  virtual ~SigninManagerFactoryObserver() {}

  // Called when a SigninManager instance is created.
  virtual void SigninManagerCreated(SigninManager* manager) {}

  // Called when a SigninManager instance is being shut down. Observers
  // of |manager| should remove themselves at this point.
  virtual void SigninManagerShutdown(SigninManager* manager) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SigninManagerFactoryObserver);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_FACTORY_OBSERVER_H_
