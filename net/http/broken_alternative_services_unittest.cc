// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/broken_alternative_services.h"

#include <algorithm>
#include <vector>

#include "base/test/test_mock_time_task_runner.h"
#include "base/time/tick_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

class BrokenAlternativeServicesTest
    : public BrokenAlternativeServices::Delegate,
      public ::testing::Test {
 public:
  BrokenAlternativeServicesTest()
      : test_task_runner_(new base::TestMockTimeTaskRunner()),
        test_task_runner_context_(test_task_runner_),
        broken_services_clock_(test_task_runner_->GetMockTickClock()),
        broken_services_(this, broken_services_clock_) {}

  // BrokenAlternativeServices::Delegate implementation
  void OnExpireBrokenAlternativeService(
      const AlternativeService& expired_alternative_service) override {
    expired_alt_svcs_.push_back(expired_alternative_service);
  }

  // All tests will run inside the scope of |test_task_runner_context_|, which
  // means any task posted to the main message loop will run on
  // |test_task_runner_|.
  scoped_refptr<base::TestMockTimeTaskRunner> test_task_runner_;
  base::TestMockTimeTaskRunner::ScopedContext test_task_runner_context_;

  const base::TickClock* broken_services_clock_;
  BrokenAlternativeServices broken_services_;

  std::vector<AlternativeService> expired_alt_svcs_;
};

TEST_F(BrokenAlternativeServicesTest, MarkBroken) {
  const AlternativeService alternative_service1(kProtoHTTP2, "foo", 443);
  const AlternativeService alternative_service2(kProtoHTTP2, "foo", 1234);

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  broken_services_.MarkAlternativeServiceBroken(alternative_service1);

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  broken_services_.MarkAlternativeServiceBroken(alternative_service2);

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  broken_services_.ConfirmAlternativeService(alternative_service1);

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  broken_services_.ConfirmAlternativeService(alternative_service2);

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  EXPECT_EQ(0u, expired_alt_svcs_.size());
}

TEST_F(BrokenAlternativeServicesTest, MarkRecentlyBroken) {
  const AlternativeService alternative_service(kProtoHTTP2, "foo", 443);

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_FALSE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service));

  broken_services_.MarkAlternativeServiceRecentlyBroken(alternative_service);
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service));

  broken_services_.ConfirmAlternativeService(alternative_service);
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_FALSE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service));
}

TEST_F(BrokenAlternativeServicesTest, ExpireBrokenAlternateProtocolMappings) {
  AlternativeService alternative_service(kProtoQUIC, "foo", 443);

  broken_services_.MarkAlternativeServiceBroken(alternative_service);

  // |broken_services_| should have posted task to expire the brokenness of
  // |alternative_service|.
  EXPECT_EQ(1u, test_task_runner_->GetPendingTaskCount());

  // Advance time until one time quantum before |alternative_service1|'s
  // brokenness expires
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(5) -
                                   base::TimeDelta::FromInternalValue(1));

  // Ensure |alternative_service| is still marked broken.
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_EQ(0u, expired_alt_svcs_.size());
  EXPECT_EQ(1u, test_task_runner_->GetPendingTaskCount());

  // Advance time by one time quantum.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));

  // Ensure |alternative_service| brokenness has expired but is still
  // considered recently broken
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_FALSE(test_task_runner_->HasPendingTask());
  EXPECT_EQ(1u, expired_alt_svcs_.size());
  EXPECT_EQ(alternative_service, expired_alt_svcs_[0]);
  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service));
}

TEST_F(BrokenAlternativeServicesTest, IsAlternativeServiceBroken) {
  // Tests the IsAlternativeServiceBroken() methods.
  AlternativeService alternative_service(kProtoQUIC, "foo", 443);
  base::TimeTicks brokenness_expiration;

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_FALSE(broken_services_.IsAlternativeServiceBroken(
      alternative_service, &brokenness_expiration));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(
      alternative_service, &brokenness_expiration));
  EXPECT_EQ(
      broken_services_clock_->NowTicks() + base::TimeDelta::FromMinutes(5),
      brokenness_expiration);

  // Fast forward time until |alternative_service|'s brokenness expires.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(5));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_FALSE(broken_services_.IsAlternativeServiceBroken(
      alternative_service, &brokenness_expiration));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(
      alternative_service, &brokenness_expiration));
  EXPECT_EQ(
      broken_services_clock_->NowTicks() + base::TimeDelta::FromMinutes(10),
      brokenness_expiration);

  broken_services_.ConfirmAlternativeService(alternative_service);
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
  EXPECT_FALSE(broken_services_.IsAlternativeServiceBroken(
      alternative_service, &brokenness_expiration));
}

TEST_F(BrokenAlternativeServicesTest, ExponentialBackoff) {
  // Tests the exponential backoff of the computed expiration delay when an
  // alt svc is marked broken. After being marked broken 10 times, the max
  // expiration delay will have been reached and exponential backoff will no
  // longer apply.

  AlternativeService alternative_service(kProtoQUIC, "foo", 443);

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(5) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(10) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(20) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(40) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(80) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(160) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(320) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(640) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(1280) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(2560) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));

  // Max expiration delay has been reached; subsequent expiration delays from
  // this point forward should not increase further.
  broken_services_.MarkAlternativeServiceBroken(alternative_service);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(2560) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(broken_services_.IsAlternativeServiceBroken(alternative_service));
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service));
}

TEST_F(BrokenAlternativeServicesTest, RemoveExpiredBrokenAltSvc) {
  // This test will mark broken an alternative service A that has already been
  // marked broken many times, then immediately mark another alternative service
  // B as broken for the first time. Because A's been marked broken many times
  // already, its brokenness will be scheduled to expire much further in the
  // future than B, even though it was marked broken before B. This test makes
  // sure that even though A was marked broken before B, B's brokenness should
  // expire before A.

  AlternativeService alternative_service1(kProtoQUIC, "foo", 443);
  AlternativeService alternative_service2(kProtoQUIC, "bar", 443);

  // Repeately mark |alternative_service1| broken and let brokenness expire.
  // Do this a few times.

  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  EXPECT_EQ(1u, test_task_runner_->GetPendingTaskCount());
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(5));
  EXPECT_EQ(1u, expired_alt_svcs_.size());
  EXPECT_EQ(alternative_service1, expired_alt_svcs_.back());

  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  EXPECT_EQ(1u, test_task_runner_->GetPendingTaskCount());
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(10));
  EXPECT_EQ(2u, expired_alt_svcs_.size());
  EXPECT_EQ(alternative_service1, expired_alt_svcs_.back());

  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  EXPECT_EQ(1u, test_task_runner_->GetPendingTaskCount());
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(20));
  EXPECT_EQ(3u, expired_alt_svcs_.size());
  EXPECT_EQ(alternative_service1, expired_alt_svcs_.back());

  expired_alt_svcs_.clear();

  // Mark |alternative_service1| broken (will be given longer expiration delay),
  // then mark |alternative_service2| broken (will be given shorter expiration
  // delay).
  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  broken_services_.MarkAlternativeServiceBroken(alternative_service2);

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  // Advance time until one time quantum before |alternative_service2|'s
  // brokenness expires.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(5) -
                                   base::TimeDelta::FromInternalValue(1));

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_EQ(0u, expired_alt_svcs_.size());

  // Advance time by one time quantum. |alternative_service2| should no longer
  // be broken.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_EQ(1u, expired_alt_svcs_.size());
  EXPECT_EQ(alternative_service2, expired_alt_svcs_[0]);

  // Advance time until one time quantum before |alternative_service1|'s
  // brokenness expires
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(40) -
                                   base::TimeDelta::FromMinutes(5) -
                                   base::TimeDelta::FromInternalValue(1));

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_EQ(1u, expired_alt_svcs_.size());
  EXPECT_EQ(alternative_service2, expired_alt_svcs_[0]);

  // Advance time by one time quantum.  |alternative_service1| should no longer
  // be broken.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_EQ(2u, expired_alt_svcs_.size());
  EXPECT_EQ(alternative_service2, expired_alt_svcs_[0]);
  EXPECT_EQ(alternative_service1, expired_alt_svcs_[1]);
}

TEST_F(BrokenAlternativeServicesTest, SetBrokenAlternativeServices) {
  AlternativeService alternative_service1(kProtoQUIC, "foo1", 443);
  AlternativeService alternative_service2(kProtoQUIC, "foo2", 443);

  base::TimeDelta delay1 = base::TimeDelta::FromMinutes(1);

  std::unique_ptr<BrokenAlternativeServiceList> broken_list =
      std::make_unique<BrokenAlternativeServiceList>();
  broken_list->push_back(
      {alternative_service1, broken_services_clock_->NowTicks() + delay1});

  std::unique_ptr<RecentlyBrokenAlternativeServices> recently_broken_map =
      std::make_unique<RecentlyBrokenAlternativeServices>();
  recently_broken_map->Put(alternative_service1, 1);
  recently_broken_map->Put(alternative_service2, 2);

  broken_services_.SetBrokenAndRecentlyBrokenAlternativeServices(
      std::move(broken_list), std::move(recently_broken_map));

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service1));
  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service2));

  // Make sure |alternative_service1| expires after the delay in |broken_list|.
  test_task_runner_->FastForwardBy(delay1 -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));

  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));

  // Make sure the broken counts in |recently_broken_map| translate to the
  // correct expiration delays if the alternative services are marked broken.
  broken_services_.MarkAlternativeServiceBroken(alternative_service2);
  broken_services_.MarkAlternativeServiceBroken(alternative_service1);

  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(10) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(20) -
                                   base::TimeDelta::FromMinutes(10) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));

  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
}

TEST_F(BrokenAlternativeServicesTest,
       SetBrokenAlternativeServicesWithExisting) {
  AlternativeService alternative_service1(kProtoQUIC, "foo1", 443);
  AlternativeService alternative_service2(kProtoQUIC, "foo2", 443);
  AlternativeService alternative_service3(kProtoQUIC, "foo3", 443);

  std::unique_ptr<BrokenAlternativeServiceList> broken_list =
      std::make_unique<BrokenAlternativeServiceList>();
  broken_list->push_back(
      {alternative_service1,
       broken_services_clock_->NowTicks() + base::TimeDelta::FromMinutes(3)});
  broken_list->push_back(
      {alternative_service3,
       broken_services_clock_->NowTicks() + base::TimeDelta::FromMinutes(1)});

  std::unique_ptr<RecentlyBrokenAlternativeServices> recently_broken_map =
      std::make_unique<RecentlyBrokenAlternativeServices>();
  recently_broken_map->Put(alternative_service1, 1);
  recently_broken_map->Put(alternative_service3, 1);

  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  broken_services_.MarkAlternativeServiceBroken(alternative_service2);

  // At this point, |alternative_service1| and |alternative_service2| are marked
  // broken and should expire in 5 minutes.
  // Adding |broken_list| should overwrite |alternative_service1|'s expiration
  // time to 3 minutes, and additionally mark |alternative_service3|
  // broken with an expiration time of 1 minute.
  broken_services_.SetBrokenAndRecentlyBrokenAlternativeServices(
      std::move(broken_list), std::move(recently_broken_map));

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service3));

  // Make sure |alternative_service3|'s brokenness expires in 1 minute.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(1) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service3));

  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service3));

  // Make sure |alternative_service1|'s brokenness expires in 2 more minutes.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(2) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service3));

  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service3));

  // Make sure |alternative_service2|'s brokenness expires in 2 more minutes.
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(2) -
                                   base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service3));

  test_task_runner_->FastForwardBy(base::TimeDelta::FromInternalValue(1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service3));

  // Make sure recently broken alternative services are in most-recently-used
  // order. SetBrokenAndRecentlyBrokenAlternativeServices() will add
  // entries in |recently_broken_map| (that aren't already marked recently
  // broken in |broken_services_|) to the back of |broken_services_|'s
  // recency list; in this case, only |alternative_service3| is added as
  // recently broken.
  auto it = broken_services_.recently_broken_alternative_services().begin();
  EXPECT_EQ(alternative_service2, it->first);
  ++it;
  EXPECT_EQ(alternative_service1, it->first);
  ++it;
  EXPECT_EQ(alternative_service3, it->first);
}

TEST_F(BrokenAlternativeServicesTest, ScheduleExpireTaskAfterExpire) {
  // This test will check that when a broken alt svc expires, an expiration task
  // is scheduled for the next broken alt svc in the expiration queue.

  AlternativeService alternative_service1(kProtoQUIC, "foo", 443);
  AlternativeService alternative_service2(kProtoQUIC, "bar", 443);

  // Mark |alternative_service1| broken and let brokenness expire. This will
  // increase its expiration delay the next time it's marked broken.
  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(5));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(test_task_runner_->HasPendingTask());

  // Mark |alternative_service1| and |alternative_service2| broken and
  // let |alternative_service2|'s brokenness expire.
  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  broken_services_.MarkAlternativeServiceBroken(alternative_service2);

  test_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(5));
  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service2));
  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));

  // Make sure an expiration task has been scheduled for expiring the brokenness
  // of |alternative_service1|.
  EXPECT_TRUE(test_task_runner_->HasPendingTask());
}

TEST_F(BrokenAlternativeServicesTest, Clear) {
  AlternativeService alternative_service1(kProtoQUIC, "foo", 443);
  AlternativeService alternative_service2(kProtoQUIC, "bar", 443);

  broken_services_.MarkAlternativeServiceBroken(alternative_service1);
  broken_services_.MarkAlternativeServiceRecentlyBroken(alternative_service2);

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service1));
  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service2));

  broken_services_.Clear();

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service1));
  EXPECT_FALSE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service2));

  std::unique_ptr<BrokenAlternativeServiceList> broken_list =
      std::make_unique<BrokenAlternativeServiceList>();
  broken_list->push_back(
      {alternative_service1,
       broken_services_clock_->NowTicks() + base::TimeDelta::FromMinutes(1)});

  std::unique_ptr<RecentlyBrokenAlternativeServices> recently_broken_map =
      std::make_unique<RecentlyBrokenAlternativeServices>();
  recently_broken_map->Put(alternative_service2, 2);

  broken_services_.SetBrokenAndRecentlyBrokenAlternativeServices(
      std::move(broken_list), std::move(recently_broken_map));

  EXPECT_TRUE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service1));
  EXPECT_TRUE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service2));

  broken_services_.Clear();

  EXPECT_FALSE(
      broken_services_.IsAlternativeServiceBroken(alternative_service1));
  EXPECT_FALSE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service1));
  EXPECT_FALSE(broken_services_.WasAlternativeServiceRecentlyBroken(
      alternative_service2));
}

}  // namespace

}  // namespace net
