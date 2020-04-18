// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_PUBLIC_CPP_PERFETTO_PRODUCER_CLIENT_H_
#define SERVICES_TRACING_PUBLIC_CPP_PERFETTO_PRODUCER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/atomicops.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/tracing/public/cpp/perfetto/task_runner.h"
#include "services/tracing/public/mojom/perfetto_service.mojom.h"
#include "third_party/perfetto/include/perfetto/tracing/core/service.h"

namespace perfetto {
class SharedMemoryArbiter;
}  // namespace perfetto

namespace tracing {

class MojoSharedMemory;

// This class is the per-process client side of the Perfetto
// producer, and is responsible for creating specific kinds
// of DataSources (like ChromeTracing) on demand, and provide
// them with TraceWriters and a configuration to start logging.

// Implementations of new DataSources should:
// * Implement ProducerClient::DataSourceBase.
// * Add a new data source name in perfetto_service.mojom.
// * Register the data source with Perfetto in ProducerHost::OnConnect.
// * Construct the new implementation when requested to
//   in ProducerClient::CreateDataSourceInstance.
class COMPONENT_EXPORT(TRACING_CPP) ProducerClient
    : public mojom::ProducerClient,
      public perfetto::Service::ProducerEndpoint {
 public:
  ProducerClient();
  ~ProducerClient() override;

  static void DeleteSoon(std::unique_ptr<ProducerClient>);

  // Returns the taskrunner used by Perfetto.
  static base::SequencedTaskRunner* GetTaskRunner();

  // Create the messagepipes that'll be used to connect
  // to the service-side ProducerHost, on the correct
  // sequence. The callback will be called on same sequence
  // as CreateMojoMessagepipes() got called on.
  using MessagepipesReadyCallback =
      base::OnceCallback<void(mojom::ProducerClientPtr,
                              mojom::ProducerHostRequest)>;
  void CreateMojoMessagepipes(MessagepipesReadyCallback);

  // mojom::ProducerClient implementation.
  // Called through Mojo by the ProducerHost on the service-side to control
  // tracing and toggle specific DataSources.
  void OnTracingStart(mojo::ScopedSharedBufferHandle shared_memory) override;
  void CreateDataSourceInstance(
      uint64_t id,
      mojom::DataSourceConfigPtr data_source_config) override;

  void TearDownDataSourceInstance(uint64_t id) override;
  void Flush(uint64_t flush_request_id,
             const std::vector<uint64_t>& data_source_ids) override;

  // perfetto::Service::ProducerEndpoint implementation.
  // Used by the TraceWriters
  // to signal Perfetto that shared memory chunks are ready
  // for consumption.
  void CommitData(const perfetto::CommitDataRequest& commit,
                  CommitDataCallback callback) override;
  // Used by the DataSource implementations to create TraceWriters
  // for writing their protobufs
  std::unique_ptr<perfetto::TraceWriter> CreateTraceWriter(
      perfetto::BufferID target_buffer) override;
  perfetto::SharedMemory* shared_memory() const override;

  // These ProducerEndpoint functions are only used on the service
  // side and should not be called on the clients.
  void RegisterDataSource(const perfetto::DataSourceDescriptor&) override;
  void UnregisterDataSource(const std::string& name) override;
  size_t shared_buffer_page_size_kb() const override;
  void NotifyFlushComplete(perfetto::FlushRequestID) override;

  static void ResetTaskRunnerForTesting();

 private:
  void CommitDataOnSequence(mojom::CommitDataRequestPtr request);
  // The callback will be run on the |origin_task_runner|, meaning
  // the same sequence as CreateMojoMessagePipes() got called on.
  void CreateMojoMessagepipesOnSequence(
      scoped_refptr<base::SequencedTaskRunner> origin_task_runner,
      MessagepipesReadyCallback,
      mojom::ProducerClientRequest,
      mojom::ProducerClientPtr);

  std::unique_ptr<mojo::Binding<mojom::ProducerClient>> binding_;
  std::unique_ptr<perfetto::SharedMemoryArbiter> shared_memory_arbiter_;
  mojom::ProducerHostPtr producer_host_;
  std::unique_ptr<MojoSharedMemory> shared_memory_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ProducerClient);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_PUBLIC_CPP_PERFETTO_PRODUCER_CLIENT_H_
