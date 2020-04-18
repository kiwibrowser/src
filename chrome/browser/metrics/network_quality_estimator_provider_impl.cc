// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/network_quality_estimator_provider_impl.h"

#include "base/sequenced_task_runner.h"
#include "chrome/browser/io_thread.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context.h"

namespace net {
class NetworkQualityEstimator;
}

namespace metrics {

namespace {

void GetNetworkQualityEstimatorOnIOThread(
    base::Callback<void(net::NetworkQualityEstimator*)> io_callback,
    IOThread* io_thread) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  net::NetworkQualityEstimator* network_quality_estimator =
      io_thread->globals()->system_request_context->network_quality_estimator();
  // |network_quality_estimator| may be nullptr when running the network service
  // out of process.
  // TODO(mmenke):  Hook this up through a Mojo API.
  if (network_quality_estimator) {
    // It is safe to run |io_callback| here since it is guaranteed to be
    // non-null.
    io_callback.Run(network_quality_estimator);
  }
}

}  // namespace

NetworkQualityEstimatorProviderImpl::NetworkQualityEstimatorProviderImpl(
    IOThread* io_thread)
    : io_thread_(io_thread), weak_ptr_factory_(this) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(io_thread_);
}

NetworkQualityEstimatorProviderImpl::~NetworkQualityEstimatorProviderImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

scoped_refptr<base::SequencedTaskRunner>
NetworkQualityEstimatorProviderImpl::GetTaskRunner() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::IO);
}

void NetworkQualityEstimatorProviderImpl::PostReplyNetworkQualityEstimator(
    base::Callback<void(net::NetworkQualityEstimator*)> io_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!content::BrowserThread::IsThreadInitialized(
          content::BrowserThread::IO)) {
    // IO thread is not yet initialized. Try again in the next message pump.
    bool task_posted = base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&NetworkQualityEstimatorProviderImpl::
                                  PostReplyNetworkQualityEstimator,
                              weak_ptr_factory_.GetWeakPtr(), io_callback));
    DCHECK(task_posted);
    return;
  }

  bool task_posted =
      content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
          ->PostTask(FROM_HERE,
                     base::Bind(&GetNetworkQualityEstimatorOnIOThread,
                                io_callback, io_thread_));
  DCHECK(task_posted);
}

}  // namespace metrics
