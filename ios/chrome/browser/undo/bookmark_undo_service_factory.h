// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UNDO_BOOKMARK_UNDO_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_UNDO_BOOKMARK_UNDO_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class BookmarkUndoService;

namespace ios {

class ChromeBrowserState;

// Singleton that owns all FaviconServices and associates them with
// ios::ChromeBrowserState.
class BookmarkUndoServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static BookmarkUndoService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static BookmarkUndoService* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);
  static BookmarkUndoServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BookmarkUndoServiceFactory>;

  BookmarkUndoServiceFactory();
  ~BookmarkUndoServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BookmarkUndoServiceFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_UNDO_BOOKMARK_UNDO_SERVICE_FACTORY_H_
