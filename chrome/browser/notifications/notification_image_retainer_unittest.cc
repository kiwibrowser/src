// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_image_retainer.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace {

const char kProfileId1[] = "Default";
const char kProfileId2[] = "User";

}  // namespace

class NotificationImageRetainerTest : public ::testing::Test {
 public:
  NotificationImageRetainerTest() = default;
  ~NotificationImageRetainerTest() override = default;

  void SetUp() override {
    NotificationImageRetainer::OverrideTempFileLifespanForTesting(true);
  }

  void TearDown() override {
    NotificationImageRetainer::OverrideTempFileLifespanForTesting(false);
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  base::string16 CalculateHash(const std::string& profile_id,
                               const GURL& origin) {
    return base::UintToString16(base::Hash(base::UTF8ToUTF16(profile_id) +
                                           base::UTF8ToUTF16(origin.spec())));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationImageRetainerTest);
};

TEST_F(NotificationImageRetainerTest, FileCreation) {
  auto image_retainer = std::make_unique<NotificationImageRetainer>(
      scoped_task_environment_.GetMainThreadTaskRunner());

  SkBitmap icon;
  icon.allocN32Pixels(64, 64);
  icon.eraseARGB(255, 100, 150, 200);
  gfx::Image image = gfx::Image::CreateFrom1xBitmap(icon);

  GURL origin1("https://www.google.com");
  GURL origin2("https://www.chromium.org");

  // Expecting separate directories per profile and origin.
  base::string16 dir_profile1_origin1 = CalculateHash(kProfileId1, origin1);
  base::string16 dir_profile1_origin2 = CalculateHash(kProfileId1, origin2);
  base::string16 dir_profile2_origin1 = CalculateHash(kProfileId2, origin1);
  base::string16 dir_profile2_origin2 = CalculateHash(kProfileId2, origin2);

  base::FilePath path1 =
      image_retainer->RegisterTemporaryImage(image, kProfileId1, origin1);
  ASSERT_TRUE(base::PathExists(path1));
  ASSERT_TRUE(path1.value().find(dir_profile1_origin1) != std::string::npos)
      << path1.value().c_str();

  std::vector<base::FilePath::StringType> components;
  path1.GetComponents(&components);
  base::FilePath image_dir;
  for (size_t i = 0; i < components.size() - 2; ++i)
    image_dir = image_dir.Append(components[i]);

  base::FilePath path2 =
      image_retainer->RegisterTemporaryImage(image, kProfileId1, origin2);
  ASSERT_TRUE(base::PathExists(path2));
  ASSERT_TRUE(path2.value().find(dir_profile1_origin2) != std::string::npos)
      << path2.value().c_str();

  base::FilePath path3 =
      image_retainer->RegisterTemporaryImage(image, kProfileId2, origin1);
  ASSERT_TRUE(base::PathExists(path3));
  ASSERT_TRUE(path3.value().find(dir_profile2_origin1) != std::string::npos)
      << path3.value().c_str();

  base::FilePath path4 =
      image_retainer->RegisterTemporaryImage(image, kProfileId2, origin2);
  ASSERT_TRUE(base::PathExists(path4));
  ASSERT_TRUE(path4.value().find(dir_profile2_origin2) != std::string::npos)
      << path4.value().c_str();

  base::RunLoop().RunUntilIdle();

  ASSERT_FALSE(base::PathExists(path1));
  ASSERT_FALSE(base::PathExists(path2));
  ASSERT_FALSE(base::PathExists(path3));
  ASSERT_FALSE(base::PathExists(path4));

  image_retainer.reset(nullptr);
  ASSERT_FALSE(base::PathExists(image_dir));
}
