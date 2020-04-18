// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_SUPERVISED_USER_WHITELIST_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_SUPERVISED_USER_WHITELIST_INSTALLER_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/strings/string16.h"

namespace base {
class FilePath;
}

class PrefRegistrySimple;
class PrefService;
class ProfileAttributesStorage;

namespace component_updater {

class ComponentUpdateService;
class OnDemandUpdater;

class SupervisedUserWhitelistInstaller {
 public:
  using WhitelistReadyCallback =
      base::Callback<void(const std::string& crx_id,
                          const base::string16& title,
                          const base::FilePath& large_icon_path,
                          const base::FilePath& whitelist_path)>;

  virtual ~SupervisedUserWhitelistInstaller() {}

  static std::unique_ptr<SupervisedUserWhitelistInstaller> Create(
      ComponentUpdateService* cus,
      ProfileAttributesStorage* profile_attributes_storage,
      PrefService* local_state);

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Generates a client ID suitable for RegisterWhitelist() and
  // UnregisterWhitelist() below from a profile path.
  static std::string ClientIdForProfilePath(const base::FilePath& profile_path);

  // Turns a CRX ID (which is derived from a hash) back into a hash.
  // Note that the resulting hash will be only 16 bytes long instead of the
  // usual 32 bytes, as the CRX ID is created from the first half of the
  // original hash, but the component installer will still accept this.
  // Public for testing.
  static std::vector<uint8_t> GetHashFromCrxId(const std::string& crx_id);

  // Starts registering all components with the ComponentUpdaterService.
  // Also removes unregistered components on disk (which are most likely left
  // over from a previous uninstallation that was interrupted, e.g. during
  // shutdown or a crash).
  virtual void RegisterComponents() = 0;

  // Subscribes for notifications about available whitelists. Clients should
  // filter out the whitelists they are interested in via the |crx_id|
  // parameter.
  virtual void Subscribe(const WhitelistReadyCallback& callback) = 0;

  // Registers a new whitelist with the given |crx_id|.
  // The |client_id| should be a unique identifier for the client that is stable
  // across restarts. If it is empty, the registration will not be persisted in
  // Local State.
  virtual void RegisterWhitelist(const std::string& client_id,
                                 const std::string& crx_id,
                                 const std::string& name) = 0;

  // Unregisters a whitelist.
  virtual void UnregisterWhitelist(const std::string& client_id,
                                   const std::string& crx_id) = 0;

 protected:
  // Triggers an update for a whitelist to be installed. Protected so it can be
  // called from the implementation subclass, and declared here so that the
  // OnDemandUpdater can friend this class and the implementation subclass can
  // live in an anonymous namespace.
  static void TriggerComponentUpdate(OnDemandUpdater* updater,
                                     const std::string& crx_id);
};

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_SUPERVISED_USER_WHITELIST_INSTALLER_H_
