// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/reporting_permissions_checker.h"

#include <set>

#include "base/bind.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/mock_permission_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

using testing::_;
using testing::Eq;
using testing::IsEmpty;
using testing::Return;
using testing::UnorderedElementsAre;

class TestingPermissionProfile : public TestingProfile {
 public:
  TestingPermissionProfile() = default;

  content::MockPermissionManager* mock_permission_manager() {
    return &mock_permission_manager_;
  }

  content::PermissionControllerDelegate* GetPermissionControllerDelegate()
      override {
    return &mock_permission_manager_;
  }

 private:
  content::MockPermissionManager mock_permission_manager_;
};

}  // namespace

class ReportingPermissionsCheckerTest : public testing::Test {
 public:
  ReportingPermissionsCheckerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        reporting_permissions_checker_factory_(&profile_),
        reporting_permissions_checker_(
            reporting_permissions_checker_factory_.CreateChecker()) {}

  content::MockPermissionManager* mock_permission_manager() {
    return profile_.mock_permission_manager();
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingPermissionProfile profile_;
  ReportingPermissionsCheckerFactory reporting_permissions_checker_factory_;
  std::unique_ptr<ReportingPermissionsChecker> reporting_permissions_checker_;
  DISALLOW_COPY_AND_ASSIGN(ReportingPermissionsCheckerTest);
};

TEST_F(ReportingPermissionsCheckerTest, ChecksReportingPermissionsBothGranted) {
  auto origin1 = url::Origin::Create(GURL("https://example.com/"));
  auto origin2 = url::Origin::Create(GURL("https://foo.example.com/"));

  std::set<url::Origin> origins = {origin1, origin2};

  EXPECT_CALL(*mock_permission_manager(),
              GetPermissionStatus(_, origin1.GetURL(), _))
      .WillOnce(Return(blink::mojom::PermissionStatus::GRANTED));
  EXPECT_CALL(*mock_permission_manager(),
              GetPermissionStatus(_, origin2.GetURL(), _))
      .WillOnce(Return(blink::mojom::PermissionStatus::GRANTED));

  std::set<url::Origin> allowed_origins;
  reporting_permissions_checker_->FilterReportingOrigins(
      std::move(origins),
      base::BindOnce(
          [](std::set<url::Origin>* dest, std::set<url::Origin> result) {
            *dest = std::move(result);
          },
          &allowed_origins));
  content::RunAllTasksUntilIdle();

  EXPECT_THAT(allowed_origins, UnorderedElementsAre(Eq(origin1), Eq(origin2)));
}

TEST_F(ReportingPermissionsCheckerTest, ChecksReportingPermissionsBothDenied) {
  auto origin1 = url::Origin::Create(GURL("https://example.com/"));
  auto origin2 = url::Origin::Create(GURL("https://foo.example.com/"));

  std::set<url::Origin> origins = {origin1, origin2};

  EXPECT_CALL(*mock_permission_manager(),
              GetPermissionStatus(_, origin1.GetURL(), _))
      .WillOnce(Return(blink::mojom::PermissionStatus::DENIED));
  EXPECT_CALL(*mock_permission_manager(),
              GetPermissionStatus(_, origin2.GetURL(), _))
      .WillOnce(Return(blink::mojom::PermissionStatus::DENIED));

  std::set<url::Origin> allowed_origins;
  reporting_permissions_checker_->FilterReportingOrigins(
      std::move(origins),
      base::BindOnce(
          [](std::set<url::Origin>* dest, std::set<url::Origin> result) {
            *dest = std::move(result);
          },
          &allowed_origins));
  content::RunAllTasksUntilIdle();

  EXPECT_THAT(allowed_origins, IsEmpty());
}

TEST_F(ReportingPermissionsCheckerTest, ChecksReportingPermissionsMixed) {
  auto origin1 = url::Origin::Create(GURL("https://example.com/"));
  auto origin2 = url::Origin::Create(GURL("https://foo.example.com/"));

  std::set<url::Origin> origins = {origin1, origin2};

  EXPECT_CALL(*mock_permission_manager(),
              GetPermissionStatus(_, origin1.GetURL(), _))
      .WillOnce(Return(blink::mojom::PermissionStatus::GRANTED));
  EXPECT_CALL(*mock_permission_manager(),
              GetPermissionStatus(_, origin2.GetURL(), _))
      .WillOnce(Return(blink::mojom::PermissionStatus::DENIED));

  std::set<url::Origin> allowed_origins;
  reporting_permissions_checker_->FilterReportingOrigins(
      std::move(origins),
      base::BindOnce(
          [](std::set<url::Origin>* dest, std::set<url::Origin> result) {
            *dest = std::move(result);
          },
          &allowed_origins));
  content::RunAllTasksUntilIdle();

  EXPECT_THAT(allowed_origins, UnorderedElementsAre(Eq(origin1)));
}
