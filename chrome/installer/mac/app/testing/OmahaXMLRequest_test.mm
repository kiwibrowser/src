// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/installer/mac/app/OmahaXMLRequest.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(OmahaXMLRequestTest, CreateReturnsValidXML) {
  NSXMLDocument* xml_body_ = [OmahaXMLRequest createXMLRequestBody];
  ASSERT_TRUE(xml_body_);

  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("chrome/test/data/mac_installer/requestCheck.dtd");
  NSString* requestDTDLocation = base::SysUTF8ToNSString(path.value());
  NSData* requestDTDData = [NSData dataWithContentsOfFile:requestDTDLocation];
  ASSERT_TRUE(requestDTDData);

  NSError* error;
  NSXMLDTD* requestXMLChecker =
      [[NSXMLDTD alloc] initWithData:requestDTDData options:0 error:&error];
  [requestXMLChecker setName:@"request"];
  [xml_body_ setDTD:requestXMLChecker];
  EXPECT_TRUE([xml_body_ validateAndReturnError:&error]);
}

}  // namespace
