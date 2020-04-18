// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_destroyer.h"

#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"

class TestingOffTheRecordDestructionProfile : public TestingProfile {
 public:
  TestingOffTheRecordDestructionProfile()
      : TestingProfile(
            base::FilePath(),
            NULL,
            scoped_refptr<ExtensionSpecialStoragePolicy>()
                std::unique_ptr<sync_preferences::PrefServiceSyncable>(),
            true,
            TestingFactories()),
        destroyed_otr_profile_(false) {
    set_incognito(true);
  }
  void DestroyOffTheRecordProfile() override {
    destroyed_otr_profile_ = true;
  }
  bool destroyed_otr_profile_;

  DISALLOW_COPY_AND_ASSIGN(TestingOffTheRecordDestructionProfile);
};

class TestingOriginalDestructionProfile : public TestingProfile {
 public:
  TestingOriginalDestructionProfile() : destroyed_otr_profile_(false) {
    DCHECK_EQ(kNull, living_instance_);
    living_instance_ = this;
  }
  ~TestingOriginalDestructionProfile() override {
    DCHECK_EQ(this, living_instance_);
    living_instance_ = NULL;
  }
  void DestroyOffTheRecordProfile() override {
    SetOffTheRecordProfile(NULL);
    destroyed_otr_profile_ = true;
  }
  bool destroyed_otr_profile_;
  static TestingOriginalDestructionProfile* living_instance_;

  // This is to avoid type casting in DCHECK_EQ & EXPECT_NE.
  static const TestingOriginalDestructionProfile* kNull;

  DISALLOW_COPY_AND_ASSIGN(TestingOriginalDestructionProfile);
};
const TestingOriginalDestructionProfile*
    TestingOriginalDestructionProfile::kNull = NULL;

TestingOriginalDestructionProfile*
    TestingOriginalDestructionProfile::living_instance_ = NULL;

class ProfileDestroyerTest : public BrowserWithTestWindowTest {
 public:
  ProfileDestroyerTest() : off_the_record_profile_(NULL) {}

 protected:
  TestingProfile* CreateProfile() override {
    if (off_the_record_profile_ == NULL)
      off_the_record_profile_ = new TestingOffTheRecordDestructionProfile();
    return off_the_record_profile_;
  }
  TestingOffTheRecordDestructionProfile* off_the_record_profile_;

  DISALLOW_COPY_AND_ASSIGN(ProfileDestroyerTest);
};

TEST_F(ProfileDestroyerTest, DelayProfileDestruction) {
  scoped_refptr<content::SiteInstance> instance1(
      content::SiteInstance::Create(off_the_record_profile_));
  std::unique_ptr<content::RenderProcessHost> render_process_host1;
  render_process_host1.reset(instance1->GetProcess());
  ASSERT_TRUE(render_process_host1.get() != NULL);

  scoped_refptr<content::SiteInstance> instance2(
      content::SiteInstance::Create(off_the_record_profile_));
  std::unique_ptr<content::RenderProcessHost> render_process_host2;
  render_process_host2.reset(instance2->GetProcess());
  ASSERT_TRUE(render_process_host2.get() != NULL);

  // destroying the browser should not destroy the off the record profile...
  set_browser(NULL);
  EXPECT_FALSE(off_the_record_profile_->destroyed_otr_profile_);

  // until we destroy the render process host holding on to it...
  render_process_host1.release()->Cleanup();

  // And asynchronicity kicked in properly.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(off_the_record_profile_->destroyed_otr_profile_);

  // I meant, ALL the render process hosts... :-)
  render_process_host2.release()->Cleanup();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(off_the_record_profile_->destroyed_otr_profile_);
}

TEST_F(ProfileDestroyerTest, DelayOriginalProfileDestruction) {
  TestingOriginalDestructionProfile* original_profile =
      new TestingOriginalDestructionProfile;

  TestingOffTheRecordDestructionProfile* off_the_record_profile =
      new TestingOffTheRecordDestructionProfile;

  original_profile->SetOffTheRecordProfile(off_the_record_profile);

  scoped_refptr<content::SiteInstance> instance1(
      content::SiteInstance::Create(off_the_record_profile));
  std::unique_ptr<content::RenderProcessHost> render_process_host1;
  render_process_host1.reset(instance1->GetProcess());
  ASSERT_TRUE(render_process_host1.get() != NULL);

  // Trying to destroy the original profile should be delayed until associated
  // off the record profile is released by all render process hosts.
  ProfileDestroyer::DestroyProfileWhenAppropriate(original_profile);
  EXPECT_NE(TestingOriginalDestructionProfile::kNull,
            TestingOriginalDestructionProfile::living_instance_);
  EXPECT_FALSE(original_profile->destroyed_otr_profile_);

  render_process_host1.release()->Cleanup();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(NULL, TestingOriginalDestructionProfile::living_instance_);

  // And the same protection should apply to the main profile.
  TestingOriginalDestructionProfile* main_profile =
      new TestingOriginalDestructionProfile;
  scoped_refptr<content::SiteInstance> instance2(
      content::SiteInstance::Create(main_profile));
  std::unique_ptr<content::RenderProcessHost> render_process_host2;
  render_process_host2.reset(instance2->GetProcess());
  ASSERT_TRUE(render_process_host2.get() != NULL);

  ProfileDestroyer::DestroyProfileWhenAppropriate(main_profile);
  EXPECT_EQ(main_profile, TestingOriginalDestructionProfile::living_instance_);
  render_process_host2.release()->Cleanup();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(NULL, TestingOriginalDestructionProfile::living_instance_);
}
