// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/extension_localization_peer.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "chrome/common/url_constants.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/message_bundle.h"
#include "ipc/ipc_sender.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace {

class StringData final : public content::RequestPeer::ReceivedData {
 public:
  explicit StringData(const std::string& data) : data_(data) {}

  const char* payload() const override { return data_.data(); }
  int length() const override { return data_.size(); }

 private:
  const std::string data_;

  DISALLOW_COPY_AND_ASSIGN(StringData);
};

}  // namespace

ExtensionLocalizationPeer::ExtensionLocalizationPeer(
    std::unique_ptr<content::RequestPeer> peer,
    IPC::Sender* message_sender,
    const GURL& request_url)
    : original_peer_(std::move(peer)),
      message_sender_(message_sender),
      request_url_(request_url) {}

ExtensionLocalizationPeer::~ExtensionLocalizationPeer() {
}

// static
std::unique_ptr<content::RequestPeer>
ExtensionLocalizationPeer::CreateExtensionLocalizationPeer(
    std::unique_ptr<content::RequestPeer> peer,
    IPC::Sender* message_sender,
    const std::string& mime_type,
    const GURL& request_url) {
  // Return the given |peer| as is if content is not text/css or it doesn't
  // belong to extension scheme.
  return (request_url.SchemeIs(extensions::kExtensionScheme) &&
          base::StartsWith(mime_type, "text/css",
                           base::CompareCase::INSENSITIVE_ASCII))
             ? base::WrapUnique(new ExtensionLocalizationPeer(
                   std::move(peer), message_sender, request_url))
             : std::move(peer);
}

void ExtensionLocalizationPeer::OnUploadProgress(uint64_t position,
                                                 uint64_t size) {
  NOTREACHED();
}

bool ExtensionLocalizationPeer::OnReceivedRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseInfo& info) {
  NOTREACHED();
  return false;
}

void ExtensionLocalizationPeer::OnReceivedResponse(
    const network::ResourceResponseInfo& info) {
  response_info_ = info;
}

void ExtensionLocalizationPeer::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  original_peer_->OnStartLoadingResponseBody(std::move(body));
}

void ExtensionLocalizationPeer::OnReceivedData(
    std::unique_ptr<ReceivedData> data) {
  data_.append(data->payload(), data->length());
}

void ExtensionLocalizationPeer::OnTransferSizeUpdated(int transfer_size_diff) {
  original_peer_->OnTransferSizeUpdated(transfer_size_diff);
}

void ExtensionLocalizationPeer::OnCompletedRequest(
    const network::URLLoaderCompletionStatus& status) {
  // Give sub-classes a chance at altering the data.
  if (status.error_code != net::OK) {
    // We failed to load the resource.
    original_peer_->OnReceivedResponse(response_info_);
    network::URLLoaderCompletionStatus aborted_status(status);
    aborted_status.error_code = net::ERR_ABORTED;
    original_peer_->OnCompletedRequest(aborted_status);
    return;
  }

  ReplaceMessages();

  original_peer_->OnReceivedResponse(response_info_);
  if (!data_.empty())
    original_peer_->OnReceivedData(std::make_unique<StringData>(data_));
  original_peer_->OnCompletedRequest(status);
}

void ExtensionLocalizationPeer::ReplaceMessages() {
  if (!message_sender_ || data_.empty())
    return;

  if (!request_url_.is_valid())
    return;

  std::string extension_id = request_url_.host();
  extensions::L10nMessagesMap* l10n_messages =
      extensions::GetL10nMessagesMap(extension_id);
  if (!l10n_messages) {
    extensions::L10nMessagesMap messages;
    message_sender_->Send(new ExtensionHostMsg_GetMessageBundle(
        extension_id, &messages));

    // Save messages we got, so we don't have to ask again.
    // Messages map is never empty, it contains at least @@extension_id value.
    extensions::ExtensionToL10nMessagesMap& l10n_messages_map =
        *extensions::GetExtensionToL10nMessagesMap();
    l10n_messages_map[extension_id] = messages;

    l10n_messages = extensions::GetL10nMessagesMap(extension_id);
  }

  std::string error;
  if (extensions::MessageBundle::ReplaceMessagesWithExternalDictionary(
          *l10n_messages, &data_, &error)) {
    data_.resize(data_.size());
  }
}
