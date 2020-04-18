// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_CHROME_WEB_CLIENT_H_
#define IOS_CHROME_BROWSER_WEB_CHROME_WEB_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#import "ios/web/public/web_client.h"

// Chrome implementation of WebClient.
class ChromeWebClient : public web::WebClient {
 public:
  ChromeWebClient();
  ~ChromeWebClient() override;

  // WebClient implementation.
  std::unique_ptr<web::WebMainParts> CreateWebMainParts() override;
  void PreWebViewCreation() const override;
  void AddAdditionalSchemes(Schemes* schemes) const override;
  std::string GetApplicationLocale() const override;
  bool IsAppSpecificURL(const GURL& url) const override;
  base::string16 GetPluginNotSupportedText() const override;
  std::string GetProduct() const override;
  std::string GetUserAgent(web::UserAgentType type) const override;
  base::string16 GetLocalizedString(int message_id) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) const override;
  std::unique_ptr<base::Value> GetServiceManifestOverlay(
      base::StringPiece name) override;
  void GetAdditionalWebUISchemes(
      std::vector<std::string>* additional_schemes) override;
  void PostBrowserURLRewriterCreation(
      web::BrowserURLRewriter* rewriter) override;
  NSString* GetDocumentStartScriptForAllFrames(
      web::BrowserState* browser_state) const override;
  NSString* GetDocumentStartScriptForMainFrame(
      web::BrowserState* browser_state) const override;
  void AllowCertificateError(
      web::WebState* web_state,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool overridable,
      const base::Callback<void(bool)>& callback) override;
  void PrepareErrorPage(NSError* error,
                        bool is_post,
                        bool is_off_the_record,
                        NSString** error_html) override;
  void RegisterServices(StaticServiceMap* services) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeWebClient);
};

#endif  // IOS_CHROME_BROWSER_WEB_CHROME_WEB_CLIENT_H_
