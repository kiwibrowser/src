// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_TEST_CHROME_WEB_UI_CONTROLLER_FACTORY_H_
#define CHROME_TEST_BASE_TEST_CHROME_WEB_UI_CONTROLLER_FACTORY_H_

#include <functional>
#include <map>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"
#include "content/public/browser/web_ui.h"

// This class replaces the ChromeWebUIFactory when the switches::kTestType flag
// is passed. It provides a registry to override CreateWebUIControllerForURL()
// by host.
class TestChromeWebUIControllerFactory : public ChromeWebUIControllerFactory {
 public:
  // Interface to create a new WebUI object.
  class WebUIProvider {
   public:
    // Create and return a new WebUI object for the |web_contents| based on the
    // |url|.
     virtual content::WebUIController* NewWebUI(content::WebUI* web_ui,
                                                const GURL& url) = 0;

   protected:
    virtual ~WebUIProvider();
  };

  typedef std::map<std::string, WebUIProvider*> FactoryOverridesMap;

  TestChromeWebUIControllerFactory();
  ~TestChromeWebUIControllerFactory() override;

  // Override the creation for urls having |host| with |provider|.
  void AddFactoryOverride(const std::string& host, WebUIProvider* provider);

  // Remove the override for urls having |host|.
  void RemoveFactoryOverride(const std::string& host);

  // ChromeWebUIFactory overrides.
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) const override;
  content::WebUIController* CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

 private:
  // Return the WebUIProvider for the |url|'s host if it exists, otherwise NULL.
  WebUIProvider* GetWebUIProvider(Profile* profile, const GURL& url) const;

  // Stores the mapping of host to WebUIProvider.
  FactoryOverridesMap factory_overrides_;

  DISALLOW_COPY_AND_ASSIGN(TestChromeWebUIControllerFactory);
};

#endif  // CHROME_TEST_BASE_TEST_CHROME_WEB_UI_CONTROLLER_FACTORY_H_
