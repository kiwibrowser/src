// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A delegate class of WebURLLoaderImpl that handles text/vnd.chromium.ftp-dir
// data.

#ifndef CONTENT_RENDERER_LOADER_FTP_DIRECTORY_LISTING_RESPONSE_DELEGATE_H_
#define CONTENT_RENDERER_LOADER_FTP_DIRECTORY_LISTING_RESPONSE_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "third_party/blink/public/platform/web_url_response.h"

namespace blink {
class WebURLLoader;
class WebURLLoaderClient;
}

class GURL;

namespace content {

class FtpDirectoryListingResponseDelegate {
 public:
  FtpDirectoryListingResponseDelegate(blink::WebURLLoaderClient* client,
                                      blink::WebURLLoader* loader,
                                      const blink::WebURLResponse& response);

  // The request has been canceled, so stop making calls to the client.
  void Cancel();

  // Passed through from ResourceHandleInternal
  void OnReceivedData(const char* data, int data_len);
  void OnCompletedRequest();

 private:
  void Init(const GURL& response_url);

  void SendDataToClient(const std::string& data);

  // Pointers to the client and associated loader so we can make callbacks as
  // we parse pieces of data.
  blink::WebURLLoaderClient* client_;
  blink::WebURLLoader* loader_;

  // Buffer for data received from the network.
  std::string buffer_;

  DISALLOW_COPY_AND_ASSIGN(FtpDirectoryListingResponseDelegate);
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_FTP_DIRECTORY_LISTING_RESPONSE_DELEGATE_H_
