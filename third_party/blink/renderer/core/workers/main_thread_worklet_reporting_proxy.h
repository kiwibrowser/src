// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_MAIN_THREAD_WORKLET_REPORTING_PROXY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_MAIN_THREAD_WORKLET_REPORTING_PROXY_H_

#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"

namespace blink {

class Document;

class CORE_EXPORT MainThreadWorkletReportingProxy
    : public WorkerReportingProxy {
 public:
  explicit MainThreadWorkletReportingProxy(Document*);
  ~MainThreadWorkletReportingProxy() override = default;

  // Implements WorkerReportingProxy.
  void CountFeature(WebFeature) override;
  void CountDeprecation(WebFeature) override;
  void DidTerminateWorkerThread() override;

 private:
  Persistent<Document> document_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_MAIN_THREAD_WORKLET_REPORTING_PROXY_H_
