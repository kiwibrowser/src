// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "media/cdm/default_cdm_factory.h"
#include "media/media_buildflags.h"
#include "media/mojo/interfaces/constants.mojom.h"
#include "media/mojo/services/cdm_service.h"
#include "media/mojo/services/media_interface_provider.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace media {

namespace {

using testing::_;
using testing::Invoke;
using testing::InvokeWithoutArgs;

MATCHER_P(MatchesResult, success, "") {
  return arg->success == success;
}

const char kClearKeyKeySystem[] = "org.w3.clearkey";
const char kInvalidKeySystem[] = "invalid.key.system";
const char kSecurityOrigin[] = "https://foo.com";

class MockCdmServiceClient : public media::CdmService::Client {
 public:
  MockCdmServiceClient() = default;
  ~MockCdmServiceClient() override = default;

  // media::CdmService::Client implementation.
  MOCK_METHOD0(EnsureSandboxed, void());

  std::unique_ptr<media::CdmFactory> CreateCdmFactory(
      service_manager::mojom::InterfaceProvider* host_interfaces) override {
    return std::make_unique<media::DefaultCdmFactory>();
  }

#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
  void AddCdmHostFilePaths(std::vector<media::CdmHostFilePath>*) override {}
#endif  // BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
};

class ServiceTestClient : public service_manager::test::ServiceTestClient,
                          public service_manager::mojom::ServiceFactory {
 public:
  explicit ServiceTestClient(service_manager::test::ServiceTest* test)
      : service_manager::test::ServiceTestClient(test) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::BindRepeating(&ServiceTestClient::Create,
                            base::Unretained(this)));
  }
  ~ServiceTestClient() override {}

  // service_manager::Service implementation.
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  // service_manager::mojom::ServiceFactory implementation.
  void CreateService(
      service_manager::mojom::ServiceRequest request,
      const std::string& name,
      service_manager::mojom::PIDReceiverPtr pid_receiver) override {
    if (name != mojom::kCdmServiceName)
      return;

    auto mock_cdm_service_client = std::make_unique<MockCdmServiceClient>();
    mock_cdm_service_client_ = mock_cdm_service_client.get();

    auto cdm_service =
        std::make_unique<CdmService>(std::move(mock_cdm_service_client));
    cdm_service_ = cdm_service.get();

    cdm_service_->SetServiceReleaseDelayForTesting(service_release_delay_);

    service_context_ = std::make_unique<service_manager::ServiceContext>(
        std::move(cdm_service), std::move(request));
    service_context_->SetQuitClosure(base::BindRepeating(
        &ServiceTestClient::DestroyService, base::Unretained(this)));
  }

  void SetServiceReleaseDelay(base::TimeDelta delay) {
    service_release_delay_ = delay;
  }

  void DestroyService() { service_context_.reset(); }

  MockCdmServiceClient* mock_cdm_service_client() {
    return mock_cdm_service_client_;
  }

  CdmService* cdm_service() { return cdm_service_; }

 private:
  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

  // Delayed service release involves a posted delayed task which will not
  // block *.RunUntilIdle() and hence cause a memory leak in the test. So by
  // default use a zero value delay to disable the delay.
  base::TimeDelta service_release_delay_;

  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  std::unique_ptr<service_manager::ServiceContext> service_context_;
  CdmService* cdm_service_ = nullptr;
  MockCdmServiceClient* mock_cdm_service_client_ = nullptr;
};

class CdmServiceTest : public service_manager::test::ServiceTest {
 public:
  CdmServiceTest() : ServiceTest("cdm_service_unittest") {}
  ~CdmServiceTest() override {}

  MOCK_METHOD0(CdmServiceConnectionClosed, void());
  MOCK_METHOD0(CdmFactoryConnectionClosed, void());
  MOCK_METHOD0(CdmConnectionClosed, void());

  void Initialize() {
    connector()->BindInterface(media::mojom::kCdmServiceName,
                               &cdm_service_ptr_);
    cdm_service_ptr_.set_connection_error_handler(base::BindRepeating(
        &CdmServiceTest::CdmServiceConnectionClosed, base::Unretained(this)));

    service_manager::mojom::InterfaceProviderPtr interfaces;
    auto provider = std::make_unique<MediaInterfaceProvider>(
        mojo::MakeRequest(&interfaces));

    ASSERT_FALSE(cdm_factory_ptr_);
    cdm_service_ptr_->CreateCdmFactory(mojo::MakeRequest(&cdm_factory_ptr_),
                                       std::move(interfaces));
    cdm_service_ptr_.FlushForTesting();
    ASSERT_TRUE(cdm_factory_ptr_);
    cdm_factory_ptr_.set_connection_error_handler(base::BindRepeating(
        &CdmServiceTest::CdmFactoryConnectionClosed, base::Unretained(this)));
  }

  void InitializeWithServiceReleaseDelay(base::TimeDelta delay) {
    service_test_client_->SetServiceReleaseDelay(delay);
    Initialize();
  }

  MOCK_METHOD3(OnCdmInitialized,
               void(mojom::CdmPromiseResultPtr result,
                    int cdm_id,
                    mojom::DecryptorPtr decryptor));

  void InitializeCdm(const std::string& key_system, bool expected_result) {
    base::RunLoop run_loop;
    cdm_factory_ptr_->CreateCdm(key_system, mojo::MakeRequest(&cdm_ptr_));
    cdm_ptr_.set_connection_error_handler(base::BindRepeating(
        &CdmServiceTest::CdmConnectionClosed, base::Unretained(this)));
    EXPECT_CALL(*this, OnCdmInitialized(MatchesResult(expected_result), _, _))
        .WillOnce(InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
    cdm_ptr_->Initialize(key_system, url::Origin::Create(GURL(kSecurityOrigin)),
                         CdmConfig(),
                         base::BindRepeating(&CdmServiceTest::OnCdmInitialized,
                                             base::Unretained(this)));
    run_loop.Run();
  }

  // service_manager::test::ServiceTest implementation.
  std::unique_ptr<service_manager::Service> CreateService() override {
    auto service_test_client = std::make_unique<ServiceTestClient>(this);
    service_test_client_ = service_test_client.get();
    return service_test_client;
  }

  mojom::CdmServicePtr cdm_service_ptr_;
  mojom::CdmFactoryPtr cdm_factory_ptr_;
  mojom::ContentDecryptionModulePtr cdm_ptr_;
  ServiceTestClient* service_test_client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CdmServiceTest);
};

}  // namespace

TEST_F(CdmServiceTest, LoadCdm) {
  Initialize();

  // Even with a dummy path where the CDM cannot be loaded, EnsureSandboxed()
  // should still be called to ensure the process is sandboxed.
  EXPECT_CALL(*service_test_client_->mock_cdm_service_client(),
              EnsureSandboxed());

  base::FilePath cdm_path(FILE_PATH_LITERAL("dummy path"));
#if defined(OS_MACOSX)
  // Token provider will not be used since the path is a dummy path.
  cdm_service_ptr_->LoadCdm(cdm_path, nullptr);
#else
  cdm_service_ptr_->LoadCdm(cdm_path);
#endif

  cdm_service_ptr_.FlushForTesting();
}

TEST_F(CdmServiceTest, InitializeCdm_Success) {
  Initialize();
  InitializeCdm(kClearKeyKeySystem, true);
}

TEST_F(CdmServiceTest, InitializeCdm_InvalidKeySystem) {
  Initialize();
  InitializeCdm(kInvalidKeySystem, false);
}

TEST_F(CdmServiceTest, DestroyAndRecreateCdm) {
  Initialize();
  InitializeCdm(kClearKeyKeySystem, true);
  cdm_ptr_.reset();
  InitializeCdm(kClearKeyKeySystem, true);
}

// CdmFactory connection error will NOT destroy CDMs. Instead, it will only be
// destroyed after |cdm_ptr_| is reset.
TEST_F(CdmServiceTest, DestroyCdmFactory) {
  Initialize();
  auto* service = service_test_client_->cdm_service();

  InitializeCdm(kClearKeyKeySystem, true);
  EXPECT_EQ(service->BoundCdmFactorySizeForTesting(), 1u);
  EXPECT_EQ(service->UnboundCdmFactorySizeForTesting(), 0u);

  cdm_factory_ptr_.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(service->BoundCdmFactorySizeForTesting(), 0u);
  EXPECT_EQ(service->UnboundCdmFactorySizeForTesting(), 1u);

  cdm_ptr_.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(service->BoundCdmFactorySizeForTesting(), 0u);
  EXPECT_EQ(service->UnboundCdmFactorySizeForTesting(), 0u);
}

// Same as DestroyCdmFactory test, but do not disable delayed service release.
// TODO(xhwang): Use ScopedTaskEnvironment::MainThreadType::MOCK_TIME and
// ScopedTaskEnvironment::FastForwardBy() so we don't have to really wait for
// the delay in the test. But currently FastForwardBy() doesn't support delayed
// task yet.
TEST_F(CdmServiceTest, DestroyCdmFactory_DelayedServiceRelease) {
  constexpr base::TimeDelta kServiceContextRefReleaseDelay =
      base::TimeDelta::FromSeconds(1);
  InitializeWithServiceReleaseDelay(kServiceContextRefReleaseDelay);

  InitializeCdm(kClearKeyKeySystem, true);
  cdm_factory_ptr_.reset();
  base::RunLoop().RunUntilIdle();

  base::RunLoop run_loop;
  auto start_time = base::Time::Now();
  cdm_ptr_.reset();
  EXPECT_CALL(*this, CdmServiceConnectionClosed())
      .WillOnce(Invoke(&run_loop, &base::RunLoop::Quit));
  run_loop.Run();
  auto time_passed = base::Time::Now() - start_time;
  EXPECT_GE(time_passed, kServiceContextRefReleaseDelay);
}

// Destroy service will destroy the CdmFactory and all CDMs.
TEST_F(CdmServiceTest, DestroyCdmService) {
  Initialize();
  InitializeCdm(kClearKeyKeySystem, true);

  base::RunLoop run_loop;
  // Ideally we should not care about order, and should only quit the loop when
  // both connections are closed.
  EXPECT_CALL(*this, CdmServiceConnectionClosed());
  EXPECT_CALL(*this, CdmFactoryConnectionClosed());
  EXPECT_CALL(*this, CdmConnectionClosed())
      .WillOnce(Invoke(&run_loop, &base::RunLoop::Quit));
  service_test_client_->DestroyService();
  run_loop.Run();
}

}  // namespace media
