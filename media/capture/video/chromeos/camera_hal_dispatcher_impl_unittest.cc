// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/camera_hal_dispatcher_impl.h"

#include <memory>
#include <utility>

#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "media/capture/video/chromeos/mojo/cros_camera_service.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::InvokeWithoutArgs;

namespace media {
namespace {

class MockCameraHalServer : public cros::mojom::CameraHalServer {
 public:
  MockCameraHalServer() : binding_(this) {}

  ~MockCameraHalServer() = default;

  void CreateChannel(
      cros::mojom::CameraModuleRequest camera_module_request) override {
    DoCreateChannel(camera_module_request);
  }
  MOCK_METHOD1(DoCreateChannel,
               void(cros::mojom::CameraModuleRequest& camera_module_request));

  cros::mojom::CameraHalServerPtrInfo GetInterfacePtrInfo() {
    cros::mojom::CameraHalServerPtrInfo camera_hal_server_ptr_info;
    cros::mojom::CameraHalServerRequest camera_hal_server_request =
        mojo::MakeRequest(&camera_hal_server_ptr_info);
    binding_.Bind(std::move(camera_hal_server_request));
    return camera_hal_server_ptr_info;
  }

 private:
  mojo::Binding<cros::mojom::CameraHalServer> binding_;
  DISALLOW_COPY_AND_ASSIGN(MockCameraHalServer);
};

class MockCameraHalClient : public cros::mojom::CameraHalClient {
 public:
  MockCameraHalClient() : binding_(this) {}

  ~MockCameraHalClient() = default;

  void SetUpChannel(cros::mojom::CameraModulePtr camera_module_ptr) override {
    DoSetUpChannel(camera_module_ptr);
  }
  MOCK_METHOD1(DoSetUpChannel,
               void(cros::mojom::CameraModulePtr& camera_module_ptr));

  cros::mojom::CameraHalClientPtrInfo GetInterfacePtrInfo() {
    cros::mojom::CameraHalClientPtrInfo camera_hal_client_ptr_info;
    cros::mojom::CameraHalClientRequest camera_hal_client_request =
        mojo::MakeRequest(&camera_hal_client_ptr_info);
    binding_.Bind(std::move(camera_hal_client_request));
    return camera_hal_client_ptr_info;
  }

 private:
  mojo::Binding<cros::mojom::CameraHalClient> binding_;
  DISALLOW_COPY_AND_ASSIGN(MockCameraHalClient);
};

}  // namespace

class CameraHalDispatcherImplTest : public ::testing::Test {
 public:
  CameraHalDispatcherImplTest() = default;

  ~CameraHalDispatcherImplTest() override = default;

  void SetUp() override {
    dispatcher_ = new CameraHalDispatcherImpl();
    EXPECT_TRUE(dispatcher_->StartThreads());
  }

  void TearDown() override { delete dispatcher_; }

  scoped_refptr<base::SingleThreadTaskRunner> GetProxyTaskRunner() {
    return dispatcher_->proxy_task_runner_;
  }

  void DoLoop() {
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  void QuitRunLoop() {
    if (run_loop_) {
      run_loop_->Quit();
    }
  }

  static void RegisterServer(CameraHalDispatcherImpl* dispatcher,
                             cros::mojom::CameraHalServerPtrInfo server) {
    dispatcher->RegisterServer(mojo::MakeProxy(std::move(server)));
  }

  static void RegisterClient(CameraHalDispatcherImpl* dispatcher,
                             cros::mojom::CameraHalClientPtrInfo client) {
    dispatcher->RegisterClient(mojo::MakeProxy(std::move(client)));
  }

 protected:
  // We can't use std::unique_ptr here because the constructor and destructor of
  // CameraHalDispatcherImpl are private.
  CameraHalDispatcherImpl* dispatcher_;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<base::RunLoop> run_loop_;
  DISALLOW_COPY_AND_ASSIGN(CameraHalDispatcherImplTest);
};

// Test that the CameraHalDisptcherImpl correctly re-establishes a Mojo channel
// for the client when the server crashes.
TEST_F(CameraHalDispatcherImplTest, ServerConnectionError) {
  // First verify that a the CameraHalDispatcherImpl establishes a Mojo channel
  // between the server and the client.
  auto mock_server = std::make_unique<MockCameraHalServer>();
  auto mock_client = std::make_unique<MockCameraHalClient>();

  EXPECT_CALL(*mock_server, DoCreateChannel(_)).Times(1);
  EXPECT_CALL(*mock_client, DoSetUpChannel(_))
      .Times(1)
      .WillOnce(
          InvokeWithoutArgs(this, &CameraHalDispatcherImplTest::QuitRunLoop));

  auto server_ptr = mock_server->GetInterfacePtrInfo();
  GetProxyTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImplTest::RegisterServer,
                     base::Unretained(dispatcher_), base::Passed(&server_ptr)));
  auto client_ptr = mock_client->GetInterfacePtrInfo();
  GetProxyTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImplTest::RegisterClient,
                     base::Unretained(dispatcher_), base::Passed(&client_ptr)));

  // Wait until the client gets the established Mojo channel.
  DoLoop();

  // Re-create a new server to simulate a server crash.
  mock_server = std::make_unique<MockCameraHalServer>();

  // Make sure we creates a new Mojo channel from the new server to the same
  // client.
  EXPECT_CALL(*mock_server, DoCreateChannel(_)).Times(1);
  EXPECT_CALL(*mock_client, DoSetUpChannel(_))
      .Times(1)
      .WillOnce(
          InvokeWithoutArgs(this, &CameraHalDispatcherImplTest::QuitRunLoop));

  server_ptr = mock_server->GetInterfacePtrInfo();
  GetProxyTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImplTest::RegisterServer,
                     base::Unretained(dispatcher_), base::Passed(&server_ptr)));

  // Wait until the clients gets the newly established Mojo channel.
  DoLoop();
};

// Test that the CameraHalDisptcherImpl correctly re-establishes a Mojo channel
// for the client when the client reconnects after crash.
TEST_F(CameraHalDispatcherImplTest, ClientConnectionError) {
  // First verify that a the CameraHalDispatcherImpl establishes a Mojo channel
  // between the server and the client.
  auto mock_server = std::make_unique<MockCameraHalServer>();
  auto mock_client = std::make_unique<MockCameraHalClient>();

  EXPECT_CALL(*mock_server, DoCreateChannel(_)).Times(1);
  EXPECT_CALL(*mock_client, DoSetUpChannel(_))
      .Times(1)
      .WillOnce(
          InvokeWithoutArgs(this, &CameraHalDispatcherImplTest::QuitRunLoop));

  auto server_ptr = mock_server->GetInterfacePtrInfo();
  GetProxyTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImplTest::RegisterServer,
                     base::Unretained(dispatcher_), base::Passed(&server_ptr)));
  auto client_ptr = mock_client->GetInterfacePtrInfo();
  GetProxyTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImplTest::RegisterClient,
                     base::Unretained(dispatcher_), base::Passed(&client_ptr)));

  // Wait until the client gets the established Mojo channel.
  DoLoop();

  // Re-create a new server to simulate a server crash.
  mock_client = std::make_unique<MockCameraHalClient>();

  // Make sure we re-create the Mojo channel from the same server to the new
  // client.
  EXPECT_CALL(*mock_server, DoCreateChannel(_)).Times(1);
  EXPECT_CALL(*mock_client, DoSetUpChannel(_))
      .Times(1)
      .WillOnce(
          InvokeWithoutArgs(this, &CameraHalDispatcherImplTest::QuitRunLoop));

  client_ptr = mock_client->GetInterfacePtrInfo();
  GetProxyTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImplTest::RegisterClient,
                     base::Unretained(dispatcher_), base::Passed(&client_ptr)));

  // Wait until the clients gets the newly established Mojo channel.
  DoLoop();
};

}  // namespace media
