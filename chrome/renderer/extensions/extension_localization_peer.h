// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_EXTENSION_LOCALIZATION_PEER_H_
#define CHROME_RENDERER_EXTENSIONS_EXTENSION_LOCALIZATION_PEER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/renderer/request_peer.h"
#include "services/network/public/cpp/resource_response_info.h"

namespace IPC {
class Sender;
}

// The ExtensionLocalizationPeer is a proxy to a
// content::RequestPeer instance.  It is used to pre-process
// CSS files requested by extensions to replace localization templates with the
// appropriate localized strings.
//
// Call the factory method CreateExtensionLocalizationPeer() to obtain an
// instance of ExtensionLocalizationPeer based on the original Peer.
class ExtensionLocalizationPeer : public content::RequestPeer {
 public:
  ~ExtensionLocalizationPeer() override;

  static std::unique_ptr<content::RequestPeer> CreateExtensionLocalizationPeer(
      std::unique_ptr<content::RequestPeer> peer,
      IPC::Sender* message_sender,
      const std::string& mime_type,
      const GURL& request_url);

  // content::RequestPeer methods.
  void OnUploadProgress(uint64_t position, uint64_t size) override;
  bool OnReceivedRedirect(const net::RedirectInfo& redirect_info,
                          const network::ResourceResponseInfo& info) override;
  void OnReceivedResponse(const network::ResourceResponseInfo& info) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnDownloadedData(int len, int encoded_data_length) override {}
  void OnReceivedData(std::unique_ptr<ReceivedData> data) override;
  void OnTransferSizeUpdated(int transfer_size_diff) override;
  void OnCompletedRequest(
      const network::URLLoaderCompletionStatus& status) override;

 private:
  friend class ExtensionLocalizationPeerTest;

  // Use CreateExtensionLocalizationPeer to create an instance.
  ExtensionLocalizationPeer(std::unique_ptr<content::RequestPeer> peer,
                            IPC::Sender* message_sender,
                            const GURL& request_url);

  // Loads message catalogs, and replaces all __MSG_some_name__ templates within
  // loaded file.
  void ReplaceMessages();

  // Original peer that handles the request once we are done processing data_.
  std::unique_ptr<content::RequestPeer> original_peer_;

  // We just pass though the response info. This holds the copy of the original.
  network::ResourceResponseInfo response_info_;

  // Sends ExtensionHostMsg_GetMessageBundle message to the browser to fetch
  // message catalog.
  IPC::Sender* message_sender_;

  // Buffer for incoming data. We wait until OnCompletedRequest before using it.
  std::string data_;

  // Original request URL.
  GURL request_url_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionLocalizationPeer);
};

#endif  // CHROME_RENDERER_EXTENSIONS_EXTENSION_LOCALIZATION_PEER_H_
