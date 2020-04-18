// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/webui/crw_web_ui_page_builder.h"

#import <Foundation/Foundation.h>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// HTML page template.
NSString* const kPageTemplate = @"<html><head>%@</head></html>";

// URL for BuildSimplePage.
const char kSimplePageUrl[] = "http://simplepage/";
// URL for BuildPageWithJSSubresource.
const char kJsPageUrl[] = "http://javascriptpage/";
// URL for BuildPageWithCSSSubresource.
const char kCssPageUrl[] = "http://csspage/";
// URL for BuildPageWithMultipleSubresources.
const char kMultipleResourcesPageUrl[] = "http://multipleresourcespage/";
// URL for BuildPageWithCSSNestedSubresources.
const char kCssImportPageUrl[] = "http://csspagewithimport/";
// URL for BuildPageWithRelativeSubresource.
const char kCssRelativePageUrl[] = "http://css/csspage.html";
// URL for BuildPageWithWebUIJS.
const char kCoreJsPageUrl[] = "http://corejs/";

// URL for JS resource.
const char kJsResourceUrl[] = "http://javascriptpage/resources/javascript.js";
// URL for CSS resource.
const char kCssResourceUrl[] = "http://csspage/resources/stylesheet.css";
// URL for CSS resource with import.
const char kCssImportResourceUrl[] = "http://csspage/resources/import.css";
// Chrome URL for messaging JavaScript.
const char kCoreJsResourceUrl[] = "chrome://resources/js/ios/web_ui.js";
// String for relative resource URL.
const char kRelativeCssString[] = "myresource.css";

// Template for JS tag with URL.
NSString* const kJsTagTemplate = @"<script src=\"%@\"></script>";
// Template for inlined JS tag.
NSString* const kJsInlinedTemplate = @"<script>%@</script>";
// Template for CSS tag with URL.
NSString* const kCssTagTemplate = @"<link rel=\"stylesheet\" href=\"%@\">";
// Template for inlined CSS tag.
NSString* const kCssInlinedTemplate = @"<style>%@</style>";
// Template for CSS with import statement.
NSString* const kCssImportTemplate = @"@import url(%@); b {diplay:block;}";

// Content for JS resource.
NSString* const kJsContent = @"console.log('This is JavaScript');";
// Content for CSS resource.
NSString* const kCssContent = @"html {height:100%;}";
// Content for relative CSS resource.
NSString* const kRelativeCssContent = @"mytag {someprop:1}";
// Dummy content for WebUI messaging JavaScript.
NSString* kCoreJsContent = @"console.log('messaging javascript');";

// Returns HTML string containing tag formed with tag_template and content.
NSString* PageForTagTemplateAndContent(NSString* tag_template,
                                       NSString* content) {
  NSString* tag = [NSString stringWithFormat:tag_template, content];
  return [NSString stringWithFormat:kPageTemplate, tag];
}

}  // namespace

// Mock subclass of CRWWebUIPageBuilder for testing.
@interface MockCRWWebUIPageBuilder : CRWWebUIPageBuilder
// Stub out webUIJavaScript method so resource bundle access is not needed.
- (NSString*)webUIJavaScript;
@end

@implementation MockCRWWebUIPageBuilder
- (NSString*)webUIJavaScript {
  return kCoreJsContent;
}
@end

// Mock CRWWebUIPageBuilderDelegate to serve responses for resource requests.
@interface MockPageBuilderDelegate : NSObject<CRWWebUIPageBuilderDelegate>
// Returns HTML page containing tag formed from tagTemplate and URL.
- (NSString*)pageWithTagTemplate:(NSString*)tagTemplate URL:(const char*)URL;
// Returns resource string for resourceURL.
- (NSString*)resourceForURL:(const GURL&)resourceURL;
@end

@implementation MockPageBuilderDelegate

- (void)webUIPageBuilder:(CRWWebUIPageBuilder*)webUIPageBuilder
    fetchResourceWithURL:(const GURL&)resourceURL
       completionHandler:(web::WebUIDelegateCompletion)completionHandler {
  completionHandler([self resourceForURL:resourceURL], resourceURL);
}

- (NSString*)resourceForURL:(const GURL&)resourceURL {
  // Resource for BuildSimplePage.
  if (resourceURL == kSimplePageUrl)
    return [NSString stringWithFormat:kPageTemplate, @""];
  // Resources for BuildPageWithJSSubresource.
  if (resourceURL == kJsPageUrl)
    return [self pageWithTagTemplate:kJsTagTemplate URL:kJsResourceUrl];
  if (resourceURL == kJsResourceUrl)
    return kJsContent;
  // Resources for BuildPageWithCSSSubresource.
  if (resourceURL == kCssPageUrl)
    return [self pageWithTagTemplate:kCssTagTemplate URL:kCssResourceUrl];
  if (resourceURL == kCssResourceUrl)
    return kCssContent;
  // Resource for BuildPageWithMultipleSubresources.
  if (resourceURL == kMultipleResourcesPageUrl) {
    NSString* JSTag =
        [NSString stringWithFormat:kJsTagTemplate,
                                   base::SysUTF8ToNSString(kJsResourceUrl)];
    NSString* CSSTag =
        [NSString stringWithFormat:kCssTagTemplate,
                                   base::SysUTF8ToNSString(kCssResourceUrl)];
    NSString* CoreJSTag =
        [NSString stringWithFormat:kJsTagTemplate,
                                   base::SysUTF8ToNSString(kCoreJsResourceUrl)];
    NSString* tags = [[JSTag stringByAppendingString:CSSTag]
        stringByAppendingString:CoreJSTag];
    return [NSString stringWithFormat:kPageTemplate, tags];
  }
  // Resources for BuildPageWithCSSNestedSubresource.
  if (resourceURL == kCssImportPageUrl) {
    return [self pageWithTagTemplate:kCssTagTemplate URL:kCssImportResourceUrl];
  }
  if (resourceURL == kCssImportResourceUrl) {
    return [NSString stringWithFormat:kCssImportTemplate,
                                      base::SysUTF8ToNSString(kCssResourceUrl)];
  }
  // Resources for BuildPageWithRelativeSubresource.
  GURL relativePageURL(kCssRelativePageUrl);
  GURL cssRelativeResourceURL = relativePageURL.Resolve(kRelativeCssString);
  if (resourceURL == relativePageURL) {
    return [self pageWithTagTemplate:kCssTagTemplate URL:kRelativeCssString];
  }
  if (resourceURL == cssRelativeResourceURL)
    return kRelativeCssContent;
  // Resource for BuildPageWithWebUIJS.
  if (resourceURL == kCoreJsPageUrl)
    return [self pageWithTagTemplate:kJsTagTemplate URL:kCoreJsResourceUrl];

  NOTREACHED();
  return nil;
}

- (NSString*)pageWithTagTemplate:(NSString*)tagTemplate URL:(const char*)URL {
  NSString* URLString = base::SysUTF8ToNSString(URL);
  NSString* tag = [NSString stringWithFormat:tagTemplate, URLString];
  return [NSString stringWithFormat:kPageTemplate, tag];
}

@end

namespace web {

class CRWWebUIPageBuilderTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    delegate_ = [[MockPageBuilderDelegate alloc] init];
    web_ui_page_builder_ =
        [[MockCRWWebUIPageBuilder alloc] initWithDelegate:delegate_];
  }
  // CRWWebUIPageBuilder for testing.
  MockCRWWebUIPageBuilder* web_ui_page_builder_;
  // Delegate for test CRWWebUIPageBuilder.
  MockPageBuilderDelegate* delegate_;
};

// Tests that a page without imports is passed to completion handler unchanged.
TEST_F(CRWWebUIPageBuilderTest, BuildSimplePage) {
  NSString* simple_page_html = [NSString stringWithFormat:kPageTemplate, @""];
  [web_ui_page_builder_ buildWebUIPageForURL:GURL(kSimplePageUrl)
                           completionHandler:^(NSString* result) {
                             EXPECT_NSEQ(simple_page_html, result);
                           }];
}

// Tests that a page with a JavaScript subresource is passed to the completion
// handler with the resource inlined properly, i.e. <script
// src="http://somejs.js"></script> becomes <script>some javascript;</script>.
TEST_F(CRWWebUIPageBuilderTest, BuildPageWithJSSubresource) {
  NSString* js_page_html =
      PageForTagTemplateAndContent(kJsInlinedTemplate, kJsContent);
  web::WebUIPageCompletion completionHandler = ^(NSString* result) {
    EXPECT_NSEQ(js_page_html, result);
  };
  [web_ui_page_builder_ buildWebUIPageForURL:GURL(kJsPageUrl)
                           completionHandler:completionHandler];
}

// Tests that a page with a CSS subresource is passed to the completion handler
// with the resource inlined properly, i.e. <link rel="stylesheet"
// href="http://somecss.css"/> becomes <style>some css</style>.
TEST_F(CRWWebUIPageBuilderTest, BuildPageWithCSSSubresource) {
  NSString* css_page_html =
      PageForTagTemplateAndContent(kCssInlinedTemplate, kCssContent);
  [web_ui_page_builder_ buildWebUIPageForURL:GURL(kCssPageUrl)
                           completionHandler:^(NSString* result) {
                             EXPECT_NSEQ(css_page_html, result);
                           }];
}

TEST_F(CRWWebUIPageBuilderTest, BuildPageWithMultipleSubresources) {
  NSString* js_tag = [NSString stringWithFormat:kJsInlinedTemplate, kJsContent];
  NSString* css_tag =
      [NSString stringWithFormat:kCssInlinedTemplate, kCssContent];
  NSString* core_js_tag =
      [NSString stringWithFormat:kJsInlinedTemplate, kCoreJsContent];
  NSString* tags = [[js_tag stringByAppendingString:css_tag]
      stringByAppendingString:core_js_tag];
  NSString* multiple_resources_html =
      [NSString stringWithFormat:kPageTemplate, tags];
  [web_ui_page_builder_ buildWebUIPageForURL:GURL(kMultipleResourcesPageUrl)
                           completionHandler:^(NSString* result) {
                             EXPECT_NSEQ(multiple_resources_html, result);
                           }];
}

// Tests that a page with a CSS subresource that contains an @import statement
// for another CSS subresource is passed to the completion handler with the
// resource inlined properly, i.e. if somecss.css from above has the statement
// @import url(morecss.css), the original tag becomes <style>contents of
// morecss</style><style>some css</style>.
TEST_F(CRWWebUIPageBuilderTest, BuildPageWithCSSNestedSubresource) {
  NSString* css_inlined_tag =
      [NSString stringWithFormat:kCssInlinedTemplate, kCssContent];
  NSString* css_inlined_content =
      [NSString stringWithFormat:kCssImportTemplate,
                                 base::SysUTF8ToNSString(kCssResourceUrl)];
  NSString* css_import_inlined_tag =
      [NSString stringWithFormat:kCssInlinedTemplate, css_inlined_content];
  NSString* tags =
      [css_inlined_tag stringByAppendingString:css_import_inlined_tag];
  NSString* css_import_page_html =
      [NSString stringWithFormat:kPageTemplate, tags];
  [web_ui_page_builder_ buildWebUIPageForURL:GURL(kCssImportPageUrl)
                           completionHandler:^(NSString* result) {
                             EXPECT_NSEQ(css_import_page_html, result);
                           }];
}

// Tests that a page with a relative subresource is properly resolved.
TEST_F(CRWWebUIPageBuilderTest, BuildPageWithRelativeSubresource) {
  NSString* css_page_html =
      PageForTagTemplateAndContent(kCssInlinedTemplate, kRelativeCssContent);
  [web_ui_page_builder_ buildWebUIPageForURL:GURL(kCssRelativePageUrl)
                           completionHandler:^(NSString* result) {
                             EXPECT_NSEQ(css_page_html, result);
                           }];
}

// Tests that the JavaScript for connecting WebUI messaging to web controller
// messaging is properly inlined.
TEST_F(CRWWebUIPageBuilderTest, BuildPageWithWebUIJS) {
  NSString* core_js_html =
      PageForTagTemplateAndContent(kJsInlinedTemplate, kCoreJsContent);
  [web_ui_page_builder_ buildWebUIPageForURL:GURL(kCoreJsPageUrl)
                           completionHandler:^(NSString* result) {
                             EXPECT_NSEQ(core_js_html, result);
                           }];
}

}  //  namespace web
