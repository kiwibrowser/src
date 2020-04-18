// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_HAL_DISPATCHER_IMPL_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_HAL_DISPATCHER_IMPL_H_

#include <memory>
#include <set>

#include "base/memory/singleton.h"
#include "base/threading/thread.h"
#include "media/capture/capture_export.h"
#include "media/capture/video/chromeos/mojo/cros_camera_service.mojom.h"
#include "media/capture/video/video_capture_device_factory.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace base {

class SingleThreadTaskRunner;
class WaitableEvent;

}  // namespace base

namespace media {

class CAPTURE_EXPORT CameraClientObserver {
 public:
  virtual ~CameraClientObserver();
  virtual void OnChannelCreated(cros::mojom::CameraModulePtr camera_module) = 0;
};

// The CameraHalDispatcherImpl hosts and waits on the unix domain socket
// /var/run/camera3.sock.  CameraHalServer and CameraHalClients connect to the
// unix domain socket to create the initial Mojo connections with the
// CameraHalDisptcherImpl, and CameraHalDispatcherImpl then creates and
// dispaches the Mojo channels between CameraHalServer and CameraHalClients to
// establish direct Mojo connections between the CameraHalServer and the
// CameraHalClients.
//
// For general documentation about the CameraHalDispater Mojo interface see the
// comments in mojo/cros_camera_service.mojom.
class CAPTURE_EXPORT CameraHalDispatcherImpl final
    : public cros::mojom::CameraHalDispatcher {
 public:
  static CameraHalDispatcherImpl* GetInstance();

  bool Start(MojoJpegDecodeAcceleratorFactoryCB jda_factory,
             MojoJpegEncodeAcceleratorFactoryCB jea_factory);

  void AddClientObserver(std::unique_ptr<CameraClientObserver> observer);

  bool IsStarted();

  // CameraHalDispatcher implementations.
  void RegisterServer(cros::mojom::CameraHalServerPtr server) final;
  void RegisterClient(cros::mojom::CameraHalClientPtr client) final;
  void GetJpegDecodeAccelerator(
      media::mojom::JpegDecodeAcceleratorRequest jda_request) final;
  void GetJpegEncodeAccelerator(
      media::mojom::JpegEncodeAcceleratorRequest jea_request) final;

 private:
  friend struct base::DefaultSingletonTraits<CameraHalDispatcherImpl>;
  // Allow the test to construct the class directly.
  friend class CameraHalDispatcherImplTest;

  CameraHalDispatcherImpl();
  ~CameraHalDispatcherImpl() final;

  bool StartThreads();

  // Creates the unix domain socket for the camera client processes and the
  // camera HALv3 adapter process to connect.
  void CreateSocket(base::WaitableEvent* started);

  // Waits for incoming connections (from HAL process or from client processes).
  // Runs on |blocking_io_thread_|.
  void StartServiceLoop(mojo::edk::ScopedInternalPlatformHandle socket_fd,
                        base::WaitableEvent* started);

  void AddClientObserverOnProxyThread(
      std::unique_ptr<CameraClientObserver> observer);

  void EstablishMojoChannel(CameraClientObserver* client_observer);

  // Handler for incoming Mojo connection on the unix domain socket.
  void OnPeerConnected(mojo::ScopedMessagePipeHandle message_pipe);

  // Mojo connection error handlers.
  void OnCameraHalServerConnectionError();
  void OnCameraHalClientConnectionError(CameraClientObserver* client);

  void StopOnProxyThread();

  mojo::edk::ScopedInternalPlatformHandle proxy_fd_;
  base::ScopedFD cancel_pipe_;

  base::Thread proxy_thread_;
  base::Thread blocking_io_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> proxy_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> blocking_io_task_runner_;

  mojo::BindingSet<cros::mojom::CameraHalDispatcher> binding_set_;

  cros::mojom::CameraHalServerPtr camera_hal_server_;

  std::set<std::unique_ptr<CameraClientObserver>> client_observers_;

  MojoJpegDecodeAcceleratorFactoryCB jda_factory_;

  MojoJpegEncodeAcceleratorFactoryCB jea_factory_;

  DISALLOW_COPY_AND_ASSIGN(CameraHalDispatcherImpl);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_HAL_DISPATCHER_IMPL_H_
