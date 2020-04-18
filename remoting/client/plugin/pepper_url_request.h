// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_URL_LOADER_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_URL_LOADER_H_

#include "base/callback.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_request_info.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "remoting/base/url_request.h"

namespace remoting {

// UrlRequest implementation that uses URLLoader provided by Pepper.
class PepperUrlRequest : public UrlRequest {
 public:
  PepperUrlRequest(pp::InstanceHandle pp_instance,
                   UrlRequest::Type type,
                   const std::string& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~PepperUrlRequest() override;

  // UrlRequest interface.
  void AddHeader(const std::string& value) override;
  void SetPostData(const std::string& content_type,
                   const std::string& data) override;
  void Start(const OnResultCallback& on_result_callback) override;

 private:
  void OnUrlOpened(int32_t result);
  void ReadResponseBody();
  void OnResponseBodyRead(int32_t result);

  pp::URLRequestInfo request_info_;
  pp::URLLoader url_loader_;
  std::string url_;

  std::string headers_;

  OnResultCallback on_result_callback_;

  std::string response_;

  pp::CompletionCallbackFactory<PepperUrlRequest> callback_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperUrlRequest);
};

class PepperUrlRequestFactory : public UrlRequestFactory {
 public:
  PepperUrlRequestFactory(pp::InstanceHandle pp_instance);
  ~PepperUrlRequestFactory() override;

   // UrlRequestFactory interface.
  std::unique_ptr<UrlRequest> CreateUrlRequest(
      UrlRequest::Type type,
      const std::string& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) override;

 private:
  pp::InstanceHandle pp_instance_;

  DISALLOW_COPY_AND_ASSIGN(PepperUrlRequestFactory);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_URL_LOADER_H_
