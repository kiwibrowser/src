// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_SIGNIN_TRACKER_FACTORY_H_
#define CHROME_BROWSER_SIGNIN_SIGNIN_TRACKER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/signin/core/browser/signin_tracker.h"

class Profile;

class SigninTrackerFactory {
 public:
  virtual ~SigninTrackerFactory();

  // Creates a SigninTracker instance that tracks signin for |profile| on
  // behalf of |observer|.
  static std::unique_ptr<SigninTracker> CreateForProfile(
      Profile* profile,
      SigninTracker::Observer* observer);

 private:
  SigninTrackerFactory();

  DISALLOW_COPY_AND_ASSIGN(SigninTrackerFactory);
};

#endif  // CHROME_BROWSER_SIGNIN_SIGNIN_TRACKER_FACTORY_H_
