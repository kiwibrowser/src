// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "OmahaXMLRequest.h"

#include "SystemInfo.h"

@implementation OmahaXMLRequest : NSObject

+ (NSXMLElement*)createElementWithName:(NSString*)name {
  return [[NSXMLElement alloc] initWithName:name];
}

+ (void)forElement:(NSXMLElement*)element
      AddAttribute:(NSString*)attribute
         WithValue:(NSString*)value {
  [element
      addAttribute:[NSXMLNode attributeWithName:attribute stringValue:value]];
}

// borisv@ indicated that the OS version, platform, appid, and version are the
// user attributes that Omaha actually looks at. The other parameters are useful
// for logging purposes but otherwise not directly used.
+ (NSXMLDocument*)createXMLRequestBody {
  // TODO: This protocol version number probably shouldn't be hard-coded. Check
  // with borisv@ regarding changing protocol verions.
  NSString* protocol = @"3.0";

  NSString* platform = @"mac";
  NSString* operatingSystem = [SystemInfo getOSVersion];
  NSString* architecture = [SystemInfo getArch];
  NSString* plat_arch =
      [NSString stringWithFormat:@"%@_%@", operatingSystem, architecture];

  NSString* appid = @"com.google.Chrome";
  NSString* version = @"0.0.0.0";
  NSString* language = @"en-us";

  NSXMLElement* root = [OmahaXMLRequest createElementWithName:@"request"];
  [OmahaXMLRequest forElement:root AddAttribute:@"protocol" WithValue:protocol];

  NSXMLElement* osChild = [OmahaXMLRequest createElementWithName:@"os"];
  [OmahaXMLRequest forElement:osChild
                 AddAttribute:@"platform"
                    WithValue:platform];
  [OmahaXMLRequest forElement:osChild
                 AddAttribute:@"version"
                    WithValue:operatingSystem];
  [OmahaXMLRequest forElement:osChild
                 AddAttribute:@"arch"
                    WithValue:architecture];
  [OmahaXMLRequest forElement:osChild AddAttribute:@"sp" WithValue:plat_arch];
  [root addChild:osChild];

  NSXMLElement* appChild = [OmahaXMLRequest createElementWithName:@"app"];
  [OmahaXMLRequest forElement:appChild AddAttribute:@"appid" WithValue:appid];
  [OmahaXMLRequest forElement:appChild
                 AddAttribute:@"version"
                    WithValue:version];
  [OmahaXMLRequest forElement:appChild AddAttribute:@"lang" WithValue:language];
  [root addChild:appChild];

  NSXMLElement* updateChildChild =
      [OmahaXMLRequest createElementWithName:@"updatecheck"];
  [appChild addChild:updateChildChild];

  NSXMLDocument* requestXMLDocument =
      [[NSXMLDocument alloc] initWithRootElement:root];
  return requestXMLDocument;
}

@end
