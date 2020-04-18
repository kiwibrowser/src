// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper_impl.h"

namespace chromeos {

EnterpriseEnrollmentHelper::CreateMockEnrollmentHelper
    EnterpriseEnrollmentHelper::create_mock_enrollment_helper_ = nullptr;

EnterpriseEnrollmentHelper::~EnterpriseEnrollmentHelper() {}

// static
void EnterpriseEnrollmentHelper::SetupEnrollmentHelperMock(
    CreateMockEnrollmentHelper creator) {
  create_mock_enrollment_helper_ = creator;
}

// static
std::unique_ptr<EnterpriseEnrollmentHelper> EnterpriseEnrollmentHelper::Create(
    EnrollmentStatusConsumer* status_consumer,
    ActiveDirectoryJoinDelegate* ad_join_delegate,
    const policy::EnrollmentConfig& enrollment_config,
    const std::string& enrolling_user_domain) {
  // Create a mock instance.
  if (create_mock_enrollment_helper_) {
    // The evaluated code might call |SetupEnrollmentHelperMock| to setup a new
    // allocator, so we reset the enrollment helper to null before that.
    auto enrollment_helper_allocator = create_mock_enrollment_helper_;
    create_mock_enrollment_helper_ = nullptr;
    EnterpriseEnrollmentHelper* helper = enrollment_helper_allocator(
        status_consumer, enrollment_config, enrolling_user_domain);
    return base::WrapUnique(helper);
  }

  return base::WrapUnique(new EnterpriseEnrollmentHelperImpl(
      status_consumer, ad_join_delegate, enrollment_config,
      enrolling_user_domain));
}

EnterpriseEnrollmentHelper::EnterpriseEnrollmentHelper(
    EnrollmentStatusConsumer* status_consumer)
    : status_consumer_(status_consumer) {
  DCHECK(status_consumer_);
}

}  // namespace chromeos
