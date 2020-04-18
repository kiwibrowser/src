// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/service.h"

#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "chromeos/services/assistant/fake_assistant_manager_service_impl.h"
#include "services/identity/public/mojom/identity_manager.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace assistant {

namespace {
constexpr base::TimeDelta kDefaultTokenExpirationDelay =
    base::TimeDelta::FromMilliseconds(1000);
}

class FakeIdentityManager : identity::mojom::IdentityManager {
 public:
  FakeIdentityManager()
      : binding_(this),
        access_token_expriation_delay_(kDefaultTokenExpirationDelay) {}

  identity::mojom::IdentityManagerPtr CreateInterfacePtrAndBind() {
    identity::mojom::IdentityManagerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  void SetAccessTokenExpirationDelay(base::TimeDelta delay) {
    access_token_expriation_delay_ = delay;
  }

  int get_access_token_count() const { return get_access_token_count_; }

 private:
  // identity::mojom::IdentityManager:
  void GetPrimaryAccountInfo(GetPrimaryAccountInfoCallback callback) override {
    AccountInfo account_info;
    account_info.account_id = "account_id";
    account_info.gaia = "fakegaiaid";
    account_info.email = "fake@email";
    account_info.full_name = "full name";
    account_info.given_name = "given name";
    account_info.hosted_domain = "hosted_domain";
    account_info.locale = "en";
    account_info.picture_url = "http://fakepicture";

    identity::AccountState account_state;
    account_state.has_refresh_token = true;
    account_state.is_primary_account = true;

    std::move(callback).Run(account_info, account_state);
  }

  void GetPrimaryAccountWhenAvailable(
      GetPrimaryAccountWhenAvailableCallback callback) override {}
  void GetAccountInfoFromGaiaId(
      const std::string& gaia_id,
      GetAccountInfoFromGaiaIdCallback callback) override {}
  void GetAccounts(GetAccountsCallback callback) override {}
  void GetAccessToken(const std::string& account_id,
                      const ::identity::ScopeSet& scopes,
                      const std::string& consumer_id,
                      GetAccessTokenCallback callback) override {
    GoogleServiceAuthError auth_error(GoogleServiceAuthError::NONE);
    std::move(callback).Run("fake access token",
                            base::Time::Now() + access_token_expriation_delay_,
                            auth_error);
    ++get_access_token_count_;
  }

  mojo::Binding<identity::mojom::IdentityManager> binding_;

  base::TimeDelta access_token_expriation_delay_;

  int get_access_token_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FakeIdentityManager);
};

class ServiceTest : public testing::Test {
 public:
  ServiceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void SetUp() override {
    Test::SetUp();
    service_ = std::make_unique<Service>();
    fake_identity_manager_ = std::make_unique<FakeIdentityManager>();
    fake_assistant_manager_ptr_ = new FakeAssistantManagerServiceImpl();

    service_->SetIdentityManagerForTesting(
        fake_identity_manager_->CreateInterfacePtrAndBind());
    service_->SetAssistantManagerForTesting(
        base::WrapUnique(fake_assistant_manager_ptr_));

    service_->RequestAccessToken();
  }

  void TearDown() override {
    Test::TearDown();
    scoped_task_environment_.RunUntilIdle();
  }

  Service* service() { return service_.get(); }

  FakeIdentityManager* identity_manager() {
    return fake_identity_manager_.get();
  }

  FakeAssistantManagerServiceImpl* assistant_manager_service() {
    return fake_assistant_manager_ptr_;
  }

 private:
  std::unique_ptr<Service> service_;

  std::unique_ptr<FakeIdentityManager> fake_identity_manager_;

  FakeAssistantManagerServiceImpl* fake_assistant_manager_ptr_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(ServiceTest);
};

TEST_F(ServiceTest, RefreshTokenAfterExpire) {
  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto timer =
      std::make_unique<base::OneShotTimer>(task_runner->GetMockTickClock());
  timer->SetTaskRunner(task_runner);
  service()->SetTimerForTesting(std::move(timer));

  base::RunLoop().RunUntilIdle();

  auto current_count = identity_manager()->get_access_token_count();
  task_runner->FastForwardBy(kDefaultTokenExpirationDelay / 2);
  base::RunLoop().RunUntilIdle();

  // Before token expire, should not request new token.
  EXPECT_EQ(identity_manager()->get_access_token_count(), current_count);

  task_runner->FastForwardBy(kDefaultTokenExpirationDelay);
  base::RunLoop().RunUntilIdle();

  // After token expire, should request once.
  EXPECT_EQ(identity_manager()->get_access_token_count(), current_count + 1);
}

}  // namespace assistant
}  // namespace chromeos
