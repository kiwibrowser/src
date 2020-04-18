// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_SCRIPTING_PERMISSIONS_MODIFIER_H_
#define CHROME_BROWSER_EXTENSIONS_SCRIPTING_PERMISSIONS_MODIFIER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"

class GURL;

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
class ExtensionPrefs;
class PermissionSet;

// Responsible for managing the majority of click-to-script features, including
// granting, withholding, and querying host permissions, and determining if an
// extension has been affected by the click-to-script project.
class ScriptingPermissionsModifier {
 public:
  ScriptingPermissionsModifier(content::BrowserContext* browser_context,
                               const scoped_refptr<const Extension>& extension);
  ~ScriptingPermissionsModifier();

  // Sets whether the extension should be allowed to execute on all urls without
  // explicit user consent. Used when the features::kRuntimeHostPermissions
  // feature is enabled.
  // This may only be called for extensions that can be affected (i.e., for
  // which CanAffectExtension() returns true). Anything else will DCHECK.
  void SetAllowedOnAllUrls(bool allowed);

  // Returns whether the extension is allowed to execute scripts on all urls
  // without user consent.
  // This may only be called for extensions that can be affected (i.e., for
  // which CanAffectExtension() returns true). Anything else will DCHECK.
  bool IsAllowedOnAllUrls() const;

  // Returns true if the associated extension can be affected by
  // features::kRuntimeHostPermissions.
  bool CanAffectExtension() const;

  // Grants the extension permission to run on the origin of |url|.
  // This may only be called for extensions that can be affected (i.e., for
  // which CanAffectExtension() returns true). Anything else will DCHECK.
  void GrantHostPermission(const GURL& url);

  // Returns true if the extension has been explicitly granted permission to run
  // on the origin of |url|.
  // This may only be called for extensions that can be affected (i.e., for
  // which CanAffectExtension() returns true). Anything else will DCHECK.
  bool HasGrantedHostPermission(const GURL& url) const;

  // Revokes permission to run on the origin of |url|. DCHECKs if |url| has not
  // been granted.
  // This may only be called for extensions that can be affected (i.e., for
  // which CanAffectExtension() returns true). Anything else will DCHECK.
  void RemoveGrantedHostPermission(const GURL& url);

  // Takes in a set of permissions and withholds any permissions that should not
  // be granted for the given |extension|, populating |granted_permissions_out|
  // with the set of all permissions that can be granted, and
  // |withheld_permissions_out| with the set of all withheld permissions. Note:
  // we pass in |permissions| explicitly here, as this is used during permission
  // initialization, where the active permissions on the extension may not be
  // the permissions to compare against.
  static void WithholdPermissionsIfNecessary(
      const Extension& extension,
      const ExtensionPrefs& extension_prefs,
      const PermissionSet& permissions,
      std::unique_ptr<const PermissionSet>* granted_permissions_out,
      std::unique_ptr<const PermissionSet>* withheld_permissions_out);

  // Returns the subset of active permissions which can be withheld.
  std::unique_ptr<const PermissionSet> GetRevokablePermissions() const;

 private:
  // Grants any withheld all-hosts (or all-hosts-like) permissions.
  void GrantWithheldImpliedAllHosts();

  // Revokes any granted all-hosts (or all-hosts-like) permissions.
  void WithholdImpliedAllHosts();

  content::BrowserContext* browser_context_;

  scoped_refptr<const Extension> extension_;

  ExtensionPrefs* extension_prefs_;

  DISALLOW_COPY_AND_ASSIGN(ScriptingPermissionsModifier);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_SCRIPTING_PERMISSIONS_MODIFIER_H_
