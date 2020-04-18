// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_
#define COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "components/cronet/cronet_url_request.h"
#include "components/cronet/cronet_url_request_context.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

namespace cronet {

class Cronet_EngineImpl;

// Implementation of Cronet_UrlRequest that uses CronetURLRequestContext.
class Cronet_UrlRequestImpl : public Cronet_UrlRequest {
 public:
  Cronet_UrlRequestImpl();
  ~Cronet_UrlRequestImpl() override;

  // Cronet_UrlRequest
  Cronet_RESULT InitWithParams(Cronet_EnginePtr engine,
                               Cronet_String url,
                               Cronet_UrlRequestParamsPtr params,
                               Cronet_UrlRequestCallbackPtr callback,
                               Cronet_ExecutorPtr executor) override;
  Cronet_RESULT Start() override;
  Cronet_RESULT FollowRedirect() override;
  Cronet_RESULT Read(Cronet_BufferPtr buffer) override;
  void Cancel() override;
  bool IsDone() override;
  void GetStatus(Cronet_UrlRequestStatusListenerPtr listener) override;

 private:
  class Callback;

  // Return |true| if request has started and is now done.
  // Must be called under |lock_| held.
  bool IsDoneLocked();

  // Helper method to set final status of CronetUrlRequest and clean up the
  // native request adapter. Returns true if request is already done, false
  // request is not done and is destroyed.
  bool DestroyRequestUnlessDone(
      Cronet_RequestFinishedInfo_FINISHED_REASON finished_reason);

  // Helper method to set final status of CronetUrlRequest and clean up the
  // native request adapter. Returns true if request is already done, false
  // request is not done and is destroyed. Must be called under |lock_| held.
  bool DestroyRequestUnlessDoneLocked(
      Cronet_RequestFinishedInfo_FINISHED_REASON finished_reason);

  // Synchronize access to |request_| and other objects below from different
  // threads.
  base::Lock lock_;
  // Cronet URLRequest used for this operation.
  CronetURLRequest* request_ = nullptr;
  bool started_ = false;
  bool waiting_on_redirect_ = false;
  bool waiting_on_read_ = false;

  // Response info updated by callback with number of bytes received. May be
  // nullptr, if no response has been received.
  std::unique_ptr<Cronet_UrlResponseInfo> response_info_;
  // The error reported by request. May be nullptr if no error has occurred.
  std::unique_ptr<Cronet_Error> error_;

  // Cronet Engine used to run network operations. Not owned, accessed from
  // client thread. Must outlive this request.
  Cronet_EngineImpl* engine_;

  DISALLOW_COPY_AND_ASSIGN(Cronet_UrlRequestImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_
