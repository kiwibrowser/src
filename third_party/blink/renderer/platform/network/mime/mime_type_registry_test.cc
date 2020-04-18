// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(MIMETypeRegistryTest, MimeTypeTest) {
  EXPECT_TRUE(MIMETypeRegistry::IsSupportedImagePrefixedMIMEType("image/gif"));
  EXPECT_TRUE(MIMETypeRegistry::IsSupportedImageResourceMIMEType("image/gif"));
  EXPECT_TRUE(MIMETypeRegistry::IsSupportedImagePrefixedMIMEType("Image/Gif"));
  EXPECT_TRUE(MIMETypeRegistry::IsSupportedImageResourceMIMEType("Image/Gif"));
  static const UChar kUpper16[] = {0x0049, 0x006d, 0x0061, 0x0067,
                                   0x0065, 0x002f, 0x0067, 0x0069,
                                   0x0066, 0};  // Image/gif in UTF16
  EXPECT_TRUE(
      MIMETypeRegistry::IsSupportedImagePrefixedMIMEType(String(kUpper16)));
  EXPECT_TRUE(
      MIMETypeRegistry::IsSupportedImagePrefixedMIMEType("image/svg+xml"));
  EXPECT_FALSE(
      MIMETypeRegistry::IsSupportedImageResourceMIMEType("image/svg+xml"));
}

TEST(MIMETypeRegistryTest, PluginMimeTypes) {
  // Since we've removed MIME type guessing based on plugin-declared file
  // extensions, ensure that the MIMETypeRegistry already contains
  // the extensions used by common PPAPI plugins.
  EXPECT_EQ("application/pdf",
            MIMETypeRegistry::GetWellKnownMIMETypeForExtension("pdf").Utf8());
  EXPECT_EQ("application/x-shockwave-flash",
            MIMETypeRegistry::GetWellKnownMIMETypeForExtension("swf").Utf8());
}

}  // namespace blink
