// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines the Chrome Extensions Cookies API functions for accessing internet
// cookies, as specified in the extension API JSON.

#ifndef CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/browser/net/chrome_cookie_notification_details.h"
#include "chrome/common/extensions/api/cookies.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"
#include "net/cookies/canonical_cookie.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "url/gurl.h"

namespace extensions {

// Observes CookieMonster notifications and routes them as events to the
// extension system.
class CookiesEventRouter : public content::NotificationObserver {
 public:
  explicit CookiesEventRouter(content::BrowserContext* context);
  ~CookiesEventRouter() override;

 private:
  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Handler for the COOKIE_CHANGED event. The method takes the details of such
  // an event and constructs a suitable JSON formatted extension event from it.
  void CookieChanged(Profile* profile, ChromeCookieDetails* details);

  // This method dispatches events to the extension message service.
  void DispatchEvent(content::BrowserContext* context,
                     events::HistogramValue histogram_value,
                     const std::string& event_name,
                     std::unique_ptr<base::ListValue> event_args,
                     GURL& cookie_domain);

  // Used for tracking registrations to CookieMonster notifications.
  content::NotificationRegistrar registrar_;

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(CookiesEventRouter);
};

// Implements the cookies.get() extension function.
class CookiesGetFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.get", COOKIES_GET)

  CookiesGetFunction();

 protected:
  ~CookiesGetFunction() override;

  // ExtensionFunction:
  bool RunAsync() override;

 private:
  void GetCookieCallback(const net::CookieList& cookie_list);

  GURL url_;
  network::mojom::CookieManagerPtr store_browser_cookie_manager_;
  std::unique_ptr<api::cookies::Get::Params> parsed_args_;
};

// Implements the cookies.getAll() extension function.
class CookiesGetAllFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.getAll", COOKIES_GETALL)

  CookiesGetAllFunction();

 protected:
  ~CookiesGetAllFunction() override;

  // ExtensionFunction:
  bool RunAsync() override;

 private:
  void GetAllCookiesCallback(const net::CookieList& cookie_list);

  GURL url_;
  network::mojom::CookieManagerPtr store_browser_cookie_manager_;
  std::unique_ptr<api::cookies::GetAll::Params> parsed_args_;
};

// Implements the cookies.set() extension function.
class CookiesSetFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.set", COOKIES_SET)

  CookiesSetFunction();

 protected:
  ~CookiesSetFunction() override;
  bool RunAsync() override;

 private:
  void SetCanonicalCookieCallback(bool set_cookie_);
  void GetCookieListCallback(const net::CookieList& cookie_list);

  enum { NO_RESPONSE, SET_COMPLETED, GET_COMPLETED } state_;
  GURL url_;
  bool success_;
  network::mojom::CookieManagerPtr store_browser_cookie_manager_;
  std::unique_ptr<api::cookies::Set::Params> parsed_args_;
};

// Implements the cookies.remove() extension function.
class CookiesRemoveFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.remove", COOKIES_REMOVE)

  CookiesRemoveFunction();

 protected:
  ~CookiesRemoveFunction() override;

  // ExtensionFunction:
  bool RunAsync() override;

 private:
  void RemoveCookieCallback(uint32_t /* num_deleted */);

  GURL url_;
  network::mojom::CookieManagerPtr store_browser_cookie_manager_;
  std::unique_ptr<api::cookies::Remove::Params> parsed_args_;
};

// Implements the cookies.getAllCookieStores() extension function.
class CookiesGetAllCookieStoresFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cookies.getAllCookieStores",
                             COOKIES_GETALLCOOKIESTORES)

 protected:
  ~CookiesGetAllCookieStoresFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class CookiesAPI : public BrowserContextKeyedAPI, public EventRouter::Observer {
 public:
  explicit CookiesAPI(content::BrowserContext* context);
  ~CookiesAPI() override;

  // KeyedService implementation.
  void Shutdown() override;

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<CookiesAPI>* GetFactoryInstance();

  // EventRouter::Observer implementation.
  void OnListenerAdded(const EventListenerInfo& details) override;

 private:
  friend class BrowserContextKeyedAPIFactory<CookiesAPI>;

  content::BrowserContext* browser_context_;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "CookiesAPI";
  }
  static const bool kServiceIsNULLWhileTesting = true;

  // Created lazily upon OnListenerAdded.
  std::unique_ptr<CookiesEventRouter> cookies_event_router_;

  DISALLOW_COPY_AND_ASSIGN(CookiesAPI);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
