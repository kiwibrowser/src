// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <ostream>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/policy/affiliation_test_helper.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/net/nss_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/account_id/account_id.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_launcher.h"
#include "content/public/test/test_utils.h"
#include "crypto/scoped_test_system_nss_key_slot.h"
#include "net/cert/nss_cert_database.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
class ResourceContext;
}

namespace policy {

namespace {

constexpr char kAffiliatedUser[] = "affiliated-user@example.com";
constexpr char kAffiliatedUserGaiaId[] = "1234567890";
constexpr char kAffiliationID[] = "some-affiliation-id";
constexpr char kAnotherAffiliationID[] = "another-affiliation-id";

struct Params {
  explicit Params(bool affiliated) : affiliated_(affiliated) {}

  friend std::ostream& operator<<(std::ostream& os, const Params& p) {
    os << "{ affiliated: " << p.affiliated_ << " }";
    return os;
  }

  bool affiliated_;
};

void CheckIsSystemSlotAvailableOnIOThreadWithCertDb(
    bool* out_system_slot_available,
    base::OnceClosure done_closure,
    net::NSSCertDatabase* cert_db) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  *out_system_slot_available = cert_db->GetSystemSlot() != nullptr;
  std::move(done_closure).Run();
}

void CheckIsSystemSlotAvailableOnIOThread(
    content::ResourceContext* resource_context,
    bool* out_system_slot_available,
    base::OnceClosure done_closure) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  auto did_get_cert_db_callback = base::BindRepeating(
      &CheckIsSystemSlotAvailableOnIOThreadWithCertDb,
      out_system_slot_available,
      base::AdaptCallbackForRepeating(std::move(done_closure)));

  net::NSSCertDatabase* cert_db = GetNSSCertDatabaseForResourceContext(
      resource_context, did_get_cert_db_callback);
  if (cert_db)
    did_get_cert_db_callback.Run(cert_db);
}

// Returns true if the system token is available for |profile|. System token
// availability is one of the aspects which are tied to user affiliation. It is
// an interesting one to test because it is evaluated very early (in
// ProfileIOData::InitializeOnUIThread).
bool IsSystemSlotAvailable(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::RunLoop run_loop;
  bool system_slot_available = false;
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(CheckIsSystemSlotAvailableOnIOThread,
                     profile->GetResourceContext(), &system_slot_available,
                     run_loop.QuitClosure()));
  run_loop.Run();
  return system_slot_available;
}

}  // namespace

class UserAffiliationBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<Params> {
 public:
  UserAffiliationBrowserTest()
      : account_id_(AccountId::FromUserEmailGaiaId(kAffiliatedUser,
                                                   kAffiliatedUserGaiaId)) {
    set_exit_when_last_browser_closes(false);
  }

 protected:
  // InProcessBrowserTest
  void SetUpCommandLine(base::CommandLine* command_line) override {
    if (content::IsPreTest()) {
      affiliation_test_helper::AppendCommandLineSwitchesForLoginManager(
          command_line);
    } else {
      const cryptohome::Identification cryptohome_id(account_id_);
      command_line->AppendSwitchASCII(chromeos::switches::kLoginUser,
                                      cryptohome_id.id());
      command_line->AppendSwitchASCII(
          chromeos::switches::kLoginProfile,
          chromeos::CryptohomeClient::GetStubSanitizedUsername(cryptohome_id));
    }
  }

  // InProcessBrowserTest
  void SetUpInProcessBrowserTestFixture() override {
    chromeos::FakeSessionManagerClient* fake_session_manager_client =
        new chromeos::FakeSessionManagerClient;
    chromeos::DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        base::WrapUnique<chromeos::SessionManagerClient>(
            fake_session_manager_client));

    UserPolicyBuilder user_policy;
    DevicePolicyCrosTestHelper test_helper;

    std::set<std::string> device_affiliation_ids;
    device_affiliation_ids.insert(kAffiliationID);
    affiliation_test_helper::SetDeviceAffiliationID(
        &test_helper, fake_session_manager_client, device_affiliation_ids);

    std::set<std::string> user_affiliation_ids;
    if (GetParam().affiliated_) {
      user_affiliation_ids.insert(kAffiliationID);
    } else {
      user_affiliation_ids.insert(kAnotherAffiliationID);
    }
    affiliation_test_helper::SetUserAffiliationIDs(
        &user_policy, fake_session_manager_client, account_id_,
        user_affiliation_ids);

    // Set retry delay to prevent timeouts.
    policy::DeviceManagementService::SetRetryDelayForTesting(0);
  }

  void TearDownOnMainThread() override {
    InProcessBrowserTest::TearDownOnMainThread();

    TearDownTestSystemSlot();
  }

  const AccountId account_id_;

  void SetUpTestSystemSlot() {
    bool system_slot_constructed_successfully = false;
    base::RunLoop loop;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&UserAffiliationBrowserTest::SetUpTestSystemSlotOnIO,
                       base::Unretained(this),
                       &system_slot_constructed_successfully),
        loop.QuitClosure());
    loop.Run();
    ASSERT_TRUE(system_slot_constructed_successfully);
  }

 private:
  void SetUpTestSystemSlotOnIO(bool* out_system_slot_constructed_successfully) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    test_system_slot_ = std::make_unique<crypto::ScopedTestSystemNSSKeySlot>();
    *out_system_slot_constructed_successfully =
        test_system_slot_->ConstructedSuccessfully();
  }

  void TearDownTestSystemSlot() {
    if (!test_system_slot_)
      return;

    base::RunLoop loop;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&UserAffiliationBrowserTest::TearDownTestSystemSlotOnIO,
                       base::Unretained(this)),
        loop.QuitClosure());
    loop.Run();
  }

  void TearDownTestSystemSlotOnIO() { test_system_slot_.reset(); }

  std::unique_ptr<crypto::ScopedTestSystemNSSKeySlot> test_system_slot_;

  DISALLOW_COPY_AND_ASSIGN(UserAffiliationBrowserTest);
};

IN_PROC_BROWSER_TEST_P(UserAffiliationBrowserTest, PRE_PRE_Affiliated) {
  affiliation_test_helper::PreLoginUser(account_id_);
}

// This part of the test performs a regular sign-in through the login manager.
IN_PROC_BROWSER_TEST_P(UserAffiliationBrowserTest, PRE_Affiliated) {
  affiliation_test_helper::LoginUser(account_id_);

  EXPECT_EQ(
      GetParam().affiliated_,
      user_manager::UserManager::Get()->FindUser(account_id_)->IsAffiliated());

  // Also test system slot availability, which is tied to user affiliation. This
  // gives us additional information, because for the system slot to be
  // available for an affiliated user, IsAffiliated() must already be returning
  // true in the ProfileIOData constructor.
  ASSERT_NO_FATAL_FAILURE(SetUpTestSystemSlot());
  EXPECT_EQ(GetParam().affiliated_,
            IsSystemSlotAvailable(ProfileManager::GetPrimaryUserProfile()));
}

// This part of the test performs a direct sign-in into the user profile using
// the --login-user command-line switch, simulating the restart after crash
// behavior on Chrome OS.
// See SetUpCommandLine for details.
IN_PROC_BROWSER_TEST_P(UserAffiliationBrowserTest, Affiliated) {
  // Note: We don't log in the user, because the login has implicitly been
  // performed using a command-line flag (see SetUpCommandLine).

  EXPECT_EQ(
      GetParam().affiliated_,
      user_manager::UserManager::Get()->FindUser(account_id_)->IsAffiliated());

  // Also test system slot availability, which is tied to user affiliation. This
  // gives us additional information, because for the system slot to be
  // available for an affiliated user, IsAffiliated() must already be returning
  // true in the ProfileIOData constructor.
  ASSERT_NO_FATAL_FAILURE(SetUpTestSystemSlot());
  EXPECT_EQ(GetParam().affiliated_,
            IsSystemSlotAvailable(ProfileManager::GetPrimaryUserProfile()));
}

INSTANTIATE_TEST_CASE_P(AffiliationCheck,
                        UserAffiliationBrowserTest,
                        ::testing::Values(Params(true), Params(false)));

}  // namespace policy
