// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_ENGINE_H_
#define COMPONENTS_CRONET_NATIVE_ENGINE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

namespace cronet {

class CronetURLRequestContext;

// Implementation of Cronet_Engine that uses CronetURLRequestContext.
class Cronet_EngineImpl : public Cronet_Engine {
 public:
  Cronet_EngineImpl();
  ~Cronet_EngineImpl() override;

  // Cronet_Engine implementation:
  Cronet_RESULT StartWithParams(Cronet_EngineParamsPtr params) override;
  bool StartNetLogToFile(Cronet_String file_name, bool log_all) override;
  void StopNetLog() override;
  Cronet_String GetVersionString() override;
  Cronet_String GetDefaultUserAgent() override;
  Cronet_RESULT Shutdown() override;

  // Check |result| and aborts if result is not SUCCESS and enableCheckResult
  // is true.
  Cronet_RESULT CheckResult(Cronet_RESULT result);

  CronetURLRequestContext* cronet_url_request_context() const {
    return context_.get();
  }

 private:
  class Callback;

  // Enable runtime CHECK of the result.
  bool enable_check_result_ = true;

  // Synchronize access to member variables from different threads.
  base::Lock lock_;
  // Cronet URLRequest context used for all network operations.
  std::unique_ptr<CronetURLRequestContext> context_;
  // Signaled when |context_| initialization is done.
  base::WaitableEvent init_completed_;

  // Flag that indicates whether logging is in progress.
  bool is_logging_ = false;
  // Signaled when |StopNetLog| is done.
  base::WaitableEvent stop_netlog_completed_;

  // Storage path used by this engine.
  std::string in_use_storage_path_;

  DISALLOW_COPY_AND_ASSIGN(Cronet_EngineImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_ENGINE_H_
