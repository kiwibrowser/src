// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/chromoting_test_fixture.h"

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/timer/timer.h"
#include "remoting/test/chromoting_test_driver_environment.h"
#include "remoting/test/connection_setup_info.h"
#include "remoting/test/connection_time_observer.h"
#include "remoting/test/host_info.h"
#include "remoting/test/test_chromoting_client.h"

namespace remoting {
namespace test {

ChromotingTestFixture::ChromotingTestFixture()
    : test_chromoting_client_(new TestChromotingClient()) {
}

ChromotingTestFixture::~ChromotingTestFixture() = default;

void ChromotingTestFixture::Disconnect() {
  test_chromoting_client_->EndConnection();
}

void ChromotingTestFixture::CreateObserver() {
  if (connection_time_observer_) {
    DestroyObserver();
  }

  connection_time_observer_.reset(new ConnectionTimeObserver());
  test_chromoting_client_->AddRemoteConnectionObserver(
      connection_time_observer_.get());
}

void ChromotingTestFixture::DestroyObserver() {
  if (!connection_time_observer_) {
    return;
  }

  test_chromoting_client_->RemoveRemoteConnectionObserver(
      connection_time_observer_.get());
  connection_time_observer_.reset();
}

bool ChromotingTestFixture::ConnectToHost(
    const base::TimeDelta& max_time_to_connect) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!g_chromoting_shared_data->host_info().IsReadyForConnection()) {
    LOG(ERROR) << "Can't connect to " << g_chromoting_shared_data->host_name();
    return false;
  }

  // Host is online and ready, initiate a remote session.
  base::RunLoop run_loop;

  CreateObserver();

  base::Timer timer(true, false);
  timer.Start(FROM_HERE, max_time_to_connect, run_loop.QuitClosure());

  connection_time_observer_->ConnectionStateChanged(
      protocol::ConnectionToHost::State::INITIALIZING,
      protocol::ErrorCode::OK);
  test_chromoting_client_->StartConnection(
      g_chromoting_shared_data->use_test_environment(),
      g_chromoting_shared_data->host_info().GenerateConnectionSetupInfo(
          g_chromoting_shared_data->access_token(),
          g_chromoting_shared_data->user_name(),
          g_chromoting_shared_data->pin()));
  run_loop.Run();

  // Note: Under different network conditions, connection time will have
  // greater variance. |max_time_to_connect| may need to be larger in high
  // latency network environments.
  if (connection_time_observer_->GetStateTransitionTime(
      protocol::ConnectionToHost::State::INITIALIZING,
      protocol::ConnectionToHost::State::CONNECTED).is_max()) {
    LOG(ERROR) << "Chromoting client did not connect to requested host";
    return false;
  }

  return true;
}

void ChromotingTestFixture::TearDown() {
  // If a chromoting connection is still connected, we want to end the
  // connection before starting the next test.
  Disconnect();
  DestroyObserver();
}

}  // namespace test
}  // namespace remoting
