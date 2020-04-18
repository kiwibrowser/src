// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#import <EarlGrey/EarlGrey.h>

#include "base/strings/stringprintf.h"
#import "base/test/ios/wait_util.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#import "ios/web/shell/test/earl_grey/shell_earl_grey.h"
#import "ios/web/shell/test/earl_grey/shell_matchers.h"
#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Loads a web page with given content.
void LoadPage(const std::string& page_content) {
  const GURL url = web::test::HttpServer::MakeUrl("http://plugin");
  std::map<GURL, std::string> responses{{url, page_content}};
  web::test::SetUpSimpleHttpServer(responses);
  [ShellEarlGrey loadURL:url];
}

}  // namespace

// Plugin placeholder test cases for the web shell. These tests verify that web
// page shows a placeholder for unsupported plugins.
@interface PluginPlaceholderTestCase : WebShellTestCase
@end

@implementation PluginPlaceholderTestCase

// Tests that a large <applet> with text fallback is untouched.
- (void)testPluginPlaceholderAppletFallback {
  const char kPageDescription[] = "Applet, text fallback";
  const char kFallbackText[] = "Java? On iOS? C'mon.";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      "<applet code='Some.class' width='550' height='550'>"
      "  <p>%s</p>"
      "</applet>"
      "</body></html>",
      kPageDescription, kFallbackText);
  LoadPage(page);

  // Verify that placeholder image is not displayed.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewContainingText:kFallbackText];
  [ShellEarlGrey waitForWebViewNotContainingCSSSelector:"img"];
}

// Tests placeholder for a large <applet> with no fallback.
- (void)testPluginPlaceholderAppletOnly {
  const char kPageDescription[] = "Applet, no fallback";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      "<applet code='Some.class' width='550' height='550'>"
      "</applet>"
      "</body></html>",
      kPageDescription);
  LoadPage(page);

  // Verify that plugin object is replaced with placeholder image.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewContainingCSSSelector:"img[src*='data']"];
}

// Tests placeholder for a large <object> with a flash embed fallback.
- (void)testPluginPlaceholderObjectFlashEmbedFallback {
  const char kPageDescription[] = "Object, embed fallback";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      "<object classid='clsid:D27CDB6E-AE6D-11cf-96B8-444553540000'"
      "    codebase='http://download.macromedia.com/pub/shockwave/cabs/'"
      "flash/swflash.cab#version=6,0,0,0' width='550' height='550'>"
      "  <param name='movie' value='some.swf'>"
      "  <embed src='some.swf' type='application/x-shockwave-flash' "
      "width='550' height='550'>"
      "</object>"
      "</body></html>",
      kPageDescription);
  LoadPage(page);

  // Verify that plugin object is replaced with placeholder image.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewContainingCSSSelector:"img[src*='data']"];
}

// Tests that a large <object> with an embed fallback of unspecified type is
// untouched.
- (void)testPluginPlaceholderObjectUndefinedEmbedFallback {
  const char kPageDescription[] = "Object, embed fallback";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      "<object classid='clsid:D27CDB6E-AE6D-11cf-96B8-444553540000'"
      "    codebase='http://download.macromedia.com/pub/shockwave/cabs/'"
      "flash/swflash.cab#version=6,0,0,0' width='550' height='550'>"
      "  <param name='movie' value='some.swf'>"
      "  <embed src='some.swf' width='550' height='550'>"
      "</object>"
      "</body></html>",
      kPageDescription);
  LoadPage(page);

  // Verify that placeholder image is not displayed.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewNotContainingCSSSelector:"img"];
}

// Tests that a large <object> with text fallback is untouched.
- (void)testPluginPlaceholderObjectFallback {
  const char kPageDescription[] = "Object, text fallback";
  const char kFallbackText[] = "You don't have Flash. Tough luck!";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      "<object type='application/x-shockwave-flash' data='some.sfw'"
      "    width='550' height='550'>"
      "  <param name='movie' value='some.swf'>"
      "  <p>%s</p>"
      "</object>"
      "</body></html>",
      kPageDescription, kFallbackText);
  LoadPage(page);

  // Verify that placeholder image is not displayed.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewContainingText:kFallbackText];
  [ShellEarlGrey waitForWebViewNotContainingCSSSelector:"img"];
}

// Tests placeholder for a large <object> with no fallback.
- (void)testPluginPlaceholderObjectOnly {
  const char kPageDescription[] = "Object, no fallback";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      "<object type='application/x-shockwave-flash' data='some.swf'"
      "    width='550' height='550'>"
      "</object>"
      "</body></html>",
      kPageDescription);
  LoadPage(page);

  // Verify that plugin object is replaced with placeholder image.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewContainingCSSSelector:"img[src*='data']"];
}

// Tests that a large png <object> is untouched.
- (void)testPluginPlaceholderPNGObject {
  const char kPageDescription[] = "PNG object";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      "<object data='foo.png' type='image/png' width='550' height='550'>"
      "</object>"
      "</body></html>",
      kPageDescription);
  LoadPage(page);

  // Verify that placeholder image is not displayed.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewNotContainingCSSSelector:"img"];
}

// Test that non-major plugins (e.g., top/side ads) don't get placeholders.
- (void)testPluginPlaceholderSmallFlash {
  const char kPageDescription[] = "Flash ads";
  const std::string page = base::StringPrintf(
      "<html><body width='800' height='600'>"
      "<p>%s</p>"
      // 160x600 "skyscraper"
      "<object classid='clsid:D27CDB6E-AE6D-11cf-96B8-444553540000'"
      "    codebase='http://download.macromedia.com/pub/shockwave/cabs/'"
      "flash/swflash.cab#version=6,0,0,0' width='160' height='600'>"
      "  <param name='movie' value='some.swf'>"
      "  <embed src='some.swf' width='160' height='600'>"
      "</object>"
      // 468x60 "full banner"
      "<object classid='clsid:D27CDB6E-AE6D-11cf-96B8-444553540000'"
      "    codebase='http://download.macromedia.com/pub/shockwave/cabs/'"
      "flash/swflash.cab#version=6,0,0,0' width='468' height='60'>"
      "  <param name='movie' value='some.swf'>"
      "  <embed src='some.swf' width='468' height='60'>"
      "</object>"
      // 728x90 "leaderboard"
      "<object classid='clsid:D27CDB6E-AE6D-11cf-96B8-444553540000'"
      "    codebase='http://download.macromedia.com/pub/shockwave/cabs/'"
      "flash/swflash.cab#version=6,0,0,0' width='728' height='90'>"
      "  <param name='movie' value='some.swf'>"
      "  <embed src='some.swf' width='728' height='90'>"
      "</object>"
      "</body></html>",
      kPageDescription);
  LoadPage(page);

  // Verify that placeholder image is not displayed.
  [ShellEarlGrey waitForWebViewContainingText:kPageDescription];
  [ShellEarlGrey waitForWebViewNotContainingCSSSelector:"img"];
}

@end
