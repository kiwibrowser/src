// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/blob_storage/blob_dispatcher_host.h"

#include <algorithm>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "content/browser/bad_message.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/common/fileapi/webblob_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_features.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "url/gurl.h"

using storage::BlobStorageContext;

namespace content {
namespace {
// These are used for UMA stats, don't change.
enum RefcountOperation {
  BDH_DECREMENT = 0,
  BDH_INCREMENT,
  BDH_TRACING_ENUM_LAST
};
} // namespace

BlobDispatcherHost::BlobDispatcherHost(
    int process_id,
    scoped_refptr<ChromeBlobStorageContext> blob_storage_context)
    : BrowserMessageFilter(BlobMsgStart),
      process_id_(process_id),
      blob_storage_context_(std::move(blob_storage_context)) {}

BlobDispatcherHost::~BlobDispatcherHost() {
  ClearHostFromBlobStorageContext();
}

void BlobDispatcherHost::OnChannelClosing() {
  ClearHostFromBlobStorageContext();
  public_blob_urls_.clear();
}

bool BlobDispatcherHost::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BlobDispatcherHost, message)
    IPC_MESSAGE_HANDLER(BlobHostMsg_RegisterPublicURL, OnRegisterPublicBlobURL)
    IPC_MESSAGE_HANDLER(BlobHostMsg_RevokePublicURL, OnRevokePublicBlobURL)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BlobDispatcherHost::OnRegisterPublicBlobURL(const GURL& public_url,
                                                 const std::string& uuid) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ChildProcessSecurityPolicyImpl* security_policy =
      ChildProcessSecurityPolicyImpl::GetInstance();

  // Blob urls have embedded origins. A frame should only be creating blob URLs
  // in the origin of its current document. Make sure that the origin advertised
  // on the URL is allowed to be rendered in this process.
  if (!public_url.SchemeIsBlob() ||
      !security_policy->CanCommitURL(process_id_, public_url)) {
    DVLOG(1) << "BlobDispatcherHost::OnRegisterPublicBlobURL("
             << public_url.spec() << ", " << uuid
             << "): Invalid or prohibited URL.";
    bad_message::ReceivedBadMessage(this, bad_message::BDH_DISALLOWED_ORIGIN);
    return;
  }
  if (uuid.empty()) {
    bad_message::ReceivedBadMessage(this,
                                    bad_message::BDH_INVALID_URL_OPERATION);
    return;
  }
  BlobStorageContext* context = this->context();
  if (context->registry().IsURLMapped(public_url)) {
    DVLOG(1) << "BlobDispatcherHost::OnRegisterPublicBlobURL("
             << public_url.spec() << ", " << uuid << "): Invalid url or uuid.";
    UMA_HISTOGRAM_ENUMERATION("Storage.Blob.InvalidURLRegister", BDH_INCREMENT,
                              BDH_TRACING_ENUM_LAST);
    return;
  }
  context->RegisterPublicBlobURL(public_url, uuid);
  public_blob_urls_.insert(public_url);
}

void BlobDispatcherHost::OnRevokePublicBlobURL(const GURL& public_url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!public_url.is_valid()) {
    bad_message::ReceivedBadMessage(this,
                                    bad_message::BDH_INVALID_URL_OPERATION);
    return;
  }
  if (!IsUrlRegisteredInHost(public_url)) {
    DVLOG(1) << "BlobDispatcherHost::OnRevokePublicBlobURL("
             << public_url.spec() << "): Unknown URL.";
    UMA_HISTOGRAM_ENUMERATION("Storage.Blob.InvalidURLRegister", BDH_DECREMENT,
                              BDH_TRACING_ENUM_LAST);
    return;
  }
  context()->RevokePublicBlobURL(public_url);
  public_blob_urls_.erase(public_url);
}

storage::BlobStorageContext* BlobDispatcherHost::context() {
  return blob_storage_context_->context();
}

bool BlobDispatcherHost::IsUrlRegisteredInHost(const GURL& blob_url) {
  return base::ContainsKey(public_blob_urls_, blob_url);
}

void BlobDispatcherHost::ClearHostFromBlobStorageContext() {
  BlobStorageContext* context = this->context();
  for (const auto& url : public_blob_urls_) {
    context->RevokePublicBlobURL(url);
  }
}

}  // namespace content
