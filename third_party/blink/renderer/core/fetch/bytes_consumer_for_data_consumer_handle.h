// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BYTES_CONSUMER_FOR_DATA_CONSUMER_HANDLE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BYTES_CONSUMER_FOR_DATA_CONSUMER_HANDLE_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_data_consumer_handle.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExecutionContext;

class CORE_EXPORT BytesConsumerForDataConsumerHandle final
    : public BytesConsumer,
      public WebDataConsumerHandle::Client {
  EAGERLY_FINALIZE();
  DECLARE_EAGER_FINALIZATION_OPERATOR_NEW();

 public:
  BytesConsumerForDataConsumerHandle(ExecutionContext*,
                                     std::unique_ptr<WebDataConsumerHandle>);
  ~BytesConsumerForDataConsumerHandle() override;

  Result BeginRead(const char** buffer, size_t* available) override;
  Result EndRead(size_t read_size) override;
  void SetClient(BytesConsumer::Client*) override;
  void ClearClient() override;

  void Cancel() override;
  PublicState GetPublicState() const override;
  Error GetError() const override {
    DCHECK(state_ == InternalState::kErrored);
    return error_;
  }
  String DebugName() const override {
    return "BytesConsumerForDataConsumerHandle";
  }

  // WebDataConsumerHandle::Client
  void DidGetReadable() override;

  void Trace(blink::Visitor*) override;

 private:
  void Close();
  void SetError();
  void Notify();

  Member<ExecutionContext> execution_context_;
  std::unique_ptr<WebDataConsumerHandle::Reader> reader_;
  Member<BytesConsumer::Client> client_;
  InternalState state_ = InternalState::kWaiting;
  Error error_;
  bool is_in_two_phase_read_ = false;
  bool has_pending_notification_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_BYTES_CONSUMER_FOR_DATA_CONSUMER_HANDLE_H_
