// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/account_investigator.h"

#include <map>
#include <string>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/timer/timer.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_gaia_cookie_manager_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::HistogramTester;
using base::Time;
using base::TimeDelta;
using gaia::ListedAccount;
using signin_metrics::AccountRelation;
using signin_metrics::ReportingType;

class AccountInvestigatorTest : public testing::Test {
 protected:
  AccountInvestigatorTest()
      : signin_client_(&prefs_),
        signin_manager_(&signin_client_,
                        nullptr,
                        &account_tracker_service_,
                        nullptr),
        gaia_cookie_manager_service_(nullptr,
                                     GaiaConstants::kChromeSource,
                                     &signin_client_),
        investigator_(&gaia_cookie_manager_service_, &prefs_, &signin_manager_),
        fake_url_fetcher_factory_(nullptr) {
    AccountTrackerService::RegisterPrefs(prefs_.registry());
    AccountInvestigator::RegisterPrefs(prefs_.registry());
    SigninManagerBase::RegisterProfilePrefs(prefs_.registry());
    account_tracker_service_.Initialize(&signin_client_);
    gaia_cookie_manager_service_.Init(&fake_url_fetcher_factory_);
  }

  ~AccountInvestigatorTest() override { investigator_.Shutdown(); }

  FakeSigninManager* signin_manager() { return &signin_manager_; }
  PrefService* pref_service() { return &prefs_; }
  AccountInvestigator* investigator() { return &investigator_; }
  FakeGaiaCookieManagerService* cookie_manager_service() {
    return &gaia_cookie_manager_service_;
  }

  // Wrappers to invoke private methods through friend class.
  TimeDelta Delay(const Time previous,
                  const Time now,
                  const TimeDelta interval) {
    return AccountInvestigator::CalculatePeriodicDelay(previous, now, interval);
  }
  std::string Hash(const std::vector<ListedAccount>& signed_in_accounts,
                   const std::vector<ListedAccount>& signed_out_accounts) {
    return AccountInvestigator::HashAccounts(signed_in_accounts,
                                             signed_out_accounts);
  }
  AccountRelation Relation(
      const AccountInfo& info,
      const std::vector<ListedAccount>& signed_in_accounts,
      const std::vector<ListedAccount>& signed_out_accounts) {
    return AccountInvestigator::DiscernRelation(info,
                                                signed_in_accounts,
                                                signed_out_accounts);
  }
  void SharedReport(const std::vector<gaia::ListedAccount>& signed_in_accounts,
                    const std::vector<gaia::ListedAccount>& signed_out_accounts,
                    const Time now,
                    const ReportingType type) {
    investigator_.SharedCookieJarReport(signed_in_accounts, signed_out_accounts,
                                        now, type);
  }
  void TryPeriodicReport() { investigator_.TryPeriodicReport(); }
  bool* periodic_pending() { return &investigator_.periodic_pending_; }
  bool* previously_authenticated() {
    return &investigator_.previously_authenticated_;
  }
  base::OneShotTimer* timer() { return &investigator_.timer_; }

  void ExpectRelationReport(
      const std::vector<ListedAccount> signed_in_accounts,
      const std::vector<ListedAccount> signed_out_accounts,
      const ReportingType type,
      const AccountRelation expected) {
    HistogramTester histogram_tester;
    investigator_.SignedInAccountRelationReport(signed_in_accounts,
                                                signed_out_accounts,
                                                type);
    ExpectRelationReport(type, histogram_tester, expected);
  }

  void ExpectRelationReport(const ReportingType type,
                            const HistogramTester& histogram_tester,
                            const AccountRelation expected) {
    histogram_tester.ExpectUniqueSample(
        "Signin.CookieJar.ChromeAccountRelation" + suffix_[type],
        static_cast<int>(expected), 1);
  }

  // If |relation| is a nullptr, then it should not have been recorded.
  // If |stable_age| is a nullptr, then we're not sure what the expected time
  // should have been, but it still should have been recorded.
  void ExpectSharedReportHistograms(const ReportingType type,
                                    const HistogramTester& histogram_tester,
                                    const TimeDelta* stable_age,
                                    const int signed_in_count,
                                    const int signed_out_count,
                                    const int total_count,
                                    const AccountRelation* relation,
                                    const bool is_shared) {
    if (stable_age) {
      histogram_tester.ExpectUniqueSample(
          "Signin.CookieJar.StableAge" + suffix_[type], stable_age->InSeconds(),
          1);
    } else {
      histogram_tester.ExpectTotalCount(
          "Signin.CookieJar.StableAge" + suffix_[type], 1);
    }
    histogram_tester.ExpectUniqueSample(
        "Signin.CookieJar.SignedInCount" + suffix_[type], signed_in_count, 1);
    histogram_tester.ExpectUniqueSample(
        "Signin.CookieJar.SignedOutCount" + suffix_[type], signed_out_count, 1);
    histogram_tester.ExpectUniqueSample(
        "Signin.CookieJar.TotalCount" + suffix_[type], total_count, 1);
    if (relation) {
      histogram_tester.ExpectUniqueSample(
          "Signin.CookieJar.ChromeAccountRelation" + suffix_[type],
          static_cast<int>(*relation), 1);
    } else {
      histogram_tester.ExpectTotalCount(
          "Signin.CookieJar.ChromeAccountRelation" + suffix_[type], 0);
    }
    histogram_tester.ExpectUniqueSample("Signin.IsShared" + suffix_[type],
                                        is_shared, 1);
  }

 private:
  // Timer needs a message loop.
  base::MessageLoop message_loop_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  AccountTrackerService account_tracker_service_;
  TestSigninClient signin_client_;
  FakeSigninManager signin_manager_;
  FakeGaiaCookieManagerService gaia_cookie_manager_service_;
  AccountInvestigator investigator_;
  net::FakeURLFetcherFactory fake_url_fetcher_factory_;
  std::map<ReportingType, std::string> suffix_ = {
      {ReportingType::PERIODIC, "_Periodic"},
      {ReportingType::ON_CHANGE, "_OnChange"}};
};

namespace {

ListedAccount Account(const std::string& id) {
  ListedAccount account;
  account.id = id;
  return account;
}

AccountInfo Info(const std::string& id) {
  AccountInfo info;
  info.account_id = id;
  return info;
}

const std::vector<ListedAccount> no_accounts{};
const std::vector<ListedAccount> just_one{Account("1")};
const std::vector<ListedAccount> just_two{Account("2")};
const std::vector<ListedAccount> both{Account("1"), Account("2")};
const std::vector<ListedAccount> both_reversed{Account("2"), Account("1")};

const AccountInfo one(Info("1"));
const AccountInfo three(Info("3"));

TEST_F(AccountInvestigatorTest, CalculatePeriodicDelay) {
  const Time epoch;
  const TimeDelta day(TimeDelta::FromDays(1));
  const TimeDelta big(TimeDelta::FromDays(1000));

  EXPECT_EQ(day, Delay(epoch, epoch, day));
  EXPECT_EQ(day, Delay(epoch + big, epoch + big, day));
  EXPECT_EQ(TimeDelta(), Delay(epoch, epoch + big, day));
  EXPECT_EQ(day, Delay(epoch + big, epoch, day));
  EXPECT_EQ(day, Delay(epoch, epoch + day, TimeDelta::FromDays(2)));
}

TEST_F(AccountInvestigatorTest, HashAccounts) {
  EXPECT_EQ(Hash(no_accounts, no_accounts), Hash(no_accounts, no_accounts));
  EXPECT_EQ(Hash(just_one, just_two), Hash(just_one, just_two));
  EXPECT_EQ(Hash(both, no_accounts), Hash(both, no_accounts));
  EXPECT_EQ(Hash(no_accounts, both), Hash(no_accounts, both));
  EXPECT_EQ(Hash(both, no_accounts), Hash(both_reversed, no_accounts));
  EXPECT_EQ(Hash(no_accounts, both), Hash(no_accounts, both_reversed));

  EXPECT_NE(Hash(no_accounts, no_accounts), Hash(just_one, no_accounts));
  EXPECT_NE(Hash(no_accounts, no_accounts), Hash(no_accounts, just_one));
  EXPECT_NE(Hash(just_one, no_accounts), Hash(just_two, no_accounts));
  EXPECT_NE(Hash(just_one, no_accounts), Hash(both, no_accounts));
  EXPECT_NE(Hash(just_one, no_accounts), Hash(no_accounts, just_one));
}

TEST_F(AccountInvestigatorTest, DiscernRelation) {
  EXPECT_EQ(AccountRelation::EMPTY_COOKIE_JAR,
            Relation(one, no_accounts, no_accounts));
  EXPECT_EQ(AccountRelation::SINGLE_SIGNED_IN_MATCH_NO_SIGNED_OUT,
            Relation(one, just_one, no_accounts));
  EXPECT_EQ(AccountRelation::SINGLE_SINGED_IN_MATCH_WITH_SIGNED_OUT,
            Relation(one, just_one, just_two));
  EXPECT_EQ(AccountRelation::WITH_SIGNED_IN_NO_MATCH,
            Relation(one, just_two, no_accounts));
  EXPECT_EQ(AccountRelation::ONE_OF_SIGNED_IN_MATCH_ANY_SIGNED_OUT,
            Relation(one, both, just_one));
  EXPECT_EQ(AccountRelation::ONE_OF_SIGNED_IN_MATCH_ANY_SIGNED_OUT,
            Relation(one, both, no_accounts));
  EXPECT_EQ(AccountRelation::NO_SIGNED_IN_ONE_OF_SIGNED_OUT_MATCH,
            Relation(one, no_accounts, both));
  EXPECT_EQ(AccountRelation::NO_SIGNED_IN_SINGLE_SIGNED_OUT_MATCH,
            Relation(one, no_accounts, just_one));
  EXPECT_EQ(AccountRelation::WITH_SIGNED_IN_ONE_OF_SIGNED_OUT_MATCH,
            Relation(one, just_two, just_one));
  EXPECT_EQ(AccountRelation::NO_SIGNED_IN_WITH_SIGNED_OUT_NO_MATCH,
            Relation(three, no_accounts, both));
}

TEST_F(AccountInvestigatorTest, SignedInAccountRelationReport) {
  ExpectRelationReport(just_one, no_accounts, ReportingType::PERIODIC,
                       AccountRelation::WITH_SIGNED_IN_NO_MATCH);
  signin_manager()->SignIn("1", "a", "A");
  ExpectRelationReport(just_one, no_accounts, ReportingType::PERIODIC,
                       AccountRelation::SINGLE_SIGNED_IN_MATCH_NO_SIGNED_OUT);
  ExpectRelationReport(just_two, no_accounts, ReportingType::ON_CHANGE,
                       AccountRelation::WITH_SIGNED_IN_NO_MATCH);
}

TEST_F(AccountInvestigatorTest, SharedCookieJarReportEmpty) {
  const HistogramTester histogram_tester;
  const TimeDelta expected_stable_age;
  SharedReport(no_accounts, no_accounts, Time(), ReportingType::PERIODIC);
  ExpectSharedReportHistograms(ReportingType::PERIODIC, histogram_tester,
                               &expected_stable_age, 0, 0, 0, nullptr, false);
}

TEST_F(AccountInvestigatorTest, SharedCookieJarReportWithAccount) {
  signin_manager()->SignIn("1", "a", "A");
  base::Time now = base::Time::Now();
  pref_service()->SetDouble(prefs::kGaiaCookieChangedTime, now.ToDoubleT());
  const AccountRelation expected_relation(
      AccountRelation::ONE_OF_SIGNED_IN_MATCH_ANY_SIGNED_OUT);
  const HistogramTester histogram_tester;
  const TimeDelta expected_stable_age(TimeDelta::FromDays(1));
  SharedReport(both, no_accounts, now + TimeDelta::FromDays(1),
               ReportingType::ON_CHANGE);
  ExpectSharedReportHistograms(ReportingType::ON_CHANGE, histogram_tester,
                               &expected_stable_age, 2, 0, 2,
                               &expected_relation, false);
}

TEST_F(AccountInvestigatorTest, OnGaiaAccountsInCookieUpdatedError) {
  const HistogramTester histogram_tester;
  GoogleServiceAuthError error(GoogleServiceAuthError::SERVICE_UNAVAILABLE);
  investigator()->OnGaiaAccountsInCookieUpdated(just_one, no_accounts, error);
  EXPECT_EQ(0u, histogram_tester.GetTotalCountsForPrefix("Signin.").size());
}

TEST_F(AccountInvestigatorTest, OnGaiaAccountsInCookieUpdatedOnChange) {
  const HistogramTester histogram_tester;
  investigator()->OnGaiaAccountsInCookieUpdated(
      just_one, no_accounts, GoogleServiceAuthError::AuthErrorNone());
  ExpectSharedReportHistograms(ReportingType::ON_CHANGE, histogram_tester,
                               nullptr, 1, 0, 1, nullptr, false);
}

TEST_F(AccountInvestigatorTest, OnGaiaAccountsInCookieUpdatedSigninOnly) {
  // Initial update to simulate the update on first-time-run.
  investigator()->OnGaiaAccountsInCookieUpdated(
      no_accounts, no_accounts, GoogleServiceAuthError::AuthErrorNone());

  const HistogramTester histogram_tester;
  signin_manager()->SignIn("1", "a", "A");
  pref_service()->SetString(prefs::kGaiaCookieHash,
                            Hash(just_one, no_accounts));
  investigator()->OnGaiaAccountsInCookieUpdated(
      just_one, no_accounts, GoogleServiceAuthError::AuthErrorNone());
  EXPECT_EQ(1u, histogram_tester.GetTotalCountsForPrefix("Signin.").size());
  ExpectRelationReport(ReportingType::ON_CHANGE, histogram_tester,
                       AccountRelation::SINGLE_SIGNED_IN_MATCH_NO_SIGNED_OUT);
}

TEST_F(AccountInvestigatorTest,
       OnGaiaAccountsInCookieUpdatedSigninSignOutOfContent) {
  const HistogramTester histogram_tester;
  signin_manager()->SignIn("1", "a", "A");
  investigator()->OnGaiaAccountsInCookieUpdated(
      just_one, no_accounts, GoogleServiceAuthError::AuthErrorNone());
  ExpectRelationReport(ReportingType::ON_CHANGE, histogram_tester,
                       AccountRelation::SINGLE_SIGNED_IN_MATCH_NO_SIGNED_OUT);

  // Simulate a sign out of the content area.
  const HistogramTester histogram_tester2;
  investigator()->OnGaiaAccountsInCookieUpdated(
        no_accounts, just_one, GoogleServiceAuthError::AuthErrorNone());
  const AccountRelation expected_relation =
      AccountRelation::NO_SIGNED_IN_SINGLE_SIGNED_OUT_MATCH;
  ExpectSharedReportHistograms(ReportingType::ON_CHANGE, histogram_tester2,
                               nullptr, 0, 1, 1, &expected_relation, true);
}

TEST_F(AccountInvestigatorTest, Initialize) {
  EXPECT_FALSE(*previously_authenticated());
  EXPECT_FALSE(timer()->IsRunning());

  investigator()->Initialize();
  EXPECT_FALSE(*previously_authenticated());
  EXPECT_TRUE(timer()->IsRunning());

  investigator()->Shutdown();
  EXPECT_FALSE(timer()->IsRunning());
}

TEST_F(AccountInvestigatorTest, InitializeSignedIn) {
  signin_manager()->SignIn("1", "a", "A");
  EXPECT_FALSE(*previously_authenticated());

  investigator()->Initialize();
  EXPECT_TRUE(*previously_authenticated());
}

TEST_F(AccountInvestigatorTest, TryPeriodicReportStale) {
  investigator()->Initialize();
  cookie_manager_service()->set_list_accounts_stale_for_testing(true);
  cookie_manager_service()->SetListAccountsResponseOneAccount("f@bar.com", "1");

  const HistogramTester histogram_tester;
  TryPeriodicReport();
  EXPECT_TRUE(*periodic_pending());
  EXPECT_EQ(0u, histogram_tester.GetTotalCountsForPrefix("Signin.").size());

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(*periodic_pending());
  ExpectSharedReportHistograms(ReportingType::PERIODIC, histogram_tester,
                               nullptr, 1, 0, 1, nullptr, false);
}

TEST_F(AccountInvestigatorTest, TryPeriodicReportEmpty) {
  cookie_manager_service()->set_list_accounts_stale_for_testing(false);
  const HistogramTester histogram_tester;

  TryPeriodicReport();
  EXPECT_FALSE(*periodic_pending());
  ExpectSharedReportHistograms(ReportingType::PERIODIC, histogram_tester,
                               nullptr, 0, 0, 0, nullptr, false);
}

}  // namespace
