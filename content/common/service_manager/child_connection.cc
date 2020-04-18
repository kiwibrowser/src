// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_manager/child_connection.h"

#include <stdint.h>
#include <utility>

#include "base/macros.h"
#include "content/common/child.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace content {

class ChildConnection::IOThreadContext
    : public base::RefCountedThreadSafe<IOThreadContext> {
 public:
  IOThreadContext() {}

  void Initialize(const service_manager::Identity& child_identity,
                  service_manager::Connector* connector,
                  mojo::ScopedMessagePipeHandle service_pipe,
                  scoped_refptr<base::SequencedTaskRunner> io_task_runner) {
    DCHECK(!io_task_runner_);
    io_task_runner_ = io_task_runner;
    std::unique_ptr<service_manager::Connector> io_thread_connector;
    if (connector)
      connector_ = connector->Clone();
    child_identity_ = child_identity;
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&IOThreadContext::InitializeOnIOThread, this,
                                  child_identity, std::move(service_pipe)));
  }

  void BindInterface(const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&IOThreadContext::BindInterfaceOnIOThread, this,
                       interface_name, std::move(interface_pipe)));
  }

  void ShutDown() {
    if (!io_task_runner_)
      return;
    bool posted = io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&IOThreadContext::ShutDownOnIOThread, this));
    DCHECK(posted);
  }

  void BindInterfaceOnIOThread(const std::string& interface_name,
                               mojo::ScopedMessagePipeHandle request_handle) {
    if (connector_) {
      connector_->BindInterface(child_identity_, interface_name,
                                std::move(request_handle));
    }
  }

  void SetProcessHandle(base::ProcessHandle handle) {
    DCHECK(io_task_runner_);
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&IOThreadContext::SetProcessHandleOnIOThread,
                                  this, handle));
  }

 private:
  friend class base::RefCountedThreadSafe<IOThreadContext>;

  virtual ~IOThreadContext() {}

  void InitializeOnIOThread(
      const service_manager::Identity& child_identity,
      mojo::ScopedMessagePipeHandle service_pipe) {
    service_manager::mojom::ServicePtr service;
    service.Bind(mojo::InterfacePtrInfo<service_manager::mojom::Service>(
        std::move(service_pipe), 0u));
    auto pid_receiver_request = mojo::MakeRequest(&pid_receiver_);

    if (connector_) {
      connector_->StartService(child_identity, std::move(service),
                               std::move(pid_receiver_request));
      connector_->BindInterface(child_identity, &child_);
    }
  }

  void ShutDownOnIOThread() {
    connector_.reset();
    pid_receiver_.reset();
  }

  void SetProcessHandleOnIOThread(base::ProcessHandle handle) {
    DCHECK(pid_receiver_.is_bound());
    pid_receiver_->SetPID(base::GetProcId(handle));
    pid_receiver_.reset();
  }

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  // Usable from the IO thread only.
  std::unique_ptr<service_manager::Connector> connector_;
  service_manager::Identity child_identity_;
  // ServiceManagerConnection in the child monitors the lifetime of this pipe.
  mojom::ChildPtr child_;
  service_manager::mojom::PIDReceiverPtr pid_receiver_;

  DISALLOW_COPY_AND_ASSIGN(IOThreadContext);
};

ChildConnection::ChildConnection(
    const service_manager::Identity& child_identity,
    mojo::edk::OutgoingBrokerClientInvitation* invitation,
    service_manager::Connector* connector,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : context_(new IOThreadContext),
      child_identity_(child_identity),
      weak_factory_(this) {
  // TODO(rockot): Use a constant name for this pipe attachment rather than a
  // randomly generated token.
  service_token_ = mojo::edk::GenerateRandomToken();
  context_->Initialize(child_identity_, connector,
                       invitation->AttachMessagePipe(service_token_),
                       io_task_runner);
}

ChildConnection::~ChildConnection() {
  context_->ShutDown();
}

void ChildConnection::BindInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  context_->BindInterface(interface_name, std::move(interface_pipe));
}

void ChildConnection::SetProcessHandle(base::ProcessHandle handle) {
  process_handle_ = handle;
  context_->SetProcessHandle(handle);
}

}  // namespace content
