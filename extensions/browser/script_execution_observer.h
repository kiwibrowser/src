// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_SCRIPT_EXECUTION_OBSERVER_H_
#define EXTENSIONS_BROWSER_SCRIPT_EXECUTION_OBSERVER_H_

#include <map>
#include <set>
#include <string>

class GURL;

namespace content {
class WebContents;
}

namespace extensions {

// Observer base class for classes that need to be notified when content
// scripts and/or tabs.executeScript calls run on a page.
class ScriptExecutionObserver {
 public:
  // Map of extensions IDs to the executing script paths.
  typedef std::map<std::string, std::set<std::string> > ExecutingScriptsMap;

  // Called when script(s) have executed on a page.
  //
  // |executing_scripts_map| contains all extensions that are executing
  // scripts, mapped to the paths for those scripts. The paths may be an empty
  // set if the script has no path associated with it (e.g. in the case of
  // tabs.executeScript), but there will still be an entry for the extension.
  virtual void OnScriptsExecuted(
      const content::WebContents* web_contents,
      const ExecutingScriptsMap& executing_scripts_map,
      const GURL& on_url) = 0;

 protected:
  virtual ~ScriptExecutionObserver();
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_SCRIPT_EXECUTION_OBSERVER_H_
