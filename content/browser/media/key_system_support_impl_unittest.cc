// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/key_system_support_impl.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/cdm_registry.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/common/cdm_info.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/base/decrypt_config.h"
#include "media/base/video_codecs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

const char kTestCdmGuid[] = "62FE9C4B-384E-48FD-B28A-9F6F248BC8CC";
const char kVersion[] = "1.1.1.1";
const char kTestPath[] = "/aa/bb";
const char kTestFileSystemId[] = "file_system_id";

}  // namespace

class KeySystemSupportTest : public testing::Test {
 protected:
  void SetUp() final {
    DVLOG(1) << __func__;
    KeySystemSupportImpl::Create(mojo::MakeRequest(&key_system_support_));
  }

  // Registers |key_system| with |supported_video_codecs| and
  // |supports_persistent_license|. All other values for CdmInfo have some
  // default value as they're not returned by IsKeySystemSupported().
  void Register(const std::string& key_system,
                const std::vector<media::VideoCodec>& supported_video_codecs,
                bool supports_persistent_license,
                const base::flat_set<media::EncryptionMode>& supported_modes) {
    DVLOG(1) << __func__;

    CdmRegistry::GetInstance()->RegisterCdm(
        CdmInfo(key_system, kTestCdmGuid, base::Version(kVersion),
                base::FilePath::FromUTF8Unsafe(kTestPath), kTestFileSystemId,
                supported_video_codecs, supports_persistent_license,
                supported_modes, key_system, false));
  }

  // Determines if |key_system| is registered. If it is, updates |codecs_|
  // and |persistent_|.
  bool IsSupported(const std::string& key_system) {
    DVLOG(1) << __func__;
    bool is_available = false;
    key_system_support_->IsKeySystemSupported(key_system, &is_available,
                                              &codecs_, &persistent_,
                                              &encryption_schemes_);
    return is_available;
  }

  media::mojom::KeySystemSupportPtr key_system_support_;
  TestBrowserThreadBundle test_browser_thread_bundle_;

  // Updated by IsSupported().
  std::vector<media::VideoCodec> codecs_;
  bool persistent_;
  std::vector<media::EncryptionMode> encryption_schemes_;
};

// Note that as CdmRegistry::GetInstance() is a static, it is shared between
// tests. So use unique key system names in each test below to avoid
// interactions between the tests.

TEST_F(KeySystemSupportTest, NoKeySystems) {
  EXPECT_FALSE(IsSupported("KeySystem1"));
}

TEST_F(KeySystemSupportTest, OneKeySystem) {
  Register("KeySystem2", {media::VideoCodec::kCodecVP8}, true,
           {media::EncryptionMode::kCenc, media::EncryptionMode::kCbcs});
  EXPECT_TRUE(IsSupported("KeySystem2"));
  EXPECT_EQ(1u, codecs_.size());
  EXPECT_EQ(media::VideoCodec::kCodecVP8, codecs_[0]);
  EXPECT_TRUE(persistent_);
  EXPECT_EQ(2u, encryption_schemes_.size());
  EXPECT_EQ(media::EncryptionMode::kCenc, encryption_schemes_[0]);
  EXPECT_EQ(media::EncryptionMode::kCbcs, encryption_schemes_[1]);
}

TEST_F(KeySystemSupportTest, MultipleKeySystems) {
  Register("KeySystem3",
           {media::VideoCodec::kCodecVP8, media::VideoCodec::kCodecVP9}, true,
           {media::EncryptionMode::kCenc});
  Register("KeySystem4", {media::VideoCodec::kCodecVP9}, false,
           {media::EncryptionMode::kCbcs});
  EXPECT_TRUE(IsSupported("KeySystem3"));
  EXPECT_EQ(2u, codecs_.size());
  EXPECT_EQ(media::VideoCodec::kCodecVP8, codecs_[0]);
  EXPECT_EQ(media::VideoCodec::kCodecVP9, codecs_[1]);
  EXPECT_TRUE(persistent_);
  EXPECT_EQ(1u, encryption_schemes_.size());
  EXPECT_EQ(media::EncryptionMode::kCenc, encryption_schemes_[0]);
  EXPECT_TRUE(IsSupported("KeySystem4"));
  EXPECT_EQ(1u, codecs_.size());
  EXPECT_EQ(media::VideoCodec::kCodecVP9, codecs_[0]);
  EXPECT_FALSE(persistent_);
  EXPECT_EQ(1u, encryption_schemes_.size());
  EXPECT_EQ(media::EncryptionMode::kCbcs, encryption_schemes_[0]);
}

TEST_F(KeySystemSupportTest, MissingKeySystem) {
  Register("KeySystem5", {media::VideoCodec::kCodecVP8}, true,
           {media::EncryptionMode::kCenc});
  EXPECT_FALSE(IsSupported("KeySystem6"));
}

}  // namespace content
