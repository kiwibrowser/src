// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/url_response_body_consumer.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/macros.h"
#include "content/public/renderer/request_peer.h"
#include "content/renderer/loader/resource_dispatcher.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

constexpr uint32_t URLResponseBodyConsumer::kMaxNumConsumedBytesInTask;

class URLResponseBodyConsumer::ReceivedData final
    : public RequestPeer::ReceivedData {
 public:
  ReceivedData(const char* payload,
               int length,
               scoped_refptr<URLResponseBodyConsumer> consumer)
      : payload_(payload), length_(length), consumer_(consumer) {}

  ~ReceivedData() override { consumer_->Reclaim(length_); }

  const char* payload() const override { return payload_; }
  int length() const override { return length_; }

 private:
  const char* const payload_;
  const uint32_t length_;

  scoped_refptr<URLResponseBodyConsumer> consumer_;

  DISALLOW_COPY_AND_ASSIGN(ReceivedData);
};

URLResponseBodyConsumer::URLResponseBodyConsumer(
    int request_id,
    ResourceDispatcher* resource_dispatcher,
    mojo::ScopedDataPipeConsumerHandle handle,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : request_id_(request_id),
      resource_dispatcher_(resource_dispatcher),
      handle_(std::move(handle)),
      handle_watcher_(FROM_HERE,
                      mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                      task_runner),
      task_runner_(task_runner),
      has_seen_end_of_data_(!handle_.is_valid()) {
  handle_watcher_.Watch(
      handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&URLResponseBodyConsumer::OnReadable, base::Unretained(this)));
}

URLResponseBodyConsumer::~URLResponseBodyConsumer() {}

void URLResponseBodyConsumer::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  if (has_been_cancelled_)
    return;
  has_received_completion_ = true;
  status_ = status;
  NotifyCompletionIfAppropriate();
}

void URLResponseBodyConsumer::Cancel() {
  has_been_cancelled_ = true;
  handle_watcher_.Cancel();
}

void URLResponseBodyConsumer::SetDefersLoading() {
  is_deferred_ = true;
}

void URLResponseBodyConsumer::UnsetDefersLoading() {
  is_deferred_ = false;
  OnReadable(MOJO_RESULT_OK);
}

void URLResponseBodyConsumer::ArmOrNotify() {
  if (has_been_cancelled_)
    return;
  handle_watcher_.ArmOrNotify();
}

void URLResponseBodyConsumer::Reclaim(uint32_t size) {
  MojoResult result = handle_->EndReadData(size);
  DCHECK_EQ(MOJO_RESULT_OK, result);

  if (is_in_on_readable_)
    return;

  handle_watcher_.ArmOrNotify();
}

void URLResponseBodyConsumer::OnReadable(MojoResult unused) {
  if (has_been_cancelled_ || has_seen_end_of_data_ || is_deferred_)
    return;

  DCHECK(!is_in_on_readable_);
  uint32_t num_bytes_consumed = 0;

  // Protect |this| as RequestPeer::OnReceivedData may call deref.
  scoped_refptr<URLResponseBodyConsumer> protect(this);
  base::AutoReset<bool> is_in_on_readable(&is_in_on_readable_, true);

  while (!has_been_cancelled_ && !is_deferred_) {
    const void* buffer = nullptr;
    uint32_t available = 0;
    MojoResult result =
        handle_->BeginReadData(&buffer, &available, MOJO_READ_DATA_FLAG_NONE);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      handle_watcher_.ArmOrNotify();
      return;
    }
    if (result == MOJO_RESULT_BUSY)
      return;
    if (result == MOJO_RESULT_FAILED_PRECONDITION) {
      has_seen_end_of_data_ = true;
      NotifyCompletionIfAppropriate();
      return;
    }
    if (result != MOJO_RESULT_OK) {
      status_.error_code = net::ERR_FAILED;
      has_seen_end_of_data_ = true;
      has_received_completion_ = true;
      NotifyCompletionIfAppropriate();
      return;
    }
    DCHECK_LE(num_bytes_consumed, kMaxNumConsumedBytesInTask);
    available =
        std::min(available, kMaxNumConsumedBytesInTask - num_bytes_consumed);
    if (available == 0) {
      // We've already consumed many bytes in this task. Defer the remaining
      // to the next task.
      result = handle_->EndReadData(0);
      DCHECK_EQ(result, MOJO_RESULT_OK);
      handle_watcher_.ArmOrNotify();
      return;
    }
    num_bytes_consumed += available;
    ResourceDispatcher::PendingRequestInfo* request_info =
        resource_dispatcher_->GetPendingRequestInfo(request_id_);
    DCHECK(request_info);

    request_info->peer->OnReceivedData(std::make_unique<ReceivedData>(
        static_cast<const char*>(buffer), available, this));
  }
}

void URLResponseBodyConsumer::NotifyCompletionIfAppropriate() {
  if (has_been_cancelled_)
    return;
  if (!has_received_completion_ || !has_seen_end_of_data_)
    return;
  // Cancel this instance in order not to notify twice.
  Cancel();

  resource_dispatcher_->OnRequestComplete(request_id_, status_);
  // |this| may be deleted.
}

}  // namespace content
