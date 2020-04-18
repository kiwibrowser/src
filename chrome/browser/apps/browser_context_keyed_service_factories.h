// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_APPS_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
#define CHROME_BROWSER_APPS_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_

namespace content {
class BrowserContext;
}

namespace chrome_apps {

// Ensures the existence of any BrowserContextKeyedServiceFactory provided by
// the Chrome apps code.
void EnsureBrowserContextKeyedServiceFactoriesBuilt();

// Notifies the relevant BrowserContextKeyedServices for the browser context
// that the application is being terminated.
void NotifyApplicationTerminating(content::BrowserContext* browser_context);

}  // namespace chrome_apps

#endif  // CHROME_BROWSER_APPS_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
