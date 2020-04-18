// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/black_hole_protocol_handler.h"

#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_job.h"

namespace headless {
namespace {
class BlackHoleRequestJob : public net::URLRequestJob {
 public:
  BlackHoleRequestJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate);

  // net::URLRequestJob implementation:
  void Start() override;
  void Kill() override;

 private:
  ~BlackHoleRequestJob() override;

  void StartAsync();

  base::WeakPtrFactory<BlackHoleRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlackHoleRequestJob);
};

BlackHoleRequestJob::BlackHoleRequestJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate)
    : net::URLRequestJob(request, network_delegate), weak_factory_(this) {}

BlackHoleRequestJob::~BlackHoleRequestJob() = default;

void BlackHoleRequestJob::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BlackHoleRequestJob::StartAsync,
                                weak_factory_.GetWeakPtr()));
}

void BlackHoleRequestJob::StartAsync() {
  // Fail every request!
  NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                         net::ERR_FILE_NOT_FOUND));
}

void BlackHoleRequestJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}
}  // namespace

BlackHoleProtocolHandler::BlackHoleProtocolHandler() = default;
BlackHoleProtocolHandler::~BlackHoleProtocolHandler() = default;

net::URLRequestJob* BlackHoleProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new BlackHoleRequestJob(request, network_delegate);
}

}  // namespace headless
