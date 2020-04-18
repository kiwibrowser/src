// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/drivefs/drivefs_host.h"

#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/components/drivefs/pending_connection_manager.h"
#include "chromeos/disks/mock_disk_mount_manager.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drivefs {
namespace {

using testing::_;

class TestingMojoConnectionDelegate
    : public DriveFsHost::MojoConnectionDelegate {
 public:
  TestingMojoConnectionDelegate(
      mojom::DriveFsBootstrapPtrInfo pending_bootstrap)
      : pending_bootstrap_(std::move(pending_bootstrap)) {}

  mojom::DriveFsBootstrapPtrInfo InitializeMojoConnection() override {
    return std::move(pending_bootstrap_);
  }

  void AcceptMojoConnection(base::ScopedFD handle) override {}

 private:
  mojom::DriveFsBootstrapPtrInfo pending_bootstrap_;
};

class ForwardingOAuth2MintTokenFlow;

ACTION_P(SucceedMintToken, token) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindLambdaForTesting([=] { arg0->OnMintTokenSuccess(token, 0); }));
}

ACTION_P(FailMintToken, error) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindLambdaForTesting([=] {
        arg0->OnMintTokenFailure(GoogleServiceAuthError(error));
      }));
}

class MockOAuth2MintTokenFlow {
 public:
  MockOAuth2MintTokenFlow() = default;

  void ExpectStartAndSucceed(const std::string& expected_token,
                             const std::string& token_to_return) {
    EXPECT_CALL(*this, Start(_, expected_token))
        .WillOnce(SucceedMintToken(token_to_return));
  }

  void ExpectStartAndFail(const std::string& expected_token,
                          GoogleServiceAuthError::State error) {
    EXPECT_CALL(*this, Start(_, expected_token)).WillOnce(FailMintToken(error));
  }

  void ExpectNoStartCalls() { EXPECT_CALL(*this, Start(_, _)).Times(0); }

 private:
  friend class ForwardingOAuth2MintTokenFlow;
  MOCK_METHOD2(Start,
               void(OAuth2MintTokenFlow::Delegate* delegate,
                    const std::string& access_token));

  DISALLOW_COPY_AND_ASSIGN(MockOAuth2MintTokenFlow);
};

class ForwardingOAuth2MintTokenFlow : public OAuth2MintTokenFlow {
 public:
  ForwardingOAuth2MintTokenFlow(OAuth2MintTokenFlow::Delegate* delegate,
                                MockOAuth2MintTokenFlow* mock)
      : OAuth2MintTokenFlow(delegate, {}), delegate_(delegate), mock_(mock) {}

  void Start(net::URLRequestContextGetter* context_getter,
             const std::string& access_token) override {
    EXPECT_EQ(nullptr, context_getter);
    mock_->Start(delegate_, access_token);
  }

 private:
  Delegate* const delegate_;
  MockOAuth2MintTokenFlow* mock_;
};

class TestingDriveFsHostDelegate : public DriveFsHost::Delegate {
 public:
  TestingDriveFsHostDelegate(
      std::unique_ptr<service_manager::Connector> connector,
      const AccountId& account_id)
      : connector_(std::move(connector)), account_id_(account_id) {}

  MockOAuth2MintTokenFlow& mock_flow() { return mock_flow_; }

  void set_pending_bootstrap(mojom::DriveFsBootstrapPtrInfo pending_bootstrap) {
    pending_bootstrap_ = std::move(pending_bootstrap);
  }

  // DriveFsHost::Delegate:
  MOCK_METHOD1(OnMounted, void(const base::FilePath&));

 private:
  // DriveFsHost::Delegate:
  net::URLRequestContextGetter* GetRequestContext() override { return nullptr; }
  service_manager::Connector* GetConnector() override {
    return connector_.get();
  }
  const AccountId& GetAccountId() override { return account_id_; }

  std::unique_ptr<OAuth2MintTokenFlow> CreateMintTokenFlow(
      OAuth2MintTokenFlow::Delegate* delegate,
      const std::string& client_id,
      const std::string& app_id,
      const std::vector<std::string>& scopes) override {
    EXPECT_EQ("client ID", client_id);
    EXPECT_EQ("app ID", app_id);
    EXPECT_EQ((std::vector<std::string>{"scope1", "scope2"}), scopes);
    return std::make_unique<ForwardingOAuth2MintTokenFlow>(delegate,
                                                           &mock_flow_);
  }

  std::unique_ptr<DriveFsHost::MojoConnectionDelegate>
  CreateMojoConnectionDelegate() override {
    DCHECK(pending_bootstrap_);
    return std::make_unique<TestingMojoConnectionDelegate>(
        std::move(pending_bootstrap_));
  }

  const std::unique_ptr<service_manager::Connector> connector_;
  const AccountId account_id_;
  MockOAuth2MintTokenFlow mock_flow_;
  mojom::DriveFsBootstrapPtrInfo pending_bootstrap_;

  DISALLOW_COPY_AND_ASSIGN(TestingDriveFsHostDelegate);
};

class MockIdentityManager {
 public:
  MOCK_METHOD3(
      GetAccessToken,
      std::pair<base::Optional<std::string>, GoogleServiceAuthError::State>(
          const std::string& account_id,
          const ::identity::ScopeSet& scopes,
          const std::string& consumer_id));
};

class FakeIdentityService
    : public identity::mojom::IdentityManagerInterceptorForTesting,
      public service_manager::Service {
 public:
  explicit FakeIdentityService(MockIdentityManager* mock) : mock_(mock) {
    binder_registry_.AddInterface(
        base::BindRepeating(&FakeIdentityService::BindIdentityManagerRequest,
                            base::Unretained(this)));
  }

 private:
  void OnBindInterface(const service_manager::BindSourceInfo& source,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    binder_registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void BindIdentityManagerRequest(
      identity::mojom::IdentityManagerRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // identity::mojom::IdentityManagerInterceptorForTesting overrides:
  void GetAccessToken(const std::string& account_id,
                      const ::identity::ScopeSet& scopes,
                      const std::string& consumer_id,
                      GetAccessTokenCallback callback) override {
    auto result = mock_->GetAccessToken(account_id, scopes, consumer_id);
    std::move(callback).Run(std::move(result.first), base::Time::Now(),
                            GoogleServiceAuthError(result.second));
  }

  IdentityManager* GetForwardingInterface() override {
    NOTREACHED();
    return nullptr;
  }

  MockIdentityManager* const mock_;
  service_manager::BinderRegistry binder_registry_;
  mojo::BindingSet<identity::mojom::IdentityManager> bindings_;

  DISALLOW_COPY_AND_ASSIGN(FakeIdentityService);
};

ACTION_P(RunQuitClosure, quit) {
  std::move(*quit).Run();
}

class DriveFsHostTest : public ::testing::Test,
                        public mojom::DriveFsBootstrap,
                        public mojom::DriveFs {
 public:
  DriveFsHostTest() : bootstrap_binding_(this), binding_(this) {}

 protected:
  void SetUp() override {
    testing::Test::SetUp();
    profile_path_ = base::FilePath(FILE_PATH_LITERAL("/path/to/profile"));
    account_id_ = AccountId::FromUserEmailGaiaId("test@example.com", "ID");

    disk_manager_ = new chromeos::disks::MockDiskMountManager;
    // Takes ownership of |disk_manager_|.
    chromeos::disks::DiskMountManager::InitializeForTesting(disk_manager_);
    connector_factory_ =
        service_manager::TestConnectorFactory::CreateForUniqueService(
            std::make_unique<FakeIdentityService>(&mock_identity_manager_));
    host_delegate_ = std::make_unique<TestingDriveFsHostDelegate>(
        connector_factory_->CreateConnector(), account_id_);
    host_ = std::make_unique<DriveFsHost>(profile_path_, host_delegate_.get());
  }

  void TearDown() override {
    host_.reset();
    disk_manager_ = nullptr;
    chromeos::disks::DiskMountManager::Shutdown();
  }

  void DispatchMountEvent(
      chromeos::disks::DiskMountManager::MountEvent event,
      chromeos::MountError error_code,
      const chromeos::disks::DiskMountManager::MountPointInfo& mount_info) {
    static_cast<chromeos::disks::DiskMountManager::Observer&>(*host_)
        .OnMountEvent(event, error_code, mount_info);
  }

  std::string StartMount() {
    std::string source;
    EXPECT_CALL(
        *disk_manager_,
        MountPath(testing::StartsWith("drivefs://"), "", "drivefs-g-ID",
                  testing::Contains("datadir=/path/to/profile/GCache/v2/g-ID"),
                  _, chromeos::MOUNT_ACCESS_MODE_READ_WRITE))
        .WillOnce(testing::SaveArg<0>(&source));

    mojom::DriveFsBootstrapPtrInfo bootstrap;
    bootstrap_binding_.Bind(mojo::MakeRequest(&bootstrap));
    host_delegate_->set_pending_bootstrap(std::move(bootstrap));
    pending_delegate_request_ = mojo::MakeRequest(&delegate_ptr_);

    EXPECT_TRUE(host_->Mount());
    testing::Mock::VerifyAndClear(&disk_manager_);

    return source.substr(strlen("drivefs://"));
  }

  void DispatchMountSuccessEvent(const std::string& token) {
    DispatchMountEvent(chromeos::disks::DiskMountManager::MOUNTING,
                       chromeos::MOUNT_ERROR_NONE,
                       {base::StrCat({"drivefs://", token}),
                        "/media/drivefsroot/g-ID",
                        chromeos::MOUNT_TYPE_NETWORK_STORAGE,
                        {}});
  }

  void SendOnMounted() { delegate_ptr_->OnMounted(); }

  void DoMount() {
    auto token = StartMount();
    DispatchMountSuccessEvent(token);

    ASSERT_TRUE(PendingConnectionManager::Get().OpenIpcChannel(token, {}));
    {
      base::RunLoop run_loop;
      bootstrap_binding_.set_connection_error_handler(run_loop.QuitClosure());
      run_loop.Run();
    }
    base::RunLoop run_loop;
    base::OnceClosure quit_closure = run_loop.QuitClosure();
    EXPECT_CALL(*host_delegate_,
                OnMounted(base::FilePath("/media/drivefsroot/g-ID")))
        .WillOnce(RunQuitClosure(&quit_closure));
    SendOnMounted();
    run_loop.Run();
    ASSERT_TRUE(host_->IsMounted());
  }

  void Init(mojom::DriveFsConfigurationPtr config,
            mojom::DriveFsRequest drive_fs,
            mojom::DriveFsDelegatePtr delegate) override {
    EXPECT_EQ("test@example.com", config->user_email);
    binding_.Bind(std::move(drive_fs));
    mojo::FuseInterface(std::move(pending_delegate_request_),
                        delegate.PassInterface());
  }

  base::FilePath profile_path_;
  base::test::ScopedTaskEnvironment task_environment_;
  AccountId account_id_;
  chromeos::disks::MockDiskMountManager* disk_manager_;
  MockIdentityManager mock_identity_manager_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<TestingDriveFsHostDelegate> host_delegate_;
  std::unique_ptr<DriveFsHost> host_;

  mojo::Binding<mojom::DriveFsBootstrap> bootstrap_binding_;
  mojo::Binding<mojom::DriveFs> binding_;
  mojom::DriveFsDelegatePtr delegate_ptr_;
  mojom::DriveFsDelegateRequest pending_delegate_request_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DriveFsHostTest);
};

TEST_F(DriveFsHostTest, Basic) {
  EXPECT_FALSE(host_->IsMounted());

  ASSERT_NO_FATAL_FAILURE(DoMount());

  EXPECT_EQ(base::FilePath("/media/drivefsroot/g-ID"), host_->GetMountPath());
}

TEST_F(DriveFsHostTest, OnMountedBeforeMountEvent) {
  auto token = StartMount();
  ASSERT_TRUE(PendingConnectionManager::Get().OpenIpcChannel(token, {}));
  SendOnMounted();
  EXPECT_CALL(*host_delegate_, OnMounted(_)).Times(0);
  delegate_ptr_.FlushForTesting();

  testing::Mock::VerifyAndClear(host_delegate_.get());

  EXPECT_FALSE(host_->IsMounted());

  EXPECT_CALL(*host_delegate_,
              OnMounted(base::FilePath("/media/drivefsroot/g-ID")));

  DispatchMountSuccessEvent(token);

  ASSERT_TRUE(host_->IsMounted());
  EXPECT_EQ(base::FilePath("/media/drivefsroot/g-ID"), host_->GetMountPath());
}

TEST_F(DriveFsHostTest, UnmountAfterMountComplete) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  EXPECT_CALL(*disk_manager_, UnmountPath("/media/drivefsroot/g-ID",
                                          chromeos::UNMOUNT_OPTIONS_NONE, _));
  base::RunLoop run_loop;
  binding_.set_connection_error_handler(run_loop.QuitClosure());
  host_->Unmount();
  run_loop.Run();
}

TEST_F(DriveFsHostTest, UnmountBeforeMountEvent) {
  auto token = StartMount();
  EXPECT_FALSE(host_->IsMounted());
  host_->Unmount();
  EXPECT_FALSE(PendingConnectionManager::Get().OpenIpcChannel(token, {}));
}

TEST_F(DriveFsHostTest, UnmountBeforeMojoConnection) {
  auto token = StartMount();
  DispatchMountSuccessEvent(token);

  EXPECT_FALSE(host_->IsMounted());
  EXPECT_CALL(*disk_manager_, UnmountPath("/media/drivefsroot/g-ID",
                                          chromeos::UNMOUNT_OPTIONS_NONE, _));

  host_->Unmount();
  EXPECT_FALSE(PendingConnectionManager::Get().OpenIpcChannel(token, {}));
}

TEST_F(DriveFsHostTest, DestroyBeforeMountEvent) {
  auto token = StartMount();
  EXPECT_CALL(*disk_manager_, UnmountPath(_, _, _)).Times(0);

  host_.reset();
  EXPECT_FALSE(PendingConnectionManager::Get().OpenIpcChannel(token, {}));
}

TEST_F(DriveFsHostTest, DestroyBeforeMojoConnection) {
  auto token = StartMount();
  DispatchMountSuccessEvent(token);
  EXPECT_CALL(*disk_manager_, UnmountPath("/media/drivefsroot/g-ID",
                                          chromeos::UNMOUNT_OPTIONS_NONE, _));

  host_.reset();
  EXPECT_FALSE(PendingConnectionManager::Get().OpenIpcChannel(token, {}));
}

TEST_F(DriveFsHostTest, ObserveOtherMount) {
  auto token = StartMount();
  EXPECT_CALL(*disk_manager_, UnmountPath(_, _, _)).Times(0);

  DispatchMountEvent(chromeos::disks::DiskMountManager::MOUNTING,
                     chromeos::MOUNT_ERROR_DIRECTORY_CREATION_FAILED,
                     {"some/other/mount/event",
                      "/some/other/mount/point",
                      chromeos::MOUNT_TYPE_DEVICE,
                      {}});
  DispatchMountEvent(chromeos::disks::DiskMountManager::UNMOUNTING,
                     chromeos::MOUNT_ERROR_NONE,
                     {base::StrCat({"drivefs://", token}),
                      "/media/drivefsroot/g-ID",
                      chromeos::MOUNT_TYPE_NETWORK_STORAGE,
                      {}});
  EXPECT_FALSE(host_->IsMounted());
  host_->Unmount();
}

TEST_F(DriveFsHostTest, MountError) {
  auto token = StartMount();
  EXPECT_CALL(*disk_manager_, UnmountPath(_, _, _)).Times(0);

  DispatchMountEvent(chromeos::disks::DiskMountManager::MOUNTING,
                     chromeos::MOUNT_ERROR_DIRECTORY_CREATION_FAILED,
                     {base::StrCat({"drivefs://", token}),
                      "/media/drivefsroot/g-ID",
                      chromeos::MOUNT_TYPE_NETWORK_STORAGE,
                      {}});
  EXPECT_FALSE(host_->IsMounted());
  EXPECT_FALSE(PendingConnectionManager::Get().OpenIpcChannel(token, {}));
}

TEST_F(DriveFsHostTest, MountWhileAlreadyMounted) {
  DoMount();
  EXPECT_FALSE(host_->Mount());
}

TEST_F(DriveFsHostTest, UnsupportedAccountTypes) {
  EXPECT_CALL(*disk_manager_, MountPath(_, _, _, _, _, _)).Times(0);
  const AccountId unsupported_accounts[] = {
      AccountId::FromGaiaId("ID"),
      AccountId::FromUserEmail("test2@example.com"),
      AccountId::AdFromObjGuid("ID"),
  };
  for (auto& account : unsupported_accounts) {
    host_delegate_ = std::make_unique<TestingDriveFsHostDelegate>(
        connector_factory_->CreateConnector(), account);
    host_ = std::make_unique<DriveFsHost>(profile_path_, host_delegate_.get());
    EXPECT_FALSE(host_->Mount());
    EXPECT_FALSE(host_->IsMounted());
  }
}

TEST_F(DriveFsHostTest, GetAccessToken_Success) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  EXPECT_CALL(mock_identity_manager_,
              GetAccessToken("test@example.com", _, "drivefs"))
      .WillOnce(testing::Return(
          std::make_pair("chrome token", GoogleServiceAuthError::NONE)));
  host_delegate_->mock_flow().ExpectStartAndSucceed("chrome token",
                                                    "auth token");

  base::RunLoop run_loop;
  auto quit_closure = run_loop.QuitClosure();
  delegate_ptr_->GetAccessToken(
      "client ID", "app ID", {"scope1", "scope2"},
      base::BindLambdaForTesting(
          [&](mojom::AccessTokenStatus status, const std::string& token) {
            EXPECT_EQ(mojom::AccessTokenStatus::kSuccess, status);
            EXPECT_EQ("auth token", token);
            std::move(quit_closure).Run();
          }));
  run_loop.Run();
}

TEST_F(DriveFsHostTest, GetAccessToken_ParallelRequests) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  base::RunLoop run_loop;
  auto quit_closure = run_loop.QuitClosure();
  delegate_ptr_->GetAccessToken(
      "client ID", "app ID", {"scope1", "scope2"},
      base::BindOnce(
          [](mojom::AccessTokenStatus status, const std::string& token) {
            FAIL() << "Unexpected callback";
          }));
  delegate_ptr_->GetAccessToken(
      "client ID", "app ID", {"scope1", "scope2"},
      base::BindLambdaForTesting(
          [&](mojom::AccessTokenStatus status, const std::string& token) {
            EXPECT_EQ(mojom::AccessTokenStatus::kTransientError, status);
            EXPECT_TRUE(token.empty());
            std::move(quit_closure).Run();
          }));
  run_loop.Run();
}

TEST_F(DriveFsHostTest, GetAccessToken_SequentialRequests) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  for (int i = 0; i < 3; ++i) {
    EXPECT_CALL(mock_identity_manager_,
                GetAccessToken("test@example.com", _, "drivefs"))
        .WillOnce(testing::Return(
            std::make_pair("chrome token", GoogleServiceAuthError::NONE)));
    host_delegate_->mock_flow().ExpectStartAndSucceed("chrome token",
                                                      "auth token");

    base::RunLoop run_loop;
    auto quit_closure = run_loop.QuitClosure();
    delegate_ptr_->GetAccessToken(
        "client ID", "app ID", {"scope1", "scope2"},
        base::BindLambdaForTesting(
            [&](mojom::AccessTokenStatus status, const std::string& token) {
              EXPECT_EQ(mojom::AccessTokenStatus::kSuccess, status);
              EXPECT_EQ("auth token", token);
              std::move(quit_closure).Run();
            }));
    run_loop.Run();
  }
  for (int i = 0; i < 3; ++i) {
    EXPECT_CALL(mock_identity_manager_,
                GetAccessToken("test@example.com", _, "drivefs"))
        .WillOnce(testing::Return(std::make_pair(
            base::nullopt, GoogleServiceAuthError::ACCOUNT_DISABLED)));
    host_delegate_->mock_flow().ExpectNoStartCalls();

    base::RunLoop run_loop;
    auto quit_closure = run_loop.QuitClosure();
    delegate_ptr_->GetAccessToken(
        "client ID", "app ID", {"scope1", "scope2"},
        base::BindLambdaForTesting(
            [&](mojom::AccessTokenStatus status, const std::string& token) {
              EXPECT_EQ(mojom::AccessTokenStatus::kAuthError, status);
              EXPECT_TRUE(token.empty());
              std::move(quit_closure).Run();
            }));
    run_loop.Run();
  }
}

TEST_F(DriveFsHostTest, GetAccessToken_GetAccessTokenFailure_Permanent) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  EXPECT_CALL(mock_identity_manager_,
              GetAccessToken("test@example.com", _, "drivefs"))
      .WillOnce(testing::Return(std::make_pair(
          base::nullopt, GoogleServiceAuthError::ACCOUNT_DISABLED)));
  host_delegate_->mock_flow().ExpectNoStartCalls();

  base::RunLoop run_loop;
  auto quit_closure = run_loop.QuitClosure();
  delegate_ptr_->GetAccessToken(
      "client ID", "app ID", {"scope1", "scope2"},
      base::BindLambdaForTesting(
          [&](mojom::AccessTokenStatus status, const std::string& token) {
            EXPECT_EQ(mojom::AccessTokenStatus::kAuthError, status);
            EXPECT_TRUE(token.empty());
            std::move(quit_closure).Run();
          }));
  run_loop.Run();
}

TEST_F(DriveFsHostTest, GetAccessToken_GetAccessTokenFailure_Transient) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  EXPECT_CALL(mock_identity_manager_,
              GetAccessToken("test@example.com", _, "drivefs"))
      .WillOnce(testing::Return(std::make_pair(
          base::nullopt, GoogleServiceAuthError::SERVICE_UNAVAILABLE)));
  host_delegate_->mock_flow().ExpectNoStartCalls();

  base::RunLoop run_loop;
  auto quit_closure = run_loop.QuitClosure();
  delegate_ptr_->GetAccessToken(
      "client ID", "app ID", {"scope1", "scope2"},
      base::BindLambdaForTesting(
          [&](mojom::AccessTokenStatus status, const std::string& token) {
            EXPECT_EQ(mojom::AccessTokenStatus::kTransientError, status);
            EXPECT_TRUE(token.empty());
            std::move(quit_closure).Run();
          }));
  run_loop.Run();
}

TEST_F(DriveFsHostTest, GetAccessToken_MintTokenFailure_Permanent) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  EXPECT_CALL(mock_identity_manager_,
              GetAccessToken("test@example.com", _, "drivefs"))
      .WillOnce(testing::Return(
          std::make_pair("chrome token", GoogleServiceAuthError::NONE)));
  host_delegate_->mock_flow().ExpectStartAndFail(
      "chrome token", GoogleServiceAuthError::ACCOUNT_DISABLED);

  base::RunLoop run_loop;
  auto quit_closure = run_loop.QuitClosure();
  delegate_ptr_->GetAccessToken(
      "client ID", "app ID", {"scope1", "scope2"},
      base::BindLambdaForTesting(
          [&](mojom::AccessTokenStatus status, const std::string& token) {
            EXPECT_EQ(mojom::AccessTokenStatus::kAuthError, status);
            EXPECT_TRUE(token.empty());
            std::move(quit_closure).Run();
          }));
  run_loop.Run();
}

TEST_F(DriveFsHostTest, GetAccessToken_MintTokenFailure_Transient) {
  ASSERT_NO_FATAL_FAILURE(DoMount());

  EXPECT_CALL(mock_identity_manager_,
              GetAccessToken("test@example.com", _, "drivefs"))
      .WillOnce(testing::Return(
          std::make_pair("chrome token", GoogleServiceAuthError::NONE)));
  host_delegate_->mock_flow().ExpectStartAndFail(
      "chrome token", GoogleServiceAuthError::SERVICE_UNAVAILABLE);

  base::RunLoop run_loop;
  auto quit_closure = run_loop.QuitClosure();
  delegate_ptr_->GetAccessToken(
      "client ID", "app ID", {"scope1", "scope2"},
      base::BindLambdaForTesting(
          [&](mojom::AccessTokenStatus status, const std::string& token) {
            EXPECT_EQ(mojom::AccessTokenStatus::kTransientError, status);
            EXPECT_TRUE(token.empty());
            std::move(quit_closure).Run();
          }));
  run_loop.Run();
}

}  // namespace
}  // namespace drivefs
