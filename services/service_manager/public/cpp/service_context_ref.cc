// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/cpp/service_context_ref.h"

#include "base/bind.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace service_manager {

class ServiceContextRefImpl : public ServiceContextRef {
 public:
  ServiceContextRefImpl(
      base::WeakPtr<ServiceContextRefFactory> factory,
      scoped_refptr<base::SequencedTaskRunner> service_task_runner)
      : factory_(factory), service_task_runner_(service_task_runner) {
    // This object is not thread-safe but may be used exclusively on a different
    // thread from the one which constructed it.
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  ~ServiceContextRefImpl() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    if (service_task_runner_->RunsTasksInCurrentSequence() && factory_) {
      factory_->Release();
    } else {
      service_task_runner_->PostTask(
          FROM_HERE, base::Bind(&ServiceContextRefFactory::Release, factory_));
    }
  }

 private:
  // ServiceContextRef:
  std::unique_ptr<ServiceContextRef> Clone() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    if (service_task_runner_->RunsTasksInCurrentSequence() && factory_) {
      factory_->AddRef();
    } else {
      service_task_runner_->PostTask(
          FROM_HERE, base::Bind(&ServiceContextRefFactory::AddRef, factory_));
    }

    return std::make_unique<ServiceContextRefImpl>(factory_,
                                                   service_task_runner_);
  }

  base::WeakPtr<ServiceContextRefFactory> factory_;
  scoped_refptr<base::SequencedTaskRunner> service_task_runner_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ServiceContextRefImpl);
};

ServiceContextRefFactory::ServiceContextRefFactory(
    base::RepeatingClosure quit_closure)
    : quit_closure_(std::move(quit_closure)), weak_factory_(this) {
  DCHECK(!quit_closure_.is_null());
}

ServiceContextRefFactory::~ServiceContextRefFactory() {}

std::unique_ptr<ServiceContextRef> ServiceContextRefFactory::CreateRef() {
  AddRef();
  return std::make_unique<ServiceContextRefImpl>(
      weak_factory_.GetWeakPtr(), base::SequencedTaskRunnerHandle::Get());
}

void ServiceContextRefFactory::AddRef() {
  ++ref_count_;
}

void ServiceContextRefFactory::Release() {
  if (!--ref_count_)
    quit_closure_.Run();
}

}  // namespace service_manager
