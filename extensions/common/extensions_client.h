// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_EXTENSIONS_CLIENT_H_
#define EXTENSIONS_COMMON_EXTENSIONS_CLIENT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/strings/string_piece.h"
#include "extensions/common/permissions/api_permission_set.h"

class GURL;

namespace base {
class CommandLine;
class FilePath;
}

namespace extensions {

class APIPermissionSet;
class Extension;
class FeatureProvider;
class JSONFeatureProviderSource;
class PermissionMessageProvider;
class URLPatternSet;

// Sets up global state for the extensions system. Should be Set() once in each
// process. This should be implemented by the client of the extensions system.
class ExtensionsClient {
 public:
  typedef std::vector<std::string> ScriptingWhitelist;

  virtual ~ExtensionsClient() {}

  // Initializes global state. Not done in the constructor because unit tests
  // can create additional ExtensionsClients because the utility thread runs
  // in-process.
  virtual void Initialize() = 0;

  // Initializes web store URLs.
  // Default values could be overriden with command line.
  virtual void InitializeWebStoreUrls(base::CommandLine* command_line) = 0;

  // Returns the global PermissionMessageProvider to use to provide permission
  // warning strings.
  virtual const PermissionMessageProvider& GetPermissionMessageProvider()
      const = 0;

  // Returns the application name. For example, "Chromium" or "app_shell".
  virtual const std::string GetProductName() = 0;

  // Create a FeatureProvider for a specific feature type, e.g. "permission".
  virtual std::unique_ptr<FeatureProvider> CreateFeatureProvider(
      const std::string& name) const = 0;

  // Returns the dictionary of the API features json file.
  // TODO(devlin): We should find a way to remove this.
  virtual std::unique_ptr<JSONFeatureProviderSource> CreateAPIFeatureSource()
      const = 0;

  // Takes the list of all hosts and filters out those with special
  // permission strings. Adds the regular hosts to |new_hosts|,
  // and adds any additional permissions to |permissions|.
  // TODO(sashab): Split this function in two: One to filter out ignored host
  // permissions, and one to get permissions for the given hosts.
  virtual void FilterHostPermissions(const URLPatternSet& hosts,
                                     URLPatternSet* new_hosts,
                                     PermissionIDSet* permissions) const = 0;

  // Replaces the scripting whitelist with |whitelist|. Used in the renderer;
  // only used for testing in the browser process.
  virtual void SetScriptingWhitelist(const ScriptingWhitelist& whitelist) = 0;

  // Return the whitelist of extensions that can run content scripts on
  // any origin.
  virtual const ScriptingWhitelist& GetScriptingWhitelist() const = 0;

  // Get the set of chrome:// hosts that |extension| can have host permissions
  // for.
  virtual URLPatternSet GetPermittedChromeSchemeHosts(
      const Extension* extension,
      const APIPermissionSet& api_permissions) const = 0;

  // Returns false if content scripts are forbidden from running on |url|.
  virtual bool IsScriptableURL(const GURL& url, std::string* error) const = 0;

  // Returns true iff a schema named |name| is generated.
  virtual bool IsAPISchemaGenerated(const std::string& name) const = 0;

  // Gets the generated API schema named |name|.
  virtual base::StringPiece GetAPISchema(const std::string& name) const = 0;

  // Determines if certain fatal extensions errors should be surpressed
  // (i.e., only logged) or allowed (i.e., logged before crashing).
  virtual bool ShouldSuppressFatalErrors() const = 0;

  // Records that a fatal error was caught and suppressed. It is expected that
  // embedders will only do so if ShouldSuppressFatalErrors at some point
  // returned true.
  virtual void RecordDidSuppressFatalError() = 0;

  // Returns the base webstore URL prefix.
  virtual const GURL& GetWebstoreBaseURL() const = 0;

  // Returns the URL to use for update manifest queries.
  virtual const GURL& GetWebstoreUpdateURL() const = 0;

  // Returns a flag indicating whether or not a given URL is a valid
  // extension blacklist URL.
  virtual bool IsBlacklistUpdateURL(const GURL& url) const = 0;

  // Returns the set of file paths corresponding to any images within an
  // extension's contents that may be displayed directly within the browser UI
  // or WebUI, such as icons or theme images. This set of paths is used by the
  // extension unpacker to determine which assets should be transcoded safely
  // within the utility sandbox.
  //
  // The default implementation returns the images used as icons for the
  // extension itself, so implementors of ExtensionsClient overriding this may
  // want to call the base class version and then add additional paths to that
  // result.
  virtual std::set<base::FilePath> GetBrowserImagePaths(
      const Extension* extension);

  // Returns whether or not extension APIs are allowed in extension service
  // workers.
  // This is currently disallowed as the code to support this is work in
  // progress.
  // Can be overridden in tests.
  virtual bool ExtensionAPIEnabledInExtensionServiceWorkers() const;

  // Returns the user agent used by the content module.
  virtual std::string GetUserAgent() const;

  // Return the extensions client.
  static ExtensionsClient* Get();

  // Initialize the extensions system with this extensions client.
  static void Set(ExtensionsClient* client);
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_EXTENSIONS_CLIENT_H_
