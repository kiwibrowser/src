// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_session_impl.h"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/location.h"
#include "base/posix/eintr_wrapper.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/login_manager/arc.pb.h"
#include "components/arc/arc_bridge_host_impl.h"
#include "components/arc/arc_features.h"
#include "components/user_manager/user_manager.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

namespace {

chromeos::SessionManagerClient* GetSessionManagerClient() {
  // If the DBusThreadManager or the SessionManagerClient aren't available,
  // there isn't much we can do. This should only happen when running tests.
  if (!chromeos::DBusThreadManager::IsInitialized() ||
      !chromeos::DBusThreadManager::Get() ||
      !chromeos::DBusThreadManager::Get()->GetSessionManagerClient()) {
    return nullptr;
  }
  return chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
}

// Creates a pipe. Returns true on success, otherwise false.
// On success, |read_fd| will be set to the fd of the read side, and
// |write_fd| will be set to the one of write side.
bool CreatePipe(base::ScopedFD* read_fd, base::ScopedFD* write_fd) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) < 0) {
    PLOG(ERROR) << "pipe2()";
    return false;
  }

  read_fd->reset(fds[0]);
  write_fd->reset(fds[1]);
  return true;
}

// Waits until |raw_socket_fd| is readable.
// The operation may be cancelled originally triggered by user interaction to
// disable ARC, or ARC instance is unexpectedly stopped (e.g. crash).
// To notify such a situation, |raw_cancel_fd| is also passed to here, and the
// write side will be closed in such a case.
bool WaitForSocketReadable(int raw_socket_fd, int raw_cancel_fd) {
  struct pollfd fds[2] = {
      {raw_socket_fd, POLLIN, 0}, {raw_cancel_fd, POLLIN, 0},
  };

  if (HANDLE_EINTR(poll(fds, arraysize(fds), -1)) <= 0) {
    PLOG(ERROR) << "poll()";
    return false;
  }

  if (fds[1].revents) {
    // Notified that Stop() is invoked. Cancel the Mojo connecting.
    VLOG(1) << "Stop() was called during ConnectMojo()";
    return false;
  }

  DCHECK(fds[0].revents);
  return true;
}

// Returns the ArcStopReason corresponding to the ARC instance staring failure.
ArcStopReason GetArcStopReason(bool low_disk_space, bool stop_requested) {
  if (stop_requested)
    return ArcStopReason::SHUTDOWN;

  if (low_disk_space)
    return ArcStopReason::LOW_DISK_SPACE;

  return ArcStopReason::GENERIC_BOOT_FAILURE;
}

// Real Delegate implementation to connect Mojo.
class ArcSessionDelegateImpl : public ArcSessionImpl::Delegate {
 public:
  explicit ArcSessionDelegateImpl(ArcBridgeService* arc_bridge_service);
  ~ArcSessionDelegateImpl() override = default;

  // ArcSessionImpl::Delegate override.
  base::ScopedFD ConnectMojo(base::ScopedFD socket_fd,
                             ConnectMojoCallback callback) override;

 private:
  // Synchronously accepts a connection on |socket_fd| and then processes the
  // connected socket's file descriptor. This is designed to run on a
  // blocking thread.
  static mojo::ScopedMessagePipeHandle ConnectMojoInternal(
      mojo::edk::ScopedInternalPlatformHandle socket_fd,
      base::ScopedFD cancel_fd);

  // Called when Mojo connection is established or canceled.
  // In case of cancel or error, |server_pipe| is invalid.
  void OnMojoConnected(ConnectMojoCallback callback,
                       mojo::ScopedMessagePipeHandle server_pipe);

  // Owned by ArcServiceManager.
  ArcBridgeService* const arc_bridge_service_;

  // WeakPtrFactory to use callbacks.
  base::WeakPtrFactory<ArcSessionDelegateImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcSessionDelegateImpl);
};

ArcSessionDelegateImpl::ArcSessionDelegateImpl(
    ArcBridgeService* arc_bridge_service)
    : arc_bridge_service_(arc_bridge_service), weak_factory_(this) {}

base::ScopedFD ArcSessionDelegateImpl::ConnectMojo(
    base::ScopedFD socket_fd,
    ConnectMojoCallback callback) {
  // Prepare a pipe so that AcceptInstanceConnection can be interrupted on
  // Stop().
  base::ScopedFD cancel_fd;
  base::ScopedFD return_fd;
  if (!CreatePipe(&cancel_fd, &return_fd)) {
    LOG(ERROR) << "Failed to create a pipe to cancel accept()";
    return base::ScopedFD();
  }

  // For production, |socket_fd| passed from session_manager is either a valid
  // socket or a valid file descriptor (/dev/null). For testing, |socket_fd|
  // might be invalid.
  mojo::edk::InternalPlatformHandle raw_handle(socket_fd.release());
  raw_handle.needs_connection = true;

  mojo::edk::ScopedInternalPlatformHandle mojo_socket_fd(raw_handle);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ArcSessionDelegateImpl::ConnectMojoInternal,
                     std::move(mojo_socket_fd), std::move(cancel_fd)),
      base::BindOnce(&ArcSessionDelegateImpl::OnMojoConnected,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
  return return_fd;
}

// static
mojo::ScopedMessagePipeHandle ArcSessionDelegateImpl::ConnectMojoInternal(
    mojo::edk::ScopedInternalPlatformHandle socket_fd,
    base::ScopedFD cancel_fd) {
  if (!WaitForSocketReadable(socket_fd.get().handle, cancel_fd.get())) {
    VLOG(1) << "Mojo connection was cancelled.";
    return mojo::ScopedMessagePipeHandle();
  }

  mojo::edk::ScopedInternalPlatformHandle scoped_fd;
  if (!mojo::edk::ServerAcceptConnection(socket_fd, &scoped_fd,
                                         /* check_peer_user = */ false) ||
      !scoped_fd.is_valid()) {
    return mojo::ScopedMessagePipeHandle();
  }

  // Hardcode pid 0 since it is unused in mojo.
  const base::ProcessHandle kUnusedChildProcessHandle = 0;
  mojo::edk::PlatformChannelPair channel_pair;
  mojo::edk::OutgoingBrokerClientInvitation invitation;

  std::string token = mojo::edk::GenerateRandomToken();
  mojo::ScopedMessagePipeHandle pipe = invitation.AttachMessagePipe(token);

  invitation.Send(
      kUnusedChildProcessHandle,
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  channel_pair.PassServerHandle()));

  std::vector<mojo::edk::ScopedInternalPlatformHandle> handles;
  handles.emplace_back(channel_pair.PassClientHandle());

  // We need to send the length of the message as a single byte, so make sure it
  // fits.
  DCHECK_LT(token.size(), 256u);
  uint8_t message_length = static_cast<uint8_t>(token.size());
  struct iovec iov[] = {{&message_length, sizeof(message_length)},
                        {const_cast<char*>(token.c_str()), token.size()}};
  ssize_t result = mojo::edk::PlatformChannelSendmsgWithHandles(
      scoped_fd, iov, sizeof(iov) / sizeof(iov[0]), handles);
  if (result == -1) {
    PLOG(ERROR) << "sendmsg";
    return mojo::ScopedMessagePipeHandle();
  }

  return pipe;
}

void ArcSessionDelegateImpl::OnMojoConnected(
    ConnectMojoCallback callback,
    mojo::ScopedMessagePipeHandle server_pipe) {
  if (!server_pipe.is_valid()) {
    LOG(ERROR) << "Invalid pipe";
    std::move(callback).Run(nullptr);
    return;
  }

  mojom::ArcBridgeInstancePtr instance;
  instance.Bind(mojo::InterfacePtrInfo<mojom::ArcBridgeInstance>(
      std::move(server_pipe), 0u));
  std::move(callback).Run(std::make_unique<ArcBridgeHostImpl>(
      arc_bridge_service_, std::move(instance)));
}

}  // namespace

const char ArcSessionImpl::kPackagesCacheModeCopy[] = "copy";
const char ArcSessionImpl::kPackagesCacheModeSkipCopy[] = "skip-copy";

// static
std::unique_ptr<ArcSessionImpl::Delegate> ArcSessionImpl::CreateDelegate(
    ArcBridgeService* arc_bridge_service) {
  return std::make_unique<ArcSessionDelegateImpl>(arc_bridge_service);
}

ArcSessionImpl::ArcSessionImpl(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)), weak_factory_(this) {
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client == nullptr)
    return;
  client->AddObserver(this);
}

ArcSessionImpl::~ArcSessionImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(state_ == State::NOT_STARTED || state_ == State::STOPPED);
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client == nullptr)
    return;
  client->RemoveObserver(this);
}

void ArcSessionImpl::StartMiniInstance() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::NOT_STARTED);

  state_ = State::STARTING_MINI_INSTANCE;
  VLOG(2) << "Starting ARC mini instance";

  login_manager::StartArcMiniContainerRequest request;
  request.set_native_bridge_experiment(
      base::FeatureList::IsEnabled(arc::kNativeBridgeExperimentFeature));

  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  client->StartArcMiniContainer(
      request, base::BindOnce(&ArcSessionImpl::OnMiniInstanceStarted,
                              weak_factory_.GetWeakPtr()));
}

void ArcSessionImpl::RequestUpgrade() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  upgrade_requested_ = true;

  switch (state_) {
    case State::NOT_STARTED:
      NOTREACHED();
      break;
    case State::STARTING_MINI_INSTANCE:
      VLOG(2) << "Requested to upgrade a starting ARC mini instance";
      // OnMiniInstanceStarted() will restart a full instance.
      break;
    case State::RUNNING_MINI_INSTANCE:
      DoUpgrade();
      break;
    case State::STARTING_FULL_INSTANCE:
    case State::CONNECTING_MOJO:
    case State::RUNNING_FULL_INSTANCE:
    case State::STOPPED:
      // These mean RequestUpgrade() is called twice or called after
      // stopped, which are invalid operations.
      NOTREACHED();
      break;
  }
}

void ArcSessionImpl::OnMiniInstanceStarted(
    base::Optional<std::string> container_instance_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::STARTING_MINI_INSTANCE);

  if (!container_instance_id) {
    OnStopped(GetArcStopReason(false, stop_requested_));
    return;
  }

  container_instance_id_ = std::move(*container_instance_id);
  VLOG(2) << "ARC mini instance is successfully started: "
          << container_instance_id_;

  if (stop_requested_) {
    // The ARC instance has started to run. Request to stop.
    StopArcInstance();
    return;
  }

  state_ = State::RUNNING_MINI_INSTANCE;

  if (upgrade_requested_)
    // RequestUpgrade() has been called during the D-Bus call.
    DoUpgrade();
}

void ArcSessionImpl::DoUpgrade() {
  DCHECK_EQ(state_, State::RUNNING_MINI_INSTANCE);

  VLOG(2) << "Upgrading an existing ARC mini instance";
  state_ = State::STARTING_FULL_INSTANCE;

  login_manager::UpgradeArcContainerRequest request;
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  DCHECK(user_manager->GetPrimaryUser());

  request.set_account_id(
      cryptohome::Identification(user_manager->GetPrimaryUser()->GetAccountId())
          .id());
  request.set_skip_boot_completed_broadcast(
      !base::FeatureList::IsEnabled(arc::kBootCompletedBroadcastFeature));

  // We only enable /vendor/priv-app when voice interaction is enabled
  // because voice interaction service apk would be bundled in this
  // location.
  request.set_scan_vendor_priv_app(
      chromeos::switches::IsVoiceInteractionEnabled());

  // Set packages cache mode coming from autotests.
  const std::string packages_cache_mode_string =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          chromeos::switches::kArcPackagesCacheMode);
  if (packages_cache_mode_string == kPackagesCacheModeSkipCopy) {
    request.set_packages_cache_mode(
        login_manager::
            UpgradeArcContainerRequest_PackageCacheMode_SKIP_SETUP_COPY_ON_INIT);
  } else if (packages_cache_mode_string == kPackagesCacheModeCopy) {
    request.set_packages_cache_mode(
        login_manager::
            UpgradeArcContainerRequest_PackageCacheMode_COPY_ON_INIT);
  } else if (!packages_cache_mode_string.empty()) {
    VLOG(2) << "Invalid packages cache mode switch "
            << packages_cache_mode_string << ".";
  }

  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  client->UpgradeArcContainer(
      request,
      base::BindOnce(&ArcSessionImpl::OnUpgraded, weak_factory_.GetWeakPtr()),
      base::BindOnce(&ArcSessionImpl::OnUpgradeError,
                     weak_factory_.GetWeakPtr()));
}

void ArcSessionImpl::OnUpgraded(base::ScopedFD socket_fd) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::STARTING_FULL_INSTANCE);

  VLOG(2) << "ARC instance is successfully upgraded.";

  if (stop_requested_) {
    // The ARC instance has started to run. Request to stop.
    StopArcInstance();
    return;
  }

  VLOG(2) << "Connecting mojo...";
  state_ = State::CONNECTING_MOJO;
  accept_cancel_pipe_ = delegate_->ConnectMojo(
      std::move(socket_fd), base::BindOnce(&ArcSessionImpl::OnMojoConnected,
                                           weak_factory_.GetWeakPtr()));
  if (!accept_cancel_pipe_.is_valid()) {
    // Failed to post a task to accept() the request.
    StopArcInstance();
    return;
  }
}

void ArcSessionImpl::OnUpgradeError(bool low_disk_space) {
  OnStopped(GetArcStopReason(low_disk_space, stop_requested_));
}

void ArcSessionImpl::OnMojoConnected(
    std::unique_ptr<mojom::ArcBridgeHost> arc_bridge_host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::CONNECTING_MOJO);
  accept_cancel_pipe_.reset();

  if (stop_requested_) {
    StopArcInstance();
    return;
  }

  if (!arc_bridge_host.get()) {
    LOG(ERROR) << "Invalid pipe.";
    StopArcInstance();
    return;
  }
  arc_bridge_host_ = std::move(arc_bridge_host);

  VLOG(0) << "ARC ready.";
  state_ = State::RUNNING_FULL_INSTANCE;
}

void ArcSessionImpl::Stop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(2) << "Stopping ARC session is requested.";

  // For second time or later, just do nothing.
  // It is already in the stopping phase.
  if (stop_requested_)
    return;

  stop_requested_ = true;
  arc_bridge_host_.reset();
  switch (state_) {
    case State::NOT_STARTED:
      OnStopped(ArcStopReason::SHUTDOWN);
      return;
    case State::STARTING_MINI_INSTANCE:
    case State::STARTING_FULL_INSTANCE:
      // Before starting the ARC instance, we do nothing here.
      // At some point, a callback will be invoked on UI thread,
      // and stopping procedure will be run there.
      // On Chrome shutdown, it is not the case because the message loop is
      // already stopped here. Practically, it is not a problem because;
      // - On starting instance, the container instance can be leaked.
      // Practically it is not problematic because the session manager will
      // clean it up.
      return;

    case State::RUNNING_MINI_INSTANCE:
    case State::RUNNING_FULL_INSTANCE:
      // An ARC {mini,full} instance is running. Request to stop it.
      StopArcInstance();
      return;

    case State::CONNECTING_MOJO:
      // Mojo connection is being waited on TaskScheduler's thread.
      // Request to cancel it. Following stopping procedure will run
      // in its callback.
      accept_cancel_pipe_.reset();
      return;

    case State::STOPPED:
      // The instance is already stopped. Do nothing.
      return;
  }
}

void ArcSessionImpl::StopArcInstance() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(state_ == State::STARTING_MINI_INSTANCE ||
         state_ == State::RUNNING_MINI_INSTANCE ||
         state_ == State::STARTING_FULL_INSTANCE ||
         state_ == State::CONNECTING_MOJO ||
         state_ == State::RUNNING_FULL_INSTANCE);

  VLOG(2) << "Requesting session_manager to stop ARC instance";

  // When the instance is full instance, change the |state_| in
  // ArcInstanceStopped().
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  // Since we have the ArcInstanceStopped() callback, we don't need to do
  // anything when StopArcInstance completes.
  client->StopArcInstance(chromeos::EmptyVoidDBusMethodCallback());
}

void ArcSessionImpl::ArcInstanceStopped(
    login_manager::ArcContainerStopReason stop_reason,
    const std::string& container_instance_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(1) << "Notified that ARC instance is stopped "
          << static_cast<uint32_t>(stop_reason);

  if (container_instance_id != container_instance_id_) {
    VLOG(1) << "Container instance id mismatch. Do nothing."
            << container_instance_id << " vs " << container_instance_id_;
    return;
  }

  // Release |container_instance_id_| to avoid duplicate invocation situation.
  container_instance_id_.clear();

  // In case that crash happens during before the Mojo channel is connected,
  // unlock the TaskScheduler's thread.
  accept_cancel_pipe_.reset();

  // TODO(hidehiko): In new D-Bus signal, more detailed reason why ARC
  // container is stopped. Check it in details.
  ArcStopReason reason;
  if (stop_requested_) {
    // If the ARC instance is stopped after its explicit request,
    // return SHUTDOWN.
    reason = ArcStopReason::SHUTDOWN;
  } else if (stop_reason ==
             login_manager::ArcContainerStopReason::LOW_DISK_SPACE) {
    // ARC mini container is stopped because of upgarde failure due to low
    // disk space.
    reason = ArcStopReason::LOW_DISK_SPACE;
  } else if (stop_reason != login_manager::ArcContainerStopReason::CRASH) {
    // If the ARC instance is stopped, but it is not explicitly requested,
    // then this is triggered by some failure during the starting procedure.
    // Return GENERIC_BOOT_FAILURE for the case.
    reason = ArcStopReason::GENERIC_BOOT_FAILURE;
  } else {
    // Otherwise, this is caused by CRASH occured inside of the ARC instance.
    reason = ArcStopReason::CRASH;
  }
  OnStopped(reason);
}

bool ArcSessionImpl::IsStopRequested() {
  return stop_requested_;
}

void ArcSessionImpl::OnStopped(ArcStopReason reason) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // OnStopped() should be called once per instance.
  DCHECK_NE(state_, State::STOPPED);
  VLOG(2) << "ARC session is stopped.";
  const bool was_running = (state_ == State::RUNNING_FULL_INSTANCE);
  arc_bridge_host_.reset();
  state_ = State::STOPPED;
  for (auto& observer : observer_list_)
    observer.OnSessionStopped(reason, was_running, upgrade_requested_);
}

void ArcSessionImpl::OnShutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  stop_requested_ = true;
  if (state_ == State::STOPPED)
    return;

  // Here, the message loop is already stopped, and the Chrome will be soon
  // shutdown. Thus, it is not necessary to take care about restarting case.
  // If ArcSession is waiting for mojo connection, cancels it.
  accept_cancel_pipe_.reset();

  // Stops the ARC instance to let it graceful shutdown.
  // Note that this may fail if ARC container is not actually running, but
  // ignore an error as described below.
  if (state_ == State::STARTING_MINI_INSTANCE ||
      state_ == State::RUNNING_MINI_INSTANCE ||
      state_ == State::STARTING_FULL_INSTANCE ||
      state_ == State::CONNECTING_MOJO ||
      state_ == State::RUNNING_FULL_INSTANCE) {
    StopArcInstance();
  }

  // Directly set to the STOPPED state by OnStopped(). Note that calling
  // StopArcInstance() may not work well. At least, because the UI thread is
  // already stopped here, ArcInstanceStopped() callback cannot be invoked.
  OnStopped(ArcStopReason::SHUTDOWN);
}

std::ostream& operator<<(std::ostream& os, ArcSessionImpl::State state) {
#define MAP_STATE(name)             \
  case ArcSessionImpl::State::name: \
    return os << #name

  switch (state) {
    MAP_STATE(NOT_STARTED);
    MAP_STATE(STARTING_MINI_INSTANCE);
    MAP_STATE(RUNNING_MINI_INSTANCE);
    MAP_STATE(STARTING_FULL_INSTANCE);
    MAP_STATE(CONNECTING_MOJO);
    MAP_STATE(RUNNING_FULL_INSTANCE);
    MAP_STATE(STOPPED);
  }
#undef MAP_STATE

  // Some compilers report an error even if all values of an enum-class are
  // covered exhaustively in a switch statement.
  NOTREACHED() << "Invalid value " << static_cast<int>(state);
  return os;
}

}  // namespace arc
