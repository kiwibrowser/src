// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_HTTP_ICE_CONFIG_REQUEST_H_
#define REMOTING_PROTOCOL_HTTP_ICE_CONFIG_REQUEST_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "remoting/base/oauth_token_getter.h"
#include "remoting/base/url_request.h"
#include "remoting/protocol/ice_config_request.h"

namespace remoting {
namespace protocol {

// IceConfigRequest that fetches IceConfig from using HTTP. If the config has
// been fetched succesfully but some parts couldn't be parsed then the returned
// config contains all entries that were parsed successfully and
// |expiration_time| is set to Now, i.e. the config is considered expired.
class HttpIceConfigRequest : public IceConfigRequest {
 public:
  HttpIceConfigRequest(UrlRequestFactory* url_request_factory,
                       const std::string& url,
                       OAuthTokenGetter* oauth_token_getter);
  ~HttpIceConfigRequest() override;

  // IceConfigRequest interface.
  void Send(const OnIceConfigCallback& callback) override;

 private:
  void OnOAuthToken(OAuthTokenGetter::Status status,
                    const std::string& user_email,
                    const std::string& access_token);

  void SendRequest();

  void OnResponse(const UrlRequest::Result& result);

  std::string url_;
  OAuthTokenGetter* oauth_token_getter_;
  std::unique_ptr<UrlRequest> url_request_;
  OnIceConfigCallback on_ice_config_callback_;

  base::WeakPtrFactory<HttpIceConfigRequest> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HttpIceConfigRequest);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_HTTP_ICE_CONFIG_REQUEST_H_
