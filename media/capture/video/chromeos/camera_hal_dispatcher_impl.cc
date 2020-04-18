// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/camera_hal_dispatcher_impl.h"

#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <sys/uio.h>

#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/posix/eintr_wrapper.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace media {

namespace {

const base::FilePath::CharType kArcCamera3SocketPath[] =
    "/var/run/camera/camera3.sock";
const char kArcCameraGroup[] = "arc-camera";

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

// Waits until |raw_socket_fd| is readable.  We signal |raw_cancel_fd| when we
// want to cancel the blocking wait and stop serving connections on
// |raw_socket_fd|.  To notify such a situation, |raw_cancel_fd| is also passed
// to here, and the write side will be closed in such a case.
bool WaitForSocketReadable(int raw_socket_fd, int raw_cancel_fd) {
  struct pollfd fds[2] = {
      {raw_socket_fd, POLLIN, 0}, {raw_cancel_fd, POLLIN, 0},
  };

  if (HANDLE_EINTR(poll(fds, arraysize(fds), -1)) <= 0) {
    PLOG(ERROR) << "poll()";
    return false;
  }

  if (fds[1].revents) {
    VLOG(1) << "Stop() was called";
    return false;
  }

  DCHECK(fds[0].revents);
  return true;
}

class MojoCameraClientObserver : public CameraClientObserver {
 public:
  explicit MojoCameraClientObserver(cros::mojom::CameraHalClientPtr client)
      : client_(std::move(client)) {}

  void OnChannelCreated(cros::mojom::CameraModulePtr camera_module) override {
    client_->SetUpChannel(std::move(camera_module));
  }

  cros::mojom::CameraHalClientPtr& client() { return client_; }

 private:
  cros::mojom::CameraHalClientPtr client_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(MojoCameraClientObserver);
};

}  // namespace

CameraClientObserver::~CameraClientObserver() = default;

// static
CameraHalDispatcherImpl* CameraHalDispatcherImpl::GetInstance() {
  return base::Singleton<CameraHalDispatcherImpl>::get();
}

bool CameraHalDispatcherImpl::StartThreads() {
  DCHECK(!proxy_thread_.IsRunning());
  DCHECK(!blocking_io_thread_.IsRunning());

  if (!proxy_thread_.Start()) {
    LOG(ERROR) << "Failed to start proxy thread";
    return false;
  }
  if (!blocking_io_thread_.Start()) {
    LOG(ERROR) << "Failed to start blocking IO thread";
    proxy_thread_.Stop();
    return false;
  }
  proxy_task_runner_ = proxy_thread_.task_runner();
  blocking_io_task_runner_ = blocking_io_thread_.task_runner();
  return true;
}

bool CameraHalDispatcherImpl::Start(
    MojoJpegDecodeAcceleratorFactoryCB jda_factory,
    MojoJpegEncodeAcceleratorFactoryCB jea_factory) {
  if (!StartThreads()) {
    return false;
  }
  jda_factory_ = std::move(jda_factory);
  jea_factory_ = std::move(jea_factory);
  base::WaitableEvent started(base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
  blocking_io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImpl::CreateSocket,
                     base::Unretained(this), base::Unretained(&started)));
  started.Wait();
  return IsStarted();
}

void CameraHalDispatcherImpl::AddClientObserver(
    std::unique_ptr<CameraClientObserver> observer) {
  // If |proxy_thread_| fails to start in Start() then CameraHalDelegate will
  // not be created, and this function will not be called.
  DCHECK(proxy_thread_.IsRunning());
  proxy_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImpl::AddClientObserverOnProxyThread,
                     base::Unretained(this), base::Passed(&observer)));
}

bool CameraHalDispatcherImpl::IsStarted() {
  return proxy_thread_.IsRunning() && blocking_io_thread_.IsRunning() &&
         proxy_fd_.is_valid();
}

CameraHalDispatcherImpl::CameraHalDispatcherImpl()
    : proxy_thread_("CameraProxyThread"),
      blocking_io_thread_("CameraBlockingIOThread") {}

CameraHalDispatcherImpl::~CameraHalDispatcherImpl() {
  VLOG(1) << "Stopping CameraHalDispatcherImpl...";
  if (proxy_thread_.IsRunning()) {
    proxy_thread_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&CameraHalDispatcherImpl::StopOnProxyThread,
                                  base::Unretained(this)));
    proxy_thread_.Stop();
  }
  blocking_io_thread_.Stop();
  VLOG(1) << "CameraHalDispatcherImpl stopped";
}

void CameraHalDispatcherImpl::RegisterServer(
    cros::mojom::CameraHalServerPtr camera_hal_server) {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());

  if (camera_hal_server_) {
    LOG(ERROR) << "Camera HAL server is already registered";
    return;
  }
  camera_hal_server.set_connection_error_handler(
      base::BindOnce(&CameraHalDispatcherImpl::OnCameraHalServerConnectionError,
                     base::Unretained(this)));
  camera_hal_server_ = std::move(camera_hal_server);
  VLOG(1) << "Camera HAL server registered";

  // Set up the Mojo channels for clients which registered before the server
  // registers.
  for (auto& client_observer : client_observers_) {
    EstablishMojoChannel(client_observer.get());
  }
}

void CameraHalDispatcherImpl::RegisterClient(
    cros::mojom::CameraHalClientPtr client) {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());
  auto client_observer =
      std::make_unique<MojoCameraClientObserver>(std::move(client));
  client_observer->client().set_connection_error_handler(base::BindOnce(
      &CameraHalDispatcherImpl::OnCameraHalClientConnectionError,
      base::Unretained(this), base::Unretained(client_observer.get())));
  AddClientObserver(std::move(client_observer));
  VLOG(1) << "Camera HAL client registered";
}

void CameraHalDispatcherImpl::GetJpegDecodeAccelerator(
    media::mojom::JpegDecodeAcceleratorRequest jda_request) {
  jda_factory_.Run(std::move(jda_request));
}

void CameraHalDispatcherImpl::GetJpegEncodeAccelerator(
    media::mojom::JpegEncodeAcceleratorRequest jea_request) {
  jea_factory_.Run(std::move(jea_request));
}

void CameraHalDispatcherImpl::CreateSocket(base::WaitableEvent* started) {
  DCHECK(blocking_io_task_runner_->BelongsToCurrentThread());

  base::FilePath socket_path(kArcCamera3SocketPath);
  mojo::edk::ScopedInternalPlatformHandle socket_fd =
      mojo::edk::CreateServerHandle(
          mojo::edk::NamedPlatformHandle(socket_path.value()));
  if (!socket_fd.is_valid()) {
    LOG(ERROR) << "Failed to create the socket file: " << kArcCamera3SocketPath;
    started->Signal();
    return;
  }

  // Change permissions on the socket.
  struct group arc_camera_group;
  struct group* result = nullptr;
  char buf[1024];
  if (HANDLE_EINTR(getgrnam_r(kArcCameraGroup, &arc_camera_group, buf,
                              sizeof(buf), &result)) < 0) {
    PLOG(ERROR) << "getgrnam_r()";
    started->Signal();
    return;
  }

  if (!result) {
    LOG(ERROR) << "Group '" << kArcCameraGroup << "' not found";
    started->Signal();
    return;
  }

  if (HANDLE_EINTR(chown(kArcCamera3SocketPath, -1, arc_camera_group.gr_gid)) <
      0) {
    PLOG(ERROR) << "chown()";
    started->Signal();
    return;
  }

  if (!base::SetPosixFilePermissions(socket_path, 0660)) {
    PLOG(ERROR) << "Could not set permissions: " << socket_path.value();
    started->Signal();
    return;
  }

  blocking_io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CameraHalDispatcherImpl::StartServiceLoop,
                     base::Unretained(this), base::Passed(&socket_fd),
                     base::Unretained(started)));
}

void CameraHalDispatcherImpl::StartServiceLoop(
    mojo::edk::ScopedInternalPlatformHandle socket_fd,
    base::WaitableEvent* started) {
  DCHECK(blocking_io_task_runner_->BelongsToCurrentThread());
  DCHECK(!proxy_fd_.is_valid());
  DCHECK(!cancel_pipe_.is_valid());
  DCHECK(socket_fd.is_valid());

  base::ScopedFD cancel_fd;
  if (!CreatePipe(&cancel_fd, &cancel_pipe_)) {
    started->Signal();
    LOG(ERROR) << "Failed to create cancel pipe";
    return;
  }

  proxy_fd_ = std::move(socket_fd);
  started->Signal();
  VLOG(1) << "CameraHalDispatcherImpl started; waiting for incoming connection";

  while (true) {
    if (!WaitForSocketReadable(proxy_fd_.get().handle, cancel_fd.get())) {
      VLOG(1) << "Quit CameraHalDispatcherImpl IO thread";
      return;
    }

    mojo::edk::ScopedInternalPlatformHandle accepted_fd;
    if (mojo::edk::ServerAcceptConnection(proxy_fd_, &accepted_fd, false) &&
        accepted_fd.is_valid()) {
      VLOG(1) << "Accepted a connection";
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

      struct iovec iov = {const_cast<char*>(token.c_str()), token.length()};
      ssize_t result = mojo::edk::PlatformChannelSendmsgWithHandles(
          accepted_fd, &iov, 1, handles);
      if (result == -1) {
        PLOG(ERROR) << "sendmsg()";
      } else {
        proxy_task_runner_->PostTask(
            FROM_HERE,
            base::BindOnce(&CameraHalDispatcherImpl::OnPeerConnected,
                           base::Unretained(this), base::Passed(&pipe)));
      }
    }
  }
}

void CameraHalDispatcherImpl::AddClientObserverOnProxyThread(
    std::unique_ptr<CameraClientObserver> observer) {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());
  if (camera_hal_server_) {
    EstablishMojoChannel(observer.get());
  }
  client_observers_.insert(std::move(observer));
}

void CameraHalDispatcherImpl::EstablishMojoChannel(
    CameraClientObserver* client_observer) {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());
  cros::mojom::CameraModulePtr camera_module_ptr;
  cros::mojom::CameraModuleRequest camera_module_request =
      mojo::MakeRequest(&camera_module_ptr);
  camera_hal_server_->CreateChannel(std::move(camera_module_request));
  client_observer->OnChannelCreated(std::move(camera_module_ptr));
}

void CameraHalDispatcherImpl::OnPeerConnected(
    mojo::ScopedMessagePipeHandle message_pipe) {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());
  binding_set_.AddBinding(
      this, cros::mojom::CameraHalDispatcherRequest(std::move(message_pipe)));
  VLOG(1) << "New CameraHalDispatcher binding added";
}

void CameraHalDispatcherImpl::OnCameraHalServerConnectionError() {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());
  VLOG(1) << "Camera HAL server connection lost";
  camera_hal_server_.reset();
}

void CameraHalDispatcherImpl::OnCameraHalClientConnectionError(
    CameraClientObserver* client_observer) {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());
  for (auto& it : client_observers_) {
    if (it.get() == client_observer) {
      client_observers_.erase(it);
      VLOG(1) << "Camera HAL client connection lost";
      break;
    }
  }
}

void CameraHalDispatcherImpl::StopOnProxyThread() {
  DCHECK(proxy_task_runner_->BelongsToCurrentThread());
  if (!base::DeleteFile(base::FilePath(kArcCamera3SocketPath),
                        /* recursive */ false)) {
    LOG(ERROR) << "Failed to delete " << kArcCamera3SocketPath;
  }
  // Close |cancel_pipe_| to quit the loop in WaitForIncomingConnection.
  cancel_pipe_.reset();
  client_observers_.clear();
  camera_hal_server_.reset();
  binding_set_.CloseAllBindings();
}

}  // namespace media
