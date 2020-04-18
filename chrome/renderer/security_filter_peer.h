// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_SECURITY_FILTER_PEER_H_
#define CHROME_RENDERER_SECURITY_FILTER_PEER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/common/resource_type.h"
#include "content/public/renderer/request_peer.h"
#include "services/network/public/cpp/resource_response_info.h"

// The SecurityFilterPeer is a proxy to a
// content::RequestPeer instance.  It is used to pre-process
// unsafe resources (such as mixed-content resource).
// Call the factory method CreateSecurityFilterPeer() to obtain an instance of
// SecurityFilterPeer based on the original Peer.
// A SecurityFilterPeer is created only when the associated request is rejected,
// which means content::RequestPeer methods other than OnCompletedRequest must
// not be called.
class SecurityFilterPeer final : public content::RequestPeer {
 public:
  ~SecurityFilterPeer() override;

  static std::unique_ptr<content::RequestPeer>
  CreateSecurityFilterPeerForDeniedRequest(
      content::ResourceType resource_type,
      std::unique_ptr<content::RequestPeer> peer,
      int os_error);

  // content::RequestPeer methods.
  void OnUploadProgress(uint64_t position, uint64_t size) override;
  bool OnReceivedRedirect(const net::RedirectInfo& redirect_info,
                          const network::ResourceResponseInfo& info) override;
  void OnReceivedResponse(const network::ResourceResponseInfo& info) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnDownloadedData(int len, int encoded_data_length) override;
  void OnReceivedData(std::unique_ptr<ReceivedData> data) override;
  void OnTransferSizeUpdated(int transfer_size_diff) override;
  void OnCompletedRequest(
      const network::URLLoaderCompletionStatus& status) override;

 private:
  SecurityFilterPeer(std::unique_ptr<content::RequestPeer> peer,
                     const std::string& mime_type,
                     const std::string& data);

  static scoped_refptr<net::HttpResponseHeaders> CreateHeaders(
      const std::string& mime_type);

  std::unique_ptr<content::RequestPeer> original_peer_;
  std::string mime_type_;
  std::string data_;

  DISALLOW_COPY_AND_ASSIGN(SecurityFilterPeer);
};

#endif  // CHROME_RENDERER_SECURITY_FILTER_PEER_H_
