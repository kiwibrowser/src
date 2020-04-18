// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "OmahaCommunication.h"

#import "OmahaXMLRequest.h"
#import "OmahaXMLParser.h"

// TODO: Turn the below string into a command line flag for testing.
static NSString* const omahaURLPath =
    @"https://tools.google.com/service/update2";

@interface NSURLSession ()
- (NSURLSessionDataTask*)dataTaskWithRequest:(NSURLRequest*)request
                           completionHandler:
                               (void (^)(NSData* data,
                                         NSURLResponse* response,
                                         NSError* error))completionHandler;
@end

@implementation OmahaCommunication

@synthesize requestXMLBody = requestXMLBody_;
@synthesize delegate = delegate_;

- (id)init {
  return [self initWithBody:[OmahaXMLRequest createXMLRequestBody]];
}

- (id)initWithBody:(NSXMLDocument*)xmlBody {
  if ((self = [super init])) {
    requestXMLBody_ = xmlBody;
  }
  return self;
}

- (void)fetchDownloadURLs {
  // Forming the request
  NSURL* requestURL = [NSURL URLWithString:omahaURLPath];
  NSMutableURLRequest* request =
      [NSMutableURLRequest requestWithURL:requestURL];
  [request addValue:@"text/xml" forHTTPHeaderField:@"Content-Type"];
  NSData* requestBody =
      [[requestXMLBody_ XMLString] dataUsingEncoding:NSUTF8StringEncoding];
  request.HTTPBody = requestBody;
  request.HTTPMethod = @"POST";
  // Sending the request
  [[[NSURLSession sharedSession]
      dataTaskWithRequest:request
        completionHandler:^(NSData* data, NSURLResponse* response,
                            NSError* error) {
          NSArray* completeURLs = nil;
          if (!error) {
            completeURLs = [OmahaXMLParser parseXML:data error:&error];
          }
          // Deals with errors both from the network error and the
          // parsing error, as the user only needs to know there was a problem
          // talking with the Google Update server.
          if (error) {
            [delegate_ omahaCommunication:self onFailure:error];
          } else {
            [delegate_ omahaCommunication:self onSuccess:completeURLs];
          }
        }] resume];
}

@end
