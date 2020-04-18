// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/app_remoting_connection_helper.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "remoting/protocol/host_stub.h"
#include "remoting/test/app_remoting_test_driver_environment.h"
#include "remoting/test/remote_application_details.h"
#include "remoting/test/test_chromoting_client.h"

namespace {
const int kDefaultDPI = 96;
const int kDefaultWidth = 1024;
const int kDefaultHeight = 768;

const char kHostProcessWindowTitle[] = "Host Process";
}  // namespace

namespace remoting {
namespace test {

AppRemotingConnectionHelper::AppRemotingConnectionHelper(
    const RemoteApplicationDetails& application_details)
    : application_details_(application_details),
      connection_is_ready_for_tests_(false),
      timer_(new base::Timer(true, false)) {
}

AppRemotingConnectionHelper::~AppRemotingConnectionHelper() {
  // |client_| destroys some of its members via DeleteSoon on the message loop's
  // TaskRunner so we need to run the loop until it has no more work to do.
  if (!connection_is_ready_for_tests_) {
    client_->RemoveRemoteConnectionObserver(this);
  }
  client_.reset();

  base::RunLoop().RunUntilIdle();
}

void AppRemotingConnectionHelper::Initialize(
    std::unique_ptr<TestChromotingClient> test_chromoting_client) {
  client_ = std::move(test_chromoting_client);
  client_->AddRemoteConnectionObserver(this);
}

bool AppRemotingConnectionHelper::StartConnection() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(client_);

  RemoteHostInfo remote_host_info;
  remoting::test::AppRemotingSharedData->GetRemoteHostInfoForApplicationId(
      application_details_.application_id, &remote_host_info);

  if (!remote_host_info.IsReadyForConnection()) {
    return false;
  }
  remoting::test::AppRemotingSharedData->AddHostToReleaseList(
      application_details_.application_id, remote_host_info.host_id);

  DCHECK(!run_loop_ || !run_loop_->running());
  run_loop_.reset(new base::RunLoop());

  // We will wait up to 30 seconds to complete the remote connection and for the
  // main application window to become visible.
  DCHECK(!timer_->IsRunning());
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(30),
                run_loop_->QuitClosure());

  client_->StartConnection(
      /*use_test_api_settings=*/false,
      remote_host_info.GenerateConnectionSetupInfo(
          AppRemotingSharedData->access_token(),
          AppRemotingSharedData->user_name()));

  run_loop_->Run();
  timer_->Stop();

  if (connection_is_ready_for_tests_) {
    return true;
  } else {
    client_->EndConnection();
    return false;
  }
}

protocol::ClipboardStub* AppRemotingConnectionHelper::clipboard_forwarder() {
  return client_->clipboard_forwarder();
}

protocol::HostStub* AppRemotingConnectionHelper::host_stub() {
  return client_->host_stub();
}

protocol::InputStub* AppRemotingConnectionHelper::input_stub() {
  return client_->input_stub();
}

void AppRemotingConnectionHelper::ConnectionStateChanged(
    protocol::ConnectionToHost::State state,
    protocol::ErrorCode error_code) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // If the connection is closed or failed then mark the connection as closed
  // and quit the current RunLoop if it exists.
  if (state == protocol::ConnectionToHost::CLOSED ||
      state == protocol::ConnectionToHost::FAILED ||
      error_code != protocol::OK) {
    connection_is_ready_for_tests_ = false;

    if (run_loop_) {
      run_loop_->Quit();
    }
  }
}

void AppRemotingConnectionHelper::ConnectionReady(bool ready) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (ready) {
    SendClientConnectionDetailsToHost();
  } else {
    // We will only get called here with a false value for |ready| if the video
    // renderer encounters an error.
    connection_is_ready_for_tests_ = false;

    if (run_loop_) {
      run_loop_->Quit();
    }
  }
}

void AppRemotingConnectionHelper::HostMessageReceived(
    const protocol::ExtensionMessage& message) {
  DCHECK(thread_checker_.CalledOnValidThread());

  VLOG(2) << "HostMessage received by HostMessageReceived()."
          << " type: " << message.type() << " data: " << message.data();

  if (message.type() == "onWindowAdded") {
    HandleOnWindowAddedMessage(message);
  } else {
    VLOG(2) << "HostMessage not handled by HostMessageReceived().";
  }
}

void AppRemotingConnectionHelper::SendClientConnectionDetailsToHost() {
  // First send an access token which will be used for Google Drive access.
  protocol::ExtensionMessage message;
  message.set_type("accessToken");
  message.set_data(AppRemotingSharedData->access_token());

  VLOG(1) << "Sending access token to host";
  client_->host_stub()->DeliverClientMessage(message);

  // Next send the host a description of the client screen size.
  protocol::ClientResolution client_resolution;
  client_resolution.set_width_deprecated(kDefaultWidth);
  client_resolution.set_height_deprecated(kDefaultHeight);
  client_resolution.set_x_dpi(kDefaultDPI);
  client_resolution.set_y_dpi(kDefaultDPI);
  client_resolution.set_dips_width(kDefaultWidth);
  client_resolution.set_dips_height(kDefaultHeight);

  VLOG(1) << "Sending ClientResolution details to host";
  client_->host_stub()->NotifyClientResolution(client_resolution);

  // Finally send a message to start sending us video packets.
  protocol::VideoControl video_control;
  video_control.set_enable(true);

  VLOG(1) << "Sending enable VideoControl message to host";
  client_->host_stub()->ControlVideo(video_control);
}

void AppRemotingConnectionHelper::HandleOnWindowAddedMessage(
    const remoting::protocol::ExtensionMessage& message) {
  DCHECK_EQ(message.type(), "onWindowAdded");

  const base::DictionaryValue* message_data = nullptr;
  std::unique_ptr<base::Value> host_message =
      base::JSONReader::Read(message.data());
  if (!host_message.get() || !host_message->GetAsDictionary(&message_data)) {
    LOG(ERROR) << "onWindowAdded message received was not valid JSON.";
    if (run_loop_) {
      run_loop_->Quit();
    }
    return;
  }

  std::string current_window_title;
  message_data->GetString("title", &current_window_title);
  if (current_window_title == kHostProcessWindowTitle) {
    LOG(ERROR) << "Host Process Window is visible, this likely means that the "
               << "underlying application is in a bad state, YMMV.";
  }

  std::string main_window_title = application_details_.main_window_title;
  if (current_window_title.find_first_of(main_window_title) == 0) {
    connection_is_ready_for_tests_ = true;
    client_->RemoveRemoteConnectionObserver(this);

    if (timer_->IsRunning()) {
      timer_->Stop();
    }

    DCHECK(run_loop_);
    // Now that the main window is visible, give the app some time to settle
    // before signaling that it is ready to run tests.
    timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(2),
                  run_loop_->QuitClosure());
  }
}

}  // namespace test
}  // namespace remoting
