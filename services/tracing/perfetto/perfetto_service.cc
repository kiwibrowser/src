// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/perfetto/perfetto_service.h"

#include <utility>

#include "base/task_scheduler/post_task.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/tracing/perfetto/producer_host.h"
#include "services/tracing/public/cpp/perfetto/shared_memory.h"

#include "third_party/perfetto/include/perfetto/tracing/core/service.h"

namespace tracing {

namespace {

const char kPerfettoProducerName[] = "org.chromium.perfetto_producer";

PerfettoService* g_perfetto_service;

// Just used to destroy disconnected clients.
template <typename T>
void OnClientDisconnect(std::unique_ptr<T>) {}

}  // namespace

/*
TODO(oysteine): Right now the Perfetto service runs on a dedicated
thread for a couple of reasons:
* The sequence needs to be locked to a specific thread, or Perfetto's
  thread-checker will barf.
* The PerfettoTracingCoordinator uses
  mojo::BlockingCopyFromString to pass the string to the tracing
  controller, which requires the WithBaseSyncPrimitives task trait and
  SingleThreadTaskRunners which use this trait need to be running on a
  dedicated trait to avoid blocking other sequences.
* If a client fills up its Shared Memory Buffer when writing a Perfetto
  event proto, it'll stall until the Perfetto service clears up space.
  This won't happen if the client and the service happens to run on the same
  thread (the Mojo calls will never be executed).

The above should be resolved before we move the Perfetto usage out from the
flag so we can run this on non-thread-bound sequence.
*/

PerfettoService::PerfettoService(
    scoped_refptr<base::SequencedTaskRunner> task_runner_for_testing)
    : perfetto_task_runner_(
          task_runner_for_testing
              ? task_runner_for_testing
              : base::CreateSingleThreadTaskRunnerWithTraits(
                    {base::MayBlock(), base::WithBaseSyncPrimitives(),
                     base::TaskPriority::BACKGROUND},
                    base::SingleThreadTaskRunnerThreadMode::DEDICATED)) {
  DCHECK(!g_perfetto_service);
  g_perfetto_service = this;
  DETACH_FROM_SEQUENCE(sequence_checker_);
  perfetto_task_runner_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&PerfettoService::CreateServiceOnSequence,
                                base::Unretained(this)));
}

PerfettoService::~PerfettoService() {
  DCHECK_EQ(g_perfetto_service, this);
  g_perfetto_service = nullptr;
}

// static
PerfettoService* PerfettoService::GetInstance() {
  return g_perfetto_service;
}

// static
void PerfettoService::DestroyOnSequence(
    std::unique_ptr<PerfettoService> service) {
  PerfettoService::GetInstance()->task_runner()->DeleteSoon(FROM_HERE,
                                                            std::move(service));
}

void PerfettoService::CreateServiceOnSequence() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  service_ = perfetto::Service::CreateInstance(
      std::make_unique<MojoSharedMemory::Factory>(), &perfetto_task_runner_);
  DCHECK(service_);
}

perfetto::Service* PerfettoService::GetService() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return service_.get();
}

void PerfettoService::BindRequest(
    mojom::PerfettoServiceRequest request,
    const service_manager::BindSourceInfo& source_info) {
  perfetto_task_runner_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&PerfettoService::BindOnSequence, base::Unretained(this),
                     std::move(request), source_info.identity));
}

void PerfettoService::BindOnSequence(
    mojom::PerfettoServiceRequest request,
    const service_manager::Identity& identity) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request), identity);
}

void PerfettoService::ConnectToProducerHost(
    mojom::ProducerClientPtr producer_client,
    mojom::ProducerHostRequest producer_host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto new_producer = std::make_unique<ProducerHost>();
  new_producer->Initialize(std::move(producer_client), std::move(producer_host),
                           service_.get(), kPerfettoProducerName);
  new_producer->set_connection_error_handler(base::BindOnce(
      &OnClientDisconnect<ProducerHost>, std::move(new_producer)));
}

}  // namespace tracing
