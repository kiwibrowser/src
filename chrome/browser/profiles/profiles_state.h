// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILES_STATE_H_
#define CHROME_BROWSER_PROFILES_PROFILES_STATE_H_

#include <string>

#include "base/strings/string16.h"

#if !defined(OS_CHROMEOS)
#include <vector>
#include "chrome/browser/profiles/avatar_menu.h"
#endif

class Browser;
class PrefRegistrySimple;
class Profile;
class SigninErrorController;
namespace base { class FilePath; }

namespace profiles {

// Assortment of methods for dealing with profiles.
// TODO(michaelpg): Most of these functions can be inlined or moved to more
// appropriate locations.

// Checks if multiple profiles is enabled.
bool IsMultipleProfilesEnabled();

// Returns the path to the default profile directory, based on the given
// user data directory.
base::FilePath GetDefaultProfileDir(const base::FilePath& user_data_dir);

// Register multi-profile related preferences in Local State.
void RegisterPrefs(PrefRegistrySimple* registry);

// Returns the display name of the specified on-the-record profile (or guest),
// specified by |profile_path|, used in the avatar button or user manager. If
// |profile_path| is the guest path, it will return IDS_GUEST_PROFILE_NAME. If
// there is only one local profile present, it will return
// IDS_SINGLE_PROFILE_DISPLAY_NAME, unless the profile has a user entered
// custom name.
base::string16 GetAvatarNameForProfile(const base::FilePath& profile_path);

#if !defined(OS_CHROMEOS)
// Returns the string to use in the avatar button for the specified profile.
// This is essentially the name returned by GetAvatarNameForProfile, but it
// may be elided and contain an indicator for supervised users.
base::string16 GetAvatarButtonTextForProfile(Profile* profile);

// Returns the string to use in the fast user switcher menu for the specified
// menu item. Adds a supervision indicator to the profile name if appropriate.
base::string16 GetProfileSwitcherTextForItem(const AvatarMenu::Item& item);

// Update the name of |profile| to |new_profile_name|. This updates the profile
// preferences, which triggers an update in the ProfileAttributesStorage. This
// method should be called when the user is explicitely changing the profile
// name, as it will always set |prefs::kProfileUsingDefaultName| to false.
void UpdateProfileName(Profile* profile,
                       const base::string16& new_profile_name);

// Returns the list of secondary accounts for a specific |profile|, which is
// all the email addresses associated with the profile that are not equal to
// the |primary_account|.
std::vector<std::string> GetSecondaryAccountsForProfile(
    Profile* profile,
    const std::string& primary_account);
#endif

// Returns whether the |browser|'s profile is a non-incognito or guest profile.
// The distinction is needed because guest profiles are implemented as
// incognito profiles.
bool IsRegularOrGuestSession(Browser* browser);

// Returns true if sign in is required to browse as this profile.  Call with
// profile->GetPath() if you have a profile pointer.
// TODO(mlerman): Refactor appropriate calls to
// ProfileAttributesStorage::IsSigninRequired to call here instead.
bool IsProfileLocked(const base::FilePath& profile_path);

#if !defined(OS_CHROMEOS)
// If the lock-enabled information for this profile is not up to date, starts
// an update for the Gaia profile info.
void UpdateIsProfileLockEnabledIfNeeded(Profile* profile);

// Starts an update for a new version of the Gaia profile picture and other
// profile info.
void UpdateGaiaProfileInfoIfNeeded(Profile* profile);

// Returns the sign-in error controller for the given profile.  Some profiles,
// like guest profiles, may not have a controller so this function may return
// NULL.
SigninErrorController* GetSigninErrorController(Profile* profile);

// If the current active profile (given by prefs::kProfileLastUsed) is locked,
// changes the active profile to the Guest profile. Returns true if the active
// profile had been Guest before calling or became Guest as a result of this
// method.
bool SetActiveProfileToGuestIfLocked();
#endif

// If the profile given by |profile_path| is loaded in the ProfileManager, use
// a BrowsingDataRemover to delete all the Profile's data.
void RemoveBrowsingDataForProfile(const base::FilePath& profile_path);

// Sets the last used profile pref to |profile_dir|, unless |profile_dir| is the
// System Profile directory, which is an invalid last used profile.
void SetLastUsedProfile(const std::string& profile_dir);

#if !defined(OS_CHROMEOS)
// Returns true if there exists at least one non-supervised or non-child profile
// and they are all locked.
bool AreAllNonChildNonSupervisedProfilesLocked();
#endif

// Returns whether a public session is being run currently.
bool IsPublicSession();

}  // namespace profiles

#endif  // CHROME_BROWSER_PROFILES_PROFILES_STATE_H_
