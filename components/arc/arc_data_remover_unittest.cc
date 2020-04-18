// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_data_remover.h"

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "components/account_id/account_id.h"
#include "components/arc/arc_prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {
namespace {

class ArcDataRemoverTest : public testing::Test {
 public:
  ArcDataRemoverTest() = default;

  void SetUp() override {
    chromeos::DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        std::make_unique<chromeos::FakeSessionManagerClient>());
    chromeos::DBusThreadManager::Initialize();

    prefs::RegisterProfilePrefs(prefs_.registry());
  }

  void TearDown() override { chromeos::DBusThreadManager::Shutdown(); }

  PrefService* prefs() { return &prefs_; }

  const cryptohome::Identification& cryptohome_id() const {
    return cryptohome_id_;
  }

  chromeos::FakeSessionManagerClient* session_manager_client() {
    return static_cast<chromeos::FakeSessionManagerClient*>(
        chromeos::DBusThreadManager::Get()->GetSessionManagerClient());
  }

 private:
  TestingPrefServiceSimple prefs_;
  const cryptohome::Identification cryptohome_id_{EmptyAccountId()};

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(ArcDataRemoverTest);
};

TEST_F(ArcDataRemoverTest, NotScheduled) {
  ArcDataRemover data_remover(prefs(), cryptohome_id());

  base::RunLoop loop;
  data_remover.Run(base::BindOnce(
      [](base::RunLoop* loop, base::Optional<bool> result) {
        EXPECT_EQ(result, base::nullopt);
        loop->Quit();
      },
      &loop));
  loop.Run();
}

TEST_F(ArcDataRemoverTest, Success) {
  session_manager_client()->set_arc_available(true);

  ArcDataRemover data_remover(prefs(), cryptohome_id());
  data_remover.Schedule();

  base::RunLoop loop;
  data_remover.Run(base::BindOnce(
      [](base::RunLoop* loop, base::Optional<bool> result) {
        EXPECT_EQ(result, base::make_optional(true));
        loop->Quit();
      },
      &loop));
  loop.Run();
}

TEST_F(ArcDataRemoverTest, Fail) {
  ArcDataRemover data_remover(prefs(), cryptohome_id());
  data_remover.Schedule();

  base::RunLoop loop;
  data_remover.Run(base::BindOnce(
      [](base::RunLoop* loop, base::Optional<bool> result) {
        EXPECT_EQ(result, base::make_optional(false));
        loop->Quit();
      },
      &loop));
  loop.Run();
}

TEST_F(ArcDataRemoverTest, PrefPersistsAcrossInstances) {
  {
    ArcDataRemover data_remover(prefs(), cryptohome_id());
    data_remover.Schedule();
    EXPECT_TRUE(data_remover.IsScheduledForTesting());
  }

  {
    ArcDataRemover data_remover(prefs(), cryptohome_id());
    EXPECT_TRUE(data_remover.IsScheduledForTesting());
  }
}

}  // namespace
}  // namespace arc
