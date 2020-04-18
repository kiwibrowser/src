// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_PERMISSIONS_UPDATER_H__
#define CHROME_BROWSER_EXTENSIONS_PERMISSIONS_UPDATER_H__

#include <memory>
#include <string>

#include "base/macros.h"
#include "extensions/browser/extension_event_histogram_value.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class Extension;
class PermissionSet;
class URLPatternSet;

// Updates an Extension's active and granted permissions in persistent storage
// and notifies interested parties of the changes.
class PermissionsUpdater {
 public:
  // Platform specific delegate.
  class Delegate {
   public:
    virtual ~Delegate() {}
    // Platform specific initialization of |extension|'s permissions (does any
    // necessary filtering of permissions or similar).
    virtual void InitializePermissions(
        const Extension* extension,
        std::unique_ptr<const PermissionSet>* granted_permissions) = 0;
  };

  enum InitFlag {
    INIT_FLAG_NONE = 0,
    INIT_FLAG_TRANSIENT = 1 << 0,
  };

  enum RemoveType {
    REMOVE_SOFT,
    REMOVE_HARD,
  };

  explicit PermissionsUpdater(content::BrowserContext* browser_context);
  PermissionsUpdater(content::BrowserContext* browser_context,
                     InitFlag init_flag);
  ~PermissionsUpdater();

  // Sets a delegate to provide platform-specific logic. This should be set
  // during startup (to ensure all extensions are initialized through the
  // delegate).
  // |delegate| is a singleton instance and is leaked.
  static void SetPlatformDelegate(Delegate* delegate);

  // Adds the set of |permissions| to the |extension|'s active permission set
  // and sends the relevant messages and notifications. This method assumes the
  // user has already been prompted, if necessary, for the extra permissions.
  void AddPermissions(const Extension* extension,
                      const PermissionSet& permissions);

  // Removes the set of |permissions| from the |extension|'s active permission
  // set and sends the relevant messages and notifications.
  // If |remove_type| is REMOVE_HARD, this removes the permissions from the
  // granted permissions in the prefs (meaning that the extension would have
  // to prompt the user again for permission).
  // You should use REMOVE_HARD to ensure the extension cannot silently regain
  // the permission, which is the case when the permission is removed by the
  // user. If it's the extension itself removing the permission, it is safe to
  // use REMOVE_SOFT.
  void RemovePermissions(const Extension* extension,
                         const PermissionSet& permissions,
                         RemoveType remove_type);

  // Removes the |permissions| from |extension| and makes no effort to determine
  // if doing so is safe in the slightlest. This method shouldn't be used,
  // except for removing permissions totally blacklisted by management.
  void RemovePermissionsUnsafe(const Extension* extension,
                               const PermissionSet& permissions);

  // Sets list of hosts |extension| may not interact with (overrides default).
  void SetPolicyHostRestrictions(const Extension* extension,
                                 const URLPatternSet& runtime_blocked_hosts,
                                 const URLPatternSet& runtime_allowed_hosts);

  // Sets extension to use the default list of policy host restrictions.
  void SetUsesDefaultHostRestrictions(const Extension* extension);

  // Sets list of hosts extensions may not interact with. Extension specific
  // exceptions to this default policy are defined with
  // SetPolicyHostRestrictions.
  void SetDefaultPolicyHostRestrictions(
      const URLPatternSet& default_runtime_blocked_hosts,
      const URLPatternSet& default_runtime_allowed_hosts);

  // Returns the set of revokable permissions.
  std::unique_ptr<const PermissionSet> GetRevokablePermissions(
      const Extension* extension) const;

  // Adds all permissions in the |extension|'s active permissions to its
  // granted permission set.
  void GrantActivePermissions(const Extension* extension);

  // Initializes the |extension|'s active permission set to include only
  // permissions currently requested by the extension and all the permissions
  // required by the extension.
  void InitializePermissions(const Extension* extension);

 private:
  enum EventType {
    ADDED,
    REMOVED,
    POLICY,
  };

  // Sets the |extension|'s active permissions to |active| and records the
  // change in the prefs. If |withheld| is non-null, also sets the extension's
  // withheld permissions to |withheld|. Otherwise, |withheld| permissions are
  // not changed.
  void SetPermissions(const Extension* extension,
                      std::unique_ptr<const PermissionSet> active,
                      std::unique_ptr<const PermissionSet> withheld);

  // Dispatches specified event to the extension.
  void DispatchEvent(const std::string& extension_id,
                     events::HistogramValue histogram_value,
                     const char* event_name,
                     const PermissionSet& changed_permissions);

  // Issues the relevant events, messages and notifications when the
  // |extension|'s permissions have |changed| (|changed| is the delta).
  // Specifically, this sends the EXTENSION_PERMISSIONS_UPDATED notification,
  // the ExtensionMsg_UpdatePermissions IPC message, and fires the
  // onAdded/onRemoved events in the extension.
  void NotifyPermissionsUpdated(EventType event_type,
                                const Extension* extension,
                                const PermissionSet& changed);

  // Issues the relevant events, messages and notifications when the
  // default scope management policy have changed.
  // Specifically, this sends the ExtensionMsg_UpdateDefaultHostRestrictions
  // IPC message.
  void NotifyDefaultPolicyHostRestrictionsUpdated(
      const URLPatternSet& default_runtime_blocked_hosts,
      const URLPatternSet& default_runtime_allowed_hosts);

  // The associated BrowserContext.
  content::BrowserContext* browser_context_;

  // Initialization flag that determines whether prefs is consulted about the
  // extension. Transient extensions should not have entries in prefs.
  InitFlag init_flag_;

  DISALLOW_COPY_AND_ASSIGN(PermissionsUpdater);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_PERMISSIONS_UPDATER_H__
