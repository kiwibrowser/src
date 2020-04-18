// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "OmahaXMLParser.h"

@interface OmahaXMLParser ()<NSXMLParserDelegate>
@end

@implementation OmahaXMLParser {
  NSMutableArray* chromeIncompleteDownloadURLs_;
  NSString* chromeImageFilename_;
}

// Sets up instance of NSXMLParser and calls on delegate methods to do actual
// parsing work.
+ (NSArray*)parseXML:(NSData*)omahaResponseXML error:(NSError**)error {
  NSXMLParser* parser = [[NSXMLParser alloc] initWithData:omahaResponseXML];

  OmahaXMLParser* omahaParser = [[OmahaXMLParser alloc] init];
  [parser setDelegate:omahaParser];
  if (![parser parse]) {
    *error = [parser parserError];
    return nil;
  }

  NSMutableArray* completeDownloadURLs = [[NSMutableArray alloc] init];
  for (NSString* URL in omahaParser->chromeIncompleteDownloadURLs_) {
    [completeDownloadURLs
        addObject:[NSURL URLWithString:omahaParser->chromeImageFilename_
                         relativeToURL:[NSURL URLWithString:URL]]];
  }

  if ([completeDownloadURLs count] < 1) {
    // TODO: The below error exists only so the caller of this method would
    // catch the error created here. A better way to handle this is to make the
    // error's contents inform what the installer will try next when it attempts
    // to recover from an issue.
    *error = [NSError errorWithDomain:@"ChromeErrorDomain" code:1 userInfo:nil];
    return nil;
  }

  return completeDownloadURLs;
}

// Searches the XML data for the tag "URL" and the subsequent "codebase"
// attribute that indicates a URL follows. Copies each URL into an array.
// NOTE: The URLs in the XML file are incomplete. They need the filename
// appended to end. The second if statement checks for the tag "package" which
// contains the filename needed to complete the URLs.
- (void)parser:(NSXMLParser*)parser
    didStartElement:(NSString*)elementName
       namespaceURI:(NSString*)namespaceURI
      qualifiedName:(NSString*)qName
         attributes:(NSDictionary*)attributeDict {
  if ([elementName isEqualToString:@"url"]) {
    if (!chromeIncompleteDownloadURLs_) {
      chromeIncompleteDownloadURLs_ = [[NSMutableArray alloc] init];
    }
    NSString* extractedURL = [attributeDict objectForKey:@"codebase"];
    [chromeIncompleteDownloadURLs_ addObject:extractedURL];
  }
  if ([elementName isEqualToString:@"package"]) {
    chromeImageFilename_ =
        [[NSString alloc] initWithString:[attributeDict objectForKey:@"name"]];
  }
}

// If either component of the URL is empty then the complete URL cannot
// be generated so both variables are set to nil to flag errors.
- (void)parserDidEndDocument:(NSXMLParser*)parser {
  if (!chromeIncompleteDownloadURLs_ || !chromeImageFilename_) {
    chromeIncompleteDownloadURLs_ = nil;
    chromeImageFilename_ = nil;
  }
}

@end
