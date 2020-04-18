// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOCOMPLETE_SHORTCUTS_BACKEND_FACTORY_H_
#define IOS_CHROME_BROWSER_AUTOCOMPLETE_SHORTCUTS_BACKEND_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/ios/refcounted_browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class ShortcutsBackend;

namespace ios {

class ChromeBrowserState;

// Singleton that owns all ShortcutsBackends and associates them with
// ios::ChromeBrowserState.
class ShortcutsBackendFactory
    : public RefcountedBrowserStateKeyedServiceFactory {
 public:
  static scoped_refptr<ShortcutsBackend> GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static scoped_refptr<ShortcutsBackend> GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);
  static ShortcutsBackendFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ShortcutsBackendFactory>;

  ShortcutsBackendFactory();
  ~ShortcutsBackendFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(ShortcutsBackendFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_AUTOCOMPLETE_SHORTCUTS_BACKEND_FACTORY_H_
