// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/gpu/gpu.h"

#include <memory>
#include <string>
#include <utility>

#include "base/debug/stack_trace.h"
#include "base/memory/ptr_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/scheduling_priority.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/cpp/gpu/client_gpu_memory_buffer_manager.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace ui {

namespace {

mojom::GpuPtr DefaultFactory(service_manager::Connector* connector,
                             const std::string& service_name) {
  mojom::GpuPtr gpu_ptr;
  connector->BindInterface(service_name, &gpu_ptr);
  return gpu_ptr;
}

}  // namespace

// Encapsulates a mojom::GpuPtr object that will be used on the IO thread. This
// is required because we can't install an error handler on a
// mojom::GpuThreadSafePtr to detect if the message pipe was closed. Only the
// constructor can be called on the main thread.
class Gpu::GpuPtrIO {
 public:
  GpuPtrIO() { DETACH_FROM_THREAD(thread_checker_); }
  ~GpuPtrIO() { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_); }

  void Initialize(mojom::GpuPtrInfo ptr_info) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    gpu_ptr_.Bind(std::move(ptr_info));
    gpu_ptr_.set_connection_error_handler(
        base::BindOnce(&GpuPtrIO::ConnectionError, base::Unretained(this)));
  }

  void EstablishGpuChannel(scoped_refptr<EstablishRequest> establish_request) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    DCHECK(!establish_request_);
    establish_request_ = std::move(establish_request);

    if (gpu_ptr_.encountered_error()) {
      ConnectionError();
    } else {
      gpu_ptr_->EstablishGpuChannel(base::BindRepeating(
          &GpuPtrIO::OnEstablishedGpuChannel, base::Unretained(this)));
    }
  }

  void CreateJpegDecodeAccelerator(
      media::mojom::JpegDecodeAcceleratorRequest request) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    gpu_ptr_->CreateJpegDecodeAccelerator(std::move(request));
  }

  void CreateVideoEncodeAcceleratorProvider(
      media::mojom::VideoEncodeAcceleratorProviderRequest request) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    gpu_ptr_->CreateVideoEncodeAcceleratorProvider(std::move(request));
  }

 private:
  void ConnectionError();
  void OnEstablishedGpuChannel(int client_id,
                               mojo::ScopedMessagePipeHandle channel_handle,
                               const gpu::GPUInfo& gpu_info,
                               const gpu::GpuFeatureInfo& gpu_feature_info);

  ui::mojom::GpuPtr gpu_ptr_;

  // This will point to a request that is waiting for the result of
  // EstablishGpuChannel(). |establish_request_| will be notified when the IPC
  // callback fires or if an interface connection error occurs.
  scoped_refptr<EstablishRequest> establish_request_;
  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(GpuPtrIO);
};

// Encapsulates a single request to establish a GPU channel.
class Gpu::EstablishRequest
    : public base::RefCountedThreadSafe<Gpu::EstablishRequest> {
 public:
  EstablishRequest(Gpu* parent,
                   scoped_refptr<base::SingleThreadTaskRunner> main_task_runner)
      : parent_(parent), main_task_runner_(main_task_runner) {}

  const scoped_refptr<gpu::GpuChannelHost>& gpu_channel() {
    return gpu_channel_;
  }

  // Sends EstablishGpuChannel() request using |gpu|. This must be called from
  // the IO thread so that the response is handled on the IO thread.
  void SendRequest(GpuPtrIO* gpu) {
    DCHECK(!main_task_runner_->BelongsToCurrentThread());
    {
      base::AutoLock lock(lock_);
      if (finished_)
        return;
    }

    gpu->EstablishGpuChannel(this);
  }

  // Sets a WaitableEvent so the main thread can block for a synchronous
  // request. This must be called from main thread.
  void SetWaitableEvent(base::WaitableEvent* establish_event) {
    DCHECK(main_task_runner_->BelongsToCurrentThread());
    base::AutoLock mutex(lock_);

    // If we've already received a response then don't reset |establish_event|.
    // The caller won't block and will immediately process the response.
    if (received_)
      return;

    establish_event_ = establish_event;
    establish_event_->Reset();
  }

  // Cancels the pending request. Any asynchronous calls back into this object
  // will return early and do nothing. This must be called from main thread.
  void Cancel() {
    DCHECK(main_task_runner_->BelongsToCurrentThread());
    base::AutoLock lock(lock_);
    DCHECK(!finished_);
    finished_ = true;
  }

  // This must be called after OnEstablishedGpuChannel() from the main thread.
  void FinishOnMain() {
    // No lock needed, everything will run on |main_task_runner_| now.
    DCHECK(main_task_runner_->BelongsToCurrentThread());
    DCHECK(received_);

    // It's possible to enter FinishedOnMain() twice if EstablishGpuChannel() is
    // called, the request returns and schedules a task on |main_task_runner_|.
    // If EstablishGpuChannelSync() runs before the scheduled task then it will
    // enter FinishedOnMain() immediately and finish. The scheduled task will
    // run later and return early here, doing nothing.
    if (finished_)
      return;

    finished_ = true;

    // |this| might be deleted when running Gpu::OnEstablishedGpuChannel().
    parent_->OnEstablishedGpuChannel();
  }

  void OnEstablishedGpuChannel(int client_id,
                               mojo::ScopedMessagePipeHandle channel_handle,
                               const gpu::GPUInfo& gpu_info,
                               const gpu::GpuFeatureInfo& gpu_feature_info) {
    DCHECK(!main_task_runner_->BelongsToCurrentThread());
    base::AutoLock lock(lock_);

    // Do nothing if Cancel() was called.
    if (finished_)
      return;

    DCHECK(!received_);
    received_ = true;
    if (channel_handle.is_valid()) {
      gpu_channel_ = base::MakeRefCounted<gpu::GpuChannelHost>(
          client_id, gpu_info, gpu_feature_info, std::move(channel_handle));
    }

    if (establish_event_) {
      // Gpu::EstablishGpuChannelSync() was called. Unblock the main thread and
      // let it finish.
      establish_event_->Signal();
    } else {
      main_task_runner_->PostTask(
          FROM_HERE, base::Bind(&EstablishRequest::FinishOnMain, this));
    }
  }

 private:
  friend class base::RefCountedThreadSafe<Gpu::EstablishRequest>;

  virtual ~EstablishRequest() = default;

  Gpu* const parent_;
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  base::WaitableEvent* establish_event_ = nullptr;

  base::Lock lock_;
  bool received_ = false;
  bool finished_ = false;

  scoped_refptr<gpu::GpuChannelHost> gpu_channel_;

  DISALLOW_COPY_AND_ASSIGN(EstablishRequest);
};

void Gpu::GpuPtrIO::ConnectionError() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!establish_request_)
    return;

  // Make sure |establish_request_| fails so the main thread doesn't block
  // forever after calling Gpu::EstablishGpuChannelSync().
  establish_request_->OnEstablishedGpuChannel(
      0, mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
      gpu::GpuFeatureInfo());
  establish_request_ = nullptr;
}

void Gpu::GpuPtrIO::OnEstablishedGpuChannel(
    int client_id,
    mojo::ScopedMessagePipeHandle channel_handle,
    const gpu::GPUInfo& gpu_info,
    const gpu::GpuFeatureInfo& gpu_feature_info) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(establish_request_);

  establish_request_->OnEstablishedGpuChannel(
      client_id, std::move(channel_handle), std::move(gpu_info),
      std::move(gpu_feature_info));
  establish_request_ = nullptr;
}

Gpu::Gpu(GpuPtrFactory factory,
         scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(std::move(task_runner)),
      gpu_memory_buffer_manager_(
          std::make_unique<ClientGpuMemoryBufferManager>(factory.Run())),
      gpu_(new GpuPtrIO(), base::OnTaskRunnerDeleter(io_task_runner_)) {
  DCHECK(main_task_runner_);
  DCHECK(io_task_runner_);

  // Initialize mojom::GpuPtr on the IO thread. |gpu_| can only be used on the
  // IO thread after this point. It is safe to use base::Unretained with |gpu_|
  // for IO thread tasks as |gpu_| is destroyed by an IO thread task posted from
  // the destructor.
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&GpuPtrIO::Initialize, base::Unretained(gpu_.get()),
                     base::Passed(factory.Run().PassInterface())));
}

Gpu::~Gpu() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (pending_request_) {
    pending_request_->Cancel();
    pending_request_ = nullptr;
  }

  if (gpu_channel_)
    gpu_channel_->DestroyChannel();
}

// static
std::unique_ptr<Gpu> Gpu::Create(
    service_manager::Connector* connector,
    const std::string& service_name,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  GpuPtrFactory factory =
      base::BindRepeating(&DefaultFactory, connector, service_name);
  return base::WrapUnique(new Gpu(std::move(factory), std::move(task_runner)));
}

scoped_refptr<viz::ContextProvider> Gpu::CreateContextProvider(
    scoped_refptr<gpu::GpuChannelHost> gpu_channel) {
  int32_t stream_id = 0;
  gpu::SchedulingPriority stream_priority = gpu::SchedulingPriority::kNormal;

  constexpr bool automatic_flushes = false;
  constexpr bool support_locking = false;
  constexpr bool support_grcontext = false;

  gpu::ContextCreationAttribs attributes;
  attributes.alpha_size = -1;
  attributes.depth_size = 0;
  attributes.stencil_size = 0;
  attributes.samples = 0;
  attributes.sample_buffers = 0;
  attributes.bind_generates_resource = false;
  attributes.lose_context_when_out_of_memory = true;
  return base::MakeRefCounted<ContextProviderCommandBuffer>(
      std::move(gpu_channel), GetGpuMemoryBufferManager(), stream_id,
      stream_priority, gpu::kNullSurfaceHandle,
      GURL("chrome://gpu/MusContextFactory"), automatic_flushes,
      support_locking, support_grcontext, gpu::SharedMemoryLimits(), attributes,
      command_buffer_metrics::MUS_CLIENT_CONTEXT);
}

void Gpu::CreateJpegDecodeAccelerator(
    media::mojom::JpegDecodeAcceleratorRequest jda_request) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&GpuPtrIO::CreateJpegDecodeAccelerator,
                     base::Unretained(gpu_.get()), base::Passed(&jda_request)));
}

void Gpu::CreateVideoEncodeAcceleratorProvider(
    media::mojom::VideoEncodeAcceleratorProviderRequest vea_provider_request) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&GpuPtrIO::CreateVideoEncodeAcceleratorProvider,
                                base::Unretained(gpu_.get()),
                                base::Passed(&vea_provider_request)));
}

void Gpu::EstablishGpuChannel(gpu::GpuChannelEstablishedCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  scoped_refptr<gpu::GpuChannelHost> channel = GetGpuChannel();
  if (channel) {
    std::move(callback).Run(std::move(channel));
    return;
  }

  establish_callbacks_.push_back(std::move(callback));
  SendEstablishGpuChannelRequest();
}

scoped_refptr<gpu::GpuChannelHost> Gpu::EstablishGpuChannelSync() {
  TRACE_EVENT0("mus", "Gpu::EstablishGpuChannelSync");
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  scoped_refptr<gpu::GpuChannelHost> channel = GetGpuChannel();
  if (channel)
    return channel;

  SendEstablishGpuChannelRequest();
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::SIGNALED);
  pending_request_->SetWaitableEvent(&event);
  event.Wait();

  // Running FinishOnMain() will create |gpu_channel_| and run any callbacks
  // from calls to EstablishGpuChannel() before we return from here.
  pending_request_->FinishOnMain();

  return gpu_channel_;
}

gpu::GpuMemoryBufferManager* Gpu::GetGpuMemoryBufferManager() {
  return gpu_memory_buffer_manager_.get();
}

void Gpu::LoseChannel() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (gpu_channel_) {
    gpu_channel_->DestroyChannel();
    gpu_channel_ = nullptr;
  }
}

scoped_refptr<gpu::GpuChannelHost> Gpu::GetGpuChannel() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (gpu_channel_ && gpu_channel_->IsLost())
    gpu_channel_ = nullptr;
  return gpu_channel_;
}

void Gpu::SendEstablishGpuChannelRequest() {
  if (pending_request_)
    return;

  pending_request_ =
      base::MakeRefCounted<EstablishRequest>(this, main_task_runner_);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&EstablishRequest::SendRequest, pending_request_,
                     base::Unretained(gpu_.get())));
}

void Gpu::OnEstablishedGpuChannel() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(pending_request_);
  DCHECK(!gpu_channel_);

  gpu_channel_ = pending_request_->gpu_channel();
  pending_request_ = nullptr;

  std::vector<gpu::GpuChannelEstablishedCallback> callbacks;
  callbacks.swap(establish_callbacks_);
  for (auto&& callback : std::move(callbacks))
    std::move(callback).Run(gpu_channel_);
}

}  // namespace ui
