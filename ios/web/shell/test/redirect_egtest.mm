// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#import "ios/web/public/test/http_server/html_response_provider.h"
#import "ios/web/public/test/http_server/html_response_provider_impl.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#import "ios/web/shell/test/earl_grey/shell_earl_grey.h"
#import "ios/web/shell/test/earl_grey/shell_matchers.h"
#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"
#include "net/http/http_status_code.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::AddressFieldText;
using web::test::HttpServer;

// Redirect test cases for the web shell.
@interface RedirectTestCase : WebShellTestCase
@end

@implementation RedirectTestCase

// Tests loading of page that is redirected 3 times using a 301 redirect.
- (void)testMultipleRedirects {
  // Create map of canned responses and set up the test HTML server.
  std::map<GURL, HtmlResponseProviderImpl::Response> responses;
  const GURL firstRedirectURL = HttpServer::MakeUrl("http://firstRedirect/");
  const GURL secondRedirectURL = HttpServer::MakeUrl("http://secondRedirect/");
  const GURL thirdRedirectURL = HttpServer::MakeUrl("http://thirdRedirect/");
  const GURL destinationURL = HttpServer::MakeUrl("http://destination/");

  responses[firstRedirectURL] = HtmlResponseProviderImpl::GetRedirectResponse(
      secondRedirectURL, net::HTTP_MOVED_PERMANENTLY);
  responses[secondRedirectURL] = HtmlResponseProviderImpl::GetRedirectResponse(
      thirdRedirectURL, net::HTTP_MOVED_PERMANENTLY);
  responses[thirdRedirectURL] = HtmlResponseProviderImpl::GetRedirectResponse(
      destinationURL, net::HTTP_MOVED_PERMANENTLY);
  const char kFinalPageContent[] = "testMultipleRedirects complete";
  responses[destinationURL] =
      HtmlResponseProviderImpl::GetSimpleResponse(kFinalPageContent);
  std::unique_ptr<web::DataResponseProvider> provider(
      new HtmlResponseProvider(responses));
  web::test::SetUpHttpServer(std::move(provider));

  // Load first URL and expect destination URL to load.
  [ShellEarlGrey loadURL:firstRedirectURL];
  [[EarlGrey selectElementWithMatcher:AddressFieldText(destinationURL.spec())]
      assertWithMatcher:grey_notNil()];
  [ShellEarlGrey waitForWebViewContainingText:kFinalPageContent];
}

// Tests simple 301 redirection.
- (void)testRedirection301 {
  // Create map of canned responses and set up the test HTML server.
  std::map<GURL, HtmlResponseProviderImpl::Response> responses;
  const GURL firstRedirectURL = HttpServer::MakeUrl("http://firstRedirect/");
  const GURL destinationURL = HttpServer::MakeUrl("http://destination/");

  responses[firstRedirectURL] = HtmlResponseProviderImpl::GetRedirectResponse(
      destinationURL, net::HTTP_MOVED_PERMANENTLY);
  const char kFinalPageContent[] = "testRedirection301 complete";
  responses[destinationURL] =
      HtmlResponseProviderImpl::GetSimpleResponse(kFinalPageContent);
  std::unique_ptr<web::DataResponseProvider> provider(
      new HtmlResponseProvider(responses));
  web::test::SetUpHttpServer(std::move(provider));

  // Load first URL and expect destination URL to load.
  [ShellEarlGrey loadURL:firstRedirectURL];
  [[EarlGrey selectElementWithMatcher:AddressFieldText(destinationURL.spec())]
      assertWithMatcher:grey_notNil()];
  [ShellEarlGrey waitForWebViewContainingText:kFinalPageContent];
}

// Tests simple 302 redirection.
- (void)testRedirection302 {
  // Create map of canned responses and set up the test HTML server.
  std::map<GURL, HtmlResponseProviderImpl::Response> responses;
  const GURL firstRedirectURL = HttpServer::MakeUrl("http://firstRedirect/");
  const GURL destinationURL = HttpServer::MakeUrl("http://destination/");

  responses[firstRedirectURL] = HtmlResponseProviderImpl::GetRedirectResponse(
      destinationURL, net::HTTP_FOUND);
  const char kFinalPageContent[] = "testRedirection302 complete";
  responses[destinationURL] =
      HtmlResponseProviderImpl::GetSimpleResponse(kFinalPageContent);
  std::unique_ptr<web::DataResponseProvider> provider(
      new HtmlResponseProvider(responses));
  web::test::SetUpHttpServer(std::move(provider));

  // Load first URL and expect destination URL to load.
  [ShellEarlGrey loadURL:firstRedirectURL];
  [[EarlGrey selectElementWithMatcher:AddressFieldText(destinationURL.spec())]
      assertWithMatcher:grey_notNil()];
  [ShellEarlGrey waitForWebViewContainingText:kFinalPageContent];
}

@end
