// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_INVESTIGATOR_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_INVESTIGATOR_H_

#include <string>

#include "base/macros.h"
#include "components/prefs/pref_service.h"

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.signin
// Broad categorization of signin type from investigation.
enum class InvestigatedScenario : int {
  // First signin and should not be warned. As little friction as possible
  // should get between the user and signing in.
  UPGRADE_LOW_RISK = 0,
  // First signin but should be warned. There is a reason to believe this signin
  // may not be what the user wanted.
  UPGRADE_HIGH_RISK,
  // Relogging with the same account.
  SAME_ACCOUNT,
  // User is switching accounts, can be very dangerous depending on the amount
  // of local syncable data.
  DIFFERENT_ACCOUNT,
  // Always the last enumerated type.
  HISTOGRAM_COUNT,
};

// Performs various checks with the current account being used to login in and
// against the local data. Decides what kind of signin scenario we're in, if
// we should warn the user about sync merge dangerous, and emits metrics.
class SigninInvestigator {
 public:
  // This interface allows the embedder to only have to worry about retrieving
  // dependencies, not any of the logic for using them.
  class DependencyProvider {
   public:
    virtual ~DependencyProvider() {}
    // Returns a non owning pointer to the pref service.
    virtual PrefService* GetPrefs() = 0;
  };

  SigninInvestigator(const std::string& current_email,
                     const std::string& current_id,
                     DependencyProvider* provider);
  ~SigninInvestigator();

  // Determines the current scenario, wether it is an upgrade, same account, or
  // different.
  InvestigatedScenario Investigate();

 private:
  friend class SigninInvestigatorTest;

  // Determines if the current account is the same as the last email/gaia id.
  // Because email can change but gaia id cannot, the id is the authoritative
  // source of account equality. However, initially we were only storing the
  // last username/email that was used to sign in, so for any client that hasn't
  // logged in since we added logic to store the gaia id, the last id is blank.
  // In this case, we fallback to using last_email. This equality check is
  // slightly more strict than the version AccountId defines as == operator.
  bool AreAccountsEqualWithFallback();

  // Determines if the current upgrade signin is high risk or not. Reasons for
  // being high risk may include things like this profile/device may be shared
  // among multiple people, or there may be old syncable data created by other
  // users that do not currently use the profile/device.
  bool IsUpgradeHighRisk();

  std::string current_email_;
  std::string current_id_;

  // Non-owning pointer.
  DependencyProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(SigninInvestigator);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_INVESTIGATOR_H_
