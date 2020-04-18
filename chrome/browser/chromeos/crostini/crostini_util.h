// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_UTIL_H_

#include <string>

#include "base/optional.h"

class Profile;

// Returns true if crostini is allowed to run.
// Otherwise, returns false, e.g. if crostini is not available on the device,
// or it is in the flow to set up managed account creation.
bool IsCrostiniAllowed();

// Returns true if crostini UI can be shown. Implies crostini is allowed to run.
bool IsCrostiniUIAllowedForProfile(Profile* profile);

// Returns whether if Crostini has been enabled, i.e. the user has launched it
// at least once and not deleted it.
bool IsCrostiniEnabled(Profile* profile);

// |app_id| should be a valid Crostini app list id.
void LaunchCrostiniApp(Profile* profile, const std::string& app_id);

// Retrieves cryptohome_id from profile.
std::string CryptohomeIdForProfile(Profile* profile);

// Retrieves username from profile.  This is the text until '@' in
// profile->GetProfileUserName() email address.
std::string ContainerUserNameForProfile(Profile* profile);

// The Terminal opens Crosh but overrides the Browser's app_name so that we can
// identify it as the Crostini Terminal. In the future, we will also use these
// for Crostini apps marked Terminal=true in their .desktop file.
std::string AppNameFromCrostiniAppId(const std::string& id);

// Returns nullopt for a non-Crostini app name.
base::Optional<std::string> CrostiniAppIdFromAppName(
    const std::string& app_name);

void ShowCrostiniInstallerView(Profile* profile);
void ShowCrostiniUninstallerView(Profile* profile);

constexpr char kCrostiniTerminalAppName[] = "Terminal";
// We can use any arbitrary well-formed extension id for the Terminal app, this
// is equal to GenerateId("Terminal").
constexpr char kCrostiniTerminalId[] = "oajcgpnkmhaalajejhlfpacbiokdnnfe";

constexpr char kCrostiniDefaultVmName[] = "termina";
constexpr char kCrostiniDefaultContainerName[] = "penguin";
constexpr char kCrostiniCroshBuiltinAppId[] =
    "nkoccljplnhpfnfiajclkommnmllphnl";

#endif  // CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_UTIL_H_
