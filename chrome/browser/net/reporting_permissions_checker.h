// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_REPORTING_PERMISSIONS_CHECKER_H_
#define CHROME_BROWSER_NET_REPORTING_PERMISSIONS_CHECKER_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

class Profile;

namespace url {
class Origin;
}

class ReportingPermissionsCheckerFactory;

// Used by the Reporting API to check whether the user has allowed reports to be
// uploaded for particular origins, via the BACKGROUND_SYNC permission.
//
// Instances of this class will live on and be used from the IO thread.  The
// actual permission checking has to happen on the UI thread, via the current
// Profile; this class takes care of managing the thread orchestration for you.
// In particular, instances of this class are guaranteed to remain valid even
// once the Profile has started tearing itself down on the UI thread.
class ReportingPermissionsChecker {
 public:
  explicit ReportingPermissionsChecker(base::WeakPtr<Profile> weak_profile);

  ~ReportingPermissionsChecker();

  // Checks whether each origin in |origins| has the BACKGROUND_SYNC permission
  // set, removing any that don't.  Call this from the IO thread.  The filter
  // will perform the check on the UI thread, and invoke |result_callback| back
  // on the IO thread with the result.  If the Profile has already started
  // tearing itself down, the callback will never be invoked.
  void FilterReportingOrigins(
      std::set<url::Origin> origins,
      base::OnceCallback<void(std::set<url::Origin>)> result_callback);

 private:
  base::WeakPtr<Profile> weak_profile_;

  DISALLOW_COPY_AND_ASSIGN(ReportingPermissionsChecker);
};

// The actual BACKGROUND_SYNC permission checks need to happen on the UI thread.
// This class lives on the UI thread, and is owned by ProfileImpl.
class ReportingPermissionsCheckerFactory {
 public:
  // Does not take ownership of |profile|, which must outlive this instance.
  // Must be called from the UI thread.
  explicit ReportingPermissionsCheckerFactory(Profile* profile);

  ~ReportingPermissionsCheckerFactory();

  // Creates a new ReportingPermissionsChecker that can safely be used from
  // the IO thread, even when the owning Profile has started tearing itself
  // down.  Must be called from the UI thread.
  std::unique_ptr<ReportingPermissionsChecker> CreateChecker();

 private:
  friend class ReportingPermissionsChecker;

  // Checks whether each origin in |origins| has the BACKGROUND_SYNC permission
  // set, removing any that don't.  This must be called from the UI thread.
  static std::set<url::Origin> DoFilterReportingOrigins(
      base::WeakPtr<Profile> profile,
      std::set<url::Origin> origins);

  base::WeakPtrFactory<Profile> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ReportingPermissionsCheckerFactory);
};

#endif  // CHROME_BROWSER_NET_REPORTING_PERMISSIONS_CHECKER_H_
