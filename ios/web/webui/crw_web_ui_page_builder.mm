// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/webui/crw_web_ui_page_builder.h"

#include <map>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Prefix for script tags. Used to locate JavaScript subresources.
NSString* const kJSTagPrefix = @"<script src=\"";
// Suffix for script tags. Used to locate JavaScript subresources.
NSString* const kJSTagSuffix = @"\"></script>";
// Prefix for stylesheet tags. Used to locate CSS subresources.
NSString* const kCSSTagPrefix = @"<link rel=\"stylesheet\" href=\"";
// Suffix for stylesheet tags. Used to locate CSS subresources.
NSString* const kCSSTagSuffix = @"\">";
// Template for creating inlined JavaScript tags.
NSString* const kWebUIScriptTextTemplate = @"<script>%@</script>";
// Template for creating inlined CSS tags.
NSString* const kWebUIStyleTextTemplate = @"<style>%@</style>";
// URL placeholder for WebUI messaging JavaScript.
NSString* const kWebUIJSURL = @"chrome://resources/js/ios/web_ui.js";
}  // namespace

@interface CRWWebUIPageBuilder ()

// Builds WebUI page for URL with HTML as default resource.
- (void)buildWebUIPageForHTML:(NSString*)HTML
                     webUIURL:(const GURL&)URL
            completionHandler:(web::WebUIPageCompletion)completionHandler;

// Loads |resourceURL| by invoking _delegate.
- (void)fetchResourceWithURL:(const GURL&)resourceURL
           completionHandler:(web::WebUIDelegateCompletion)handler;

// Loads |resourceURLs| by invoking _delegate. Members of resourceURLs must be
// valid subresource URLs. |completionHandler| is called upon load of each
// resource.
- (void)fetchSubresourcesWithURLs:(const std::vector<GURL>&)resourceURLs
                completionHandler:(web::WebUIDelegateCompletion)handler;

// Looks for substrings surrounded by |prefix| and |suffix| in resource and
// returns them as an NSSet.
// For example, if prefix is "<a href='", suffix is "'>", and |resource|
// includes substrings "<a href='http://www.apple.com'>" and
// "<a href='chrome.html'>", return value will contain strings
// "http://www.apple.com" and "chrome.html".
- (NSSet*)URLStringsFromResource:(NSString*)resource
                          prefix:(NSString*)prefix
                          suffix:(NSString*)suffix;

// Returns URL strings for subresources found in |HTML|. URLs are found by
// searching for tags of the format <link rel="stylesheet" href="URLString">
// and <script src="URLString">.
- (NSSet*)URLStringsFromHTML:(NSString*)HTML;

// Returns URL strings for subresources found in |CSS|. URLs are found by
// searching for statements of the format @import(URLString).
- (NSSet*)URLStringsFromCSS:(NSString*)CSS;

// YES if subresourceURL is a valid subresource URL. Valid subresource URLs
// include files with extension ".js" and ".css".
- (BOOL)isValidSubresourceURL:(const GURL&)subresourceURL;

// YES if subresourceURL is for a CSS resource.
- (BOOL)isCSSSubresourceURL:(const GURL&)subresourceURL;

// Prepends "<link rel="stylesheet" href="|URL|">" to the HTML link tag with
// href=|sourceURL| in |HTML|.
- (void)addCSSTagToHTML:(NSMutableString*)HTML
                 forURL:(const GURL&)URL
              sourceURL:(const GURL&)sourceURL;

// Flattens HTML with provided map of URLs to resource strings.
- (void)flattenHTML:(NSMutableString*)HTML
    withSubresources:(const std::map<GURL, std::string>&)subresources;

// Returns JavaScript needed for bridging WebUI chrome.send() messages to
// the core.js defined message delivery system.
- (NSString*)webUIJavaScript;

@end

@implementation CRWWebUIPageBuilder {
  // Delegate for requesting resources.
  __weak id<CRWWebUIPageBuilderDelegate> _delegate;
}

#pragma mark - Public Methods

- (instancetype)initWithDelegate:(id<CRWWebUIPageBuilderDelegate>)delegate {
  if (self = [super init]) {
    _delegate = delegate;
  }
  return self;
}

- (void)buildWebUIPageForURL:(const GURL&)webUIURL
           completionHandler:(web::WebUIPageCompletion)completionHandler {
  [self fetchResourceWithURL:webUIURL
           completionHandler:^(NSString* webUIHTML, const GURL& URL) {
             DCHECK(webUIHTML);
             [self buildWebUIPageForHTML:webUIHTML
                                webUIURL:URL
                       completionHandler:completionHandler];
           }];
}

#pragma mark - Private Methods

- (void)buildWebUIPageForHTML:(NSString*)HTML
                     webUIURL:(const GURL&)pageURL
            completionHandler:(web::WebUIPageCompletion)completionHandler {
  __block NSMutableString* webUIHTML = [HTML mutableCopy];
  NSSet* subresourceURLStrings = [self URLStringsFromHTML:webUIHTML];
  __block NSUInteger pendingSubresourceCount = [subresourceURLStrings count];
  if (!pendingSubresourceCount) {
    completionHandler(webUIHTML);
    return;
  }
  __block std::map<GURL, std::string> subresources;
  __weak CRWWebUIPageBuilder* weakSelf = self;
  // Completion handler for subresource loads.
  __block __weak web::WebUIDelegateCompletion weakSubresourceHandler = nil;
  web::WebUIDelegateCompletion subresourceHandler = [^(
      NSString* subresource, const GURL& subresourceURL) {
    DCHECK(subresource);
    // Import statements in CSS resources are also loaded.
    if ([self isCSSSubresourceURL:subresourceURL]) {
      NSSet* URLStrings = [weakSelf URLStringsFromCSS:subresource];
      for (NSString* URLString in URLStrings) {
        GURL URL(subresourceURL.Resolve(base::SysNSStringToUTF8(URLString)));
        [weakSelf addCSSTagToHTML:webUIHTML
                           forURL:URL
                        sourceURL:subresourceURL];
        ++pendingSubresourceCount;
        [weakSelf fetchResourceWithURL:URL
                     completionHandler:weakSubresourceHandler];
      }
    }
    subresources[subresourceURL] = base::SysNSStringToUTF8(subresource);
    --pendingSubresourceCount;
    // When subresource loading is complete, flatten the default resource
    // and invoke the completion handler.
    if (!pendingSubresourceCount) {
      [weakSelf flattenHTML:webUIHTML withSubresources:subresources];
      completionHandler(webUIHTML);
    }
  } copy];

  weakSubresourceHandler = subresourceHandler;

  for (NSString* URLString in subresourceURLStrings) {
    // chrome://resources/js/ios/web_ui.js is skipped because it is
    // retrieved via webUIJavaScript rather than the net stack.
    if ([URLString isEqualToString:kWebUIJSURL]) {
      --pendingSubresourceCount;
      if (!pendingSubresourceCount) {
        [weakSelf flattenHTML:webUIHTML withSubresources:subresources];
        completionHandler(webUIHTML);
      }
      continue;
    }
    GURL URL(pageURL.Resolve(base::SysNSStringToUTF8(URLString)));
    // If the resolved URL is different from URLString, replace URLString in
    // webUIHTML so that it will be located appropriately when flattening
    // occurs.
    if (URL.spec() != base::SysNSStringToUTF8(URLString)) {
      [webUIHTML replaceOccurrencesOfString:URLString
                                 withString:base::SysUTF8ToNSString(URL.spec())
                                    options:0
                                      range:NSMakeRange(0, [HTML length])];
    }
    [self fetchResourceWithURL:URL completionHandler:subresourceHandler];
  }
}

- (void)fetchResourceWithURL:(const GURL&)resourceURL
           completionHandler:(web::WebUIDelegateCompletion)handler {
  [_delegate webUIPageBuilder:self
         fetchResourceWithURL:resourceURL
            completionHandler:handler];
}

- (void)fetchSubresourcesWithURLs:(const std::vector<GURL>&)resourceURLs
                completionHandler:(web::WebUIDelegateCompletion)handler {
  for (const GURL& URL : resourceURLs) {
    DCHECK([self isValidSubresourceURL:URL]);
    [self fetchResourceWithURL:URL completionHandler:handler];
  }
}

- (NSSet*)URLStringsFromResource:(NSString*)resource
                          prefix:(NSString*)prefix
                          suffix:(NSString*)suffix {
  DCHECK(resource);
  DCHECK(prefix);
  DCHECK(suffix);
  NSMutableSet* URLStrings = [NSMutableSet set];
  if (!resource) {
    return URLStrings;
  }
  NSError* error = nil;
  NSString* tagPattern = [NSString
      stringWithFormat:@"%@(.*?)%@",
                       [NSRegularExpression escapedPatternForString:prefix],
                       [NSRegularExpression escapedPatternForString:suffix]];
  NSRegularExpression* tagExpression = [NSRegularExpression
      regularExpressionWithPattern:tagPattern
                           options:NSRegularExpressionCaseInsensitive
                             error:&error];
  if (error) {
    DLOG(WARNING) << "Error: " << base::SysNSStringToUTF8(error.description);
    return URLStrings;
  }
  NSArray* matches =
      [tagExpression matchesInString:resource
                             options:0
                               range:NSMakeRange(0, [resource length])];
  for (NSTextCheckingResult* match in matches) {
    NSRange matchRange = [match rangeAtIndex:1];
    DCHECK(matchRange.length);
    NSString* URLString = [resource substringWithRange:matchRange];
    [URLStrings addObject:URLString];
  }
  return URLStrings;
}

- (NSSet*)URLStringsFromHTML:(NSString*)HTML {
  NSSet* JS = [self URLStringsFromResource:HTML
                                    prefix:kJSTagPrefix
                                    suffix:kJSTagSuffix];
  NSSet* CSS = [self URLStringsFromResource:HTML
                                     prefix:kCSSTagPrefix
                                     suffix:kCSSTagSuffix];
  return [JS setByAddingObjectsFromSet:CSS];
}

- (NSSet*)URLStringsFromCSS:(NSString*)CSS {
  NSString* prefix = @"@import url(";
  NSString* suffix = @");";
  return [self URLStringsFromResource:CSS prefix:prefix suffix:suffix];
}

- (BOOL)isValidSubresourceURL:(const GURL&)subresourceURL {
  base::FilePath resourcePath(subresourceURL.ExtractFileName());
  std::string extension = resourcePath.Extension();
  return extension == ".css" || extension == ".js";
}

- (BOOL)isCSSSubresourceURL:(const GURL&)subresourceURL {
  base::FilePath resourcePath(subresourceURL.ExtractFileName());
  std::string extension = resourcePath.Extension();
  return extension == ".css";
}

- (void)addCSSTagToHTML:(NSMutableString*)HTML
                 forURL:(const GURL&)URL
              sourceURL:(const GURL&)sourceURL {
  NSString* URLString = base::SysUTF8ToNSString(URL.spec());
  NSString* sourceURLString = base::SysUTF8ToNSString(sourceURL.spec());
  NSString* sourceTag =
      [NSString stringWithFormat:@"%@%@%@", kCSSTagPrefix, sourceURLString,
                                 kCSSTagSuffix];
  NSString* extendedTag = [[NSString
      stringWithFormat:@"%@%@%@", kCSSTagPrefix, URLString, kCSSTagSuffix]
      stringByAppendingString:sourceTag];
  [HTML replaceOccurrencesOfString:sourceTag
                        withString:extendedTag
                           options:0
                             range:NSMakeRange(0, [HTML length])];
}

- (void)flattenHTML:(NSMutableString*)HTML
    withSubresources:(const std::map<GURL, std::string>&)subresources {
  // Add core.js script to resources.
  // TODO(crbug.com/487000): Move inclusion of this resource into WebUI
  // implementation rather than forking each HTML file.
  GURL webUIJSURL("chrome://resources/js/ios/web_ui.js");
  std::map<GURL, std::string> resources(subresources);
  resources[webUIJSURL] = base::SysNSStringToUTF8([self webUIJavaScript]);
  NSString* linkTemplateCSS =
      [NSString stringWithFormat:@"%@%%@%@", kCSSTagPrefix, kCSSTagSuffix];
  NSString* linkTemplateJS =
      [NSString stringWithFormat:@"%@%%@%@", kJSTagPrefix, kJSTagSuffix];
  for (auto it = resources.begin(); it != resources.end(); it++) {
    NSString* linkTemplate = @"";
    NSString* textTemplate = @"";
    if ([self isCSSSubresourceURL:it->first]) {
      linkTemplate = linkTemplateCSS;
      textTemplate = kWebUIStyleTextTemplate;
    } else {  // JavaScript.
      linkTemplate = linkTemplateJS;
      textTemplate = kWebUIScriptTextTemplate;
    }
    NSString* resourceURLString = base::SysUTF8ToNSString(it->first.spec());
    NSString* linkTag =
        [NSString stringWithFormat:linkTemplate, resourceURLString];
    NSString* resource = base::SysUTF8ToNSString(it->second);
    NSString* textTag = [NSString stringWithFormat:textTemplate, resource];
    [HTML replaceOccurrencesOfString:linkTag
                          withString:textTag
                             options:0
                               range:NSMakeRange(0, [HTML length])];
  }
}

- (NSString*)webUIJavaScript {
  NSBundle* bundle = base::mac::FrameworkBundle();
  NSString* path = [bundle pathForResource:@"web_ui_bundle" ofType:@"js"];
  DCHECK(path) << "web_ui_bundle.js file not found";
  return [NSString stringWithContentsOfFile:path
                                   encoding:NSUTF8StringEncoding
                                      error:nil];
}

@end
