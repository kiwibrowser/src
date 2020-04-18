// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/barrier_closure.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

// Receiver is a class to read a Mojo data pipe. Received data are stored in
// chunks. Lives on the IO thread. Receiver is owned by Internal via
// BundledReceivers. It is created to read the script body or metadata from a
// data pipe, and is destroyed when the read finishes.
class Receiver {
 public:
  using BytesChunk = blink::WebVector<char>;

  Receiver(mojo::ScopedDataPipeConsumerHandle handle,
           uint64_t total_bytes,
           scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : handle_(std::move(handle)),
        watcher_(FROM_HERE,
                 mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                 std::move(task_runner)),
        remaining_bytes_(total_bytes) {}

  void Start(base::OnceClosure callback) {
    if (!handle_.is_valid()) {
      std::move(callback).Run();
      return;
    }
    callback_ = std::move(callback);
    // base::Unretained is safe because |watcher_| is owned by |this|.
    MojoResult rv = watcher_.Watch(
        handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
        base::Bind(&Receiver::OnReadable, base::Unretained(this)));
    DCHECK_EQ(MOJO_RESULT_OK, rv);
    watcher_.ArmOrNotify();
  }

  void OnReadable(MojoResult) {
    // It isn't necessary to handle MojoResult here since BeginReadDataRaw()
    // returns an equivalent error.
    const void* buffer = nullptr;
    uint32_t bytes_read = 0;
    MojoResult rv =
        handle_->BeginReadData(&buffer, &bytes_read, MOJO_READ_DATA_FLAG_NONE);
    switch (rv) {
      case MOJO_RESULT_BUSY:
      case MOJO_RESULT_INVALID_ARGUMENT:
        NOTREACHED();
        return;
      case MOJO_RESULT_FAILED_PRECONDITION:
        // Closed by peer.
        OnCompleted();
        return;
      case MOJO_RESULT_SHOULD_WAIT:
        watcher_.ArmOrNotify();
        return;
      case MOJO_RESULT_OK:
        break;
      default:
        // mojo::BeginReadDataRaw() should not return any other values.
        // Notify the error to the browser by resetting the handle even though
        // it's in the middle of data transfer.
        OnCompleted();
        return;
    }

    if (bytes_read > 0)
      chunks_.emplace_back(static_cast<const char*>(buffer), bytes_read);

    rv = handle_->EndReadData(bytes_read);
    DCHECK_EQ(rv, MOJO_RESULT_OK);
    CHECK_GE(remaining_bytes_, bytes_read);
    remaining_bytes_ -= bytes_read;
    watcher_.ArmOrNotify();
  }

  bool is_running() const { return handle_.is_valid(); }
  bool has_received_all_data() const { return remaining_bytes_ == 0; }

  blink::WebVector<BytesChunk> TakeChunks() {
    DCHECK(!is_running());
    return blink::WebVector<BytesChunk>(std::move(chunks_));
  }

 private:
  void OnCompleted() {
    handle_.reset();
    watcher_.Cancel();
    if (!has_received_all_data())
      chunks_.clear();
    DCHECK(callback_);
    std::move(callback_).Run();
  }

  base::OnceClosure callback_;
  mojo::ScopedDataPipeConsumerHandle handle_;
  mojo::SimpleWatcher watcher_;

  // std::vector is internally used because blink::WebVector is immutable and
  // cannot append data.
  std::vector<BytesChunk> chunks_;
  uint64_t remaining_bytes_;
};

using RawScriptData =
    blink::WebServiceWorkerInstalledScriptsManager::RawScriptData;

// BundledReceivers is a helper class to wait for the end of reading body and
// meta data. Lives on the IO thread.
class BundledReceivers {
 public:
  BundledReceivers(mojo::ScopedDataPipeConsumerHandle meta_data_handle,
                   uint64_t meta_data_size,
                   mojo::ScopedDataPipeConsumerHandle body_handle,
                   uint64_t body_size,
                   scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : meta_data_(std::move(meta_data_handle), meta_data_size, task_runner),
        body_(std::move(body_handle), body_size, std::move(task_runner)) {}

  // Starts reading the pipes and invokes |callback| when both are finished.
  void Start(base::OnceClosure callback) {
    base::RepeatingClosure wait_all_closure =
        base::BarrierClosure(2, std::move(callback));
    meta_data_.Start(wait_all_closure);
    body_.Start(std::move(wait_all_closure));
  }

  Receiver* meta_data() { return &meta_data_; }
  Receiver* body() { return &body_; }

 private:
  Receiver meta_data_;
  Receiver body_;
};

// Internal lives on the IO thread. This receives
// blink::mojom::ServiceWorkerScriptInfo for all installed scripts and then
// starts reading the body and meta data from the browser. This instance will be
// kept alive as long as the Mojo's connection is established.
class Internal : public blink::mojom::ServiceWorkerInstalledScriptsManager {
 public:
  // Called on the IO thread.
  // Creates and binds a new Internal instance to |request|.
  static void Create(
      scoped_refptr<ThreadSafeScriptContainer> script_container,
      blink::mojom::ServiceWorkerInstalledScriptsManagerRequest request,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
    mojo::MakeStrongBinding(
        std::make_unique<Internal>(std::move(script_container),
                                   std::move(task_runner)),
        std::move(request));
  }

  Internal(scoped_refptr<ThreadSafeScriptContainer> script_container,
           scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : script_container_(std::move(script_container)),
        task_runner_(std::move(task_runner)),
        weak_factory_(this) {}

  ~Internal() override {
    DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);
    // Wake up a waiting thread so it does not wait forever. If the script has
    // not been added yet, that means something went wrong. From here,
    // script_container_->Wait() will return false if the script hasn't been
    // added yet.
    script_container_->OnAllDataAddedOnIOThread();
  }

  // Implements blink::mojom::ServiceWorkerInstalledScriptsManager.
  // Called on the IO thread.
  void TransferInstalledScript(
      blink::mojom::ServiceWorkerScriptInfoPtr script_info) override {
    DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);
    GURL script_url = script_info->script_url;
    auto receivers = std::make_unique<BundledReceivers>(
        std::move(script_info->meta_data), script_info->meta_data_size,
        std::move(script_info->body), script_info->body_size, task_runner_);
    receivers->Start(base::BindOnce(&Internal::OnScriptReceived,
                                    weak_factory_.GetWeakPtr(),
                                    std::move(script_info)));
    DCHECK(!base::ContainsKey(running_receivers_, script_url));
    running_receivers_[script_url] = std::move(receivers);
  }

  // Called on the IO thread.
  void OnScriptReceived(blink::mojom::ServiceWorkerScriptInfoPtr script_info) {
    DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);
    const GURL& script_url = script_info->script_url;
    auto iter = running_receivers_.find(script_url);
    DCHECK(iter != running_receivers_.end());
    std::unique_ptr<BundledReceivers> receivers = std::move(iter->second);
    DCHECK(receivers);
    if (!receivers->body()->has_received_all_data() ||
        !receivers->meta_data()->has_received_all_data()) {
      script_container_->AddOnIOThread(script_url,
                                       RawScriptData::CreateInvalidInstance());
      running_receivers_.erase(iter);
      return;
    }

    auto script_data = RawScriptData::Create(
        blink::WebString::FromUTF8(script_info->encoding),
        receivers->body()->TakeChunks(), receivers->meta_data()->TakeChunks());
    for (const auto& entry : script_info->headers) {
      script_data->AddHeader(blink::WebString::FromUTF8(entry.first),
                             blink::WebString::FromUTF8(entry.second));
    }
    script_container_->AddOnIOThread(script_url, std::move(script_data));
    running_receivers_.erase(iter);
  }

 private:
  THREAD_CHECKER(io_thread_checker_);
  std::map<GURL, std::unique_ptr<BundledReceivers>> running_receivers_;
  scoped_refptr<ThreadSafeScriptContainer> script_container_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtrFactory<Internal> weak_factory_;
};

}  // namespace

// static
std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager>
WebServiceWorkerInstalledScriptsManagerImpl::Create(
    blink::mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  auto script_container = base::MakeRefCounted<ThreadSafeScriptContainer>();
  blink::mojom::ServiceWorkerInstalledScriptsManagerHostPtr manager_host_ptr(
      std::move(installed_scripts_info->manager_host_ptr));
  std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager> manager =
      base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
          new WebServiceWorkerInstalledScriptsManagerImpl(
              std::move(installed_scripts_info->installed_urls),
              script_container, std::move(manager_host_ptr)));
  io_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&Internal::Create, script_container,
                     std::move(installed_scripts_info->manager_request),
                     io_task_runner));
  return manager;
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        std::vector<GURL>&& installed_urls,
        scoped_refptr<ThreadSafeScriptContainer> script_container,
        blink::mojom::ServiceWorkerInstalledScriptsManagerHostPtr manager_host)
    : installed_urls_(installed_urls.begin(), installed_urls.end()),
      script_container_(std::move(script_container)),
      manager_host_(
          blink::mojom::ThreadSafeServiceWorkerInstalledScriptsManagerHostPtr::
              Create(std::move(manager_host))) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() = default;

bool WebServiceWorkerInstalledScriptsManagerImpl::IsScriptInstalled(
    const blink::WebURL& script_url) const {
  return base::ContainsKey(installed_urls_, script_url);
}

std::unique_ptr<RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& script_url) {
  TRACE_EVENT1("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData",
               "script_url", script_url.GetString().Utf8());
  if (!IsScriptInstalled(script_url))
    return nullptr;

  ThreadSafeScriptContainer::ScriptStatus status =
      script_container_->GetStatusOnWorkerThread(script_url);
  // If the script has already been taken, request the browser to send the
  // script.
  if (status == ThreadSafeScriptContainer::ScriptStatus::kTaken) {
    script_container_->ResetOnWorkerThread(script_url);
    (*manager_host_)->RequestInstalledScript(script_url);
    status = script_container_->GetStatusOnWorkerThread(script_url);
  }

  // If the script has not been received at this point, wait for arrival by
  // blocking the worker thread.
  if (status == ThreadSafeScriptContainer::ScriptStatus::kPending) {
    // Wait for arrival of the script.
    const bool success = script_container_->WaitOnWorkerThread(script_url);
    // It can fail due to an error on Mojo pipes.
    if (!success)
      return RawScriptData::CreateInvalidInstance();
    status = script_container_->GetStatusOnWorkerThread(script_url);
    DCHECK_NE(ThreadSafeScriptContainer::ScriptStatus::kPending, status);
  }

  if (status == ThreadSafeScriptContainer::ScriptStatus::kFailed)
    return RawScriptData::CreateInvalidInstance();
  DCHECK_EQ(ThreadSafeScriptContainer::ScriptStatus::kReceived, status);

  return script_container_->TakeOnWorkerThread(script_url);
}

}  // namespace content
