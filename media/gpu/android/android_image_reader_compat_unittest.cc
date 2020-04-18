// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/android_image_reader_compat.h"

#include <stdint.h>
#include <memory>

#include "base/android/build_info.h"
#include "base/test/scoped_feature_list.h"
#include "media/base/media_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class AndroidImageReaderTest : public testing::Test {
 public:
  AndroidImageReaderTest() {
    scoped_feature_list_.InitAndEnableFeature(media::kAImageReaderVideoOutput);
  }
  ~AndroidImageReaderTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Getting instance of AndroidImageReader will invoke AndroidImageReader
// constructor which will dlopen the mediandk and androidndk .so files and do
// all the required symbol lookups.
TEST_F(AndroidImageReaderTest, GetImageReaderInstance) {
  // It is expected that image reader support will be available from android
  // version OREO.
  EXPECT_EQ(AndroidImageReader::GetInstance().IsSupported(),
            base::android::BuildInfo::GetInstance()->sdk_int() >=
                base::android::SDK_VERSION_OREO);
}

// There should be only 1 instance of AndroidImageReader im memory. Hence 2
// instances should have same memory address.
TEST_F(AndroidImageReaderTest, CompareImageReaderInstance) {
  AndroidImageReader& a1 = AndroidImageReader::GetInstance();
  AndroidImageReader& a2 = AndroidImageReader::GetInstance();
  ASSERT_EQ(&a1, &a2);
}

}  // namespace media
