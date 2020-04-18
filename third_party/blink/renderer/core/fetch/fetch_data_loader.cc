// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/fetch_data_loader.h"

#include <memory>
#include "mojo/public/cpp/system/simple_watcher.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer.h"
#include "third_party/blink/renderer/core/fetch/multipart_parser.h"
#include "third_party/blink/renderer/core/fileapi/file.h"
#include "third_party/blink/renderer/core/html/forms/form_data.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/platform/loader/fetch/text_resource_decoder_options.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/parsed_content_disposition.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/typed_arrays/array_buffer_builder.h"

namespace blink {

namespace {

class FetchDataLoaderAsBlobHandle final : public FetchDataLoader,
                                          public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(FetchDataLoaderAsBlobHandle);

 public:
  explicit FetchDataLoaderAsBlobHandle(const String& mime_type)
      : mime_type_(mime_type) {}

  void Start(BytesConsumer* consumer,
             FetchDataLoader::Client* client) override {
    DCHECK(!client_);
    DCHECK(!consumer_);

    client_ = client;
    consumer_ = consumer;

    scoped_refptr<BlobDataHandle> blob_handle =
        consumer_->DrainAsBlobDataHandle();
    if (blob_handle) {
      DCHECK_NE(UINT64_MAX, blob_handle->size());
      if (blob_handle->GetType() != mime_type_) {
        // A new Blob is created to override the Blob's type.
        auto blob_size = blob_handle->size();
        auto blob_data = BlobData::Create();
        blob_data->SetContentType(mime_type_);
        blob_data->AppendBlob(std::move(blob_handle), 0, blob_size);
        client_->DidFetchDataLoadedBlobHandle(
            BlobDataHandle::Create(std::move(blob_data), blob_size));
      } else {
        client_->DidFetchDataLoadedBlobHandle(std::move(blob_handle));
      }
      return;
    }

    blob_data_ = BlobData::Create();
    blob_data_->SetContentType(mime_type_);
    consumer_->SetClient(this);
    OnStateChange();
  }

  void Cancel() override { consumer_->Cancel(); }

  void OnStateChange() override {
    while (true) {
      const char* buffer;
      size_t available;
      auto result = consumer_->BeginRead(&buffer, &available);
      if (result == BytesConsumer::Result::kShouldWait)
        return;
      if (result == BytesConsumer::Result::kOk) {
        blob_data_->AppendBytes(buffer, available);
        result = consumer_->EndRead(available);
      }
      switch (result) {
        case BytesConsumer::Result::kOk:
          break;
        case BytesConsumer::Result::kShouldWait:
          NOTREACHED();
          return;
        case BytesConsumer::Result::kDone: {
          auto size = blob_data_->length();
          client_->DidFetchDataLoadedBlobHandle(
              BlobDataHandle::Create(std::move(blob_data_), size));
          return;
        }
        case BytesConsumer::Result::kError:
          client_->DidFetchDataLoadFailed();
          return;
      }
    }
  }

  String DebugName() const override { return "FetchDataLoaderAsBlobHandle"; }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(consumer_);
    visitor->Trace(client_);
    FetchDataLoader::Trace(visitor);
    BytesConsumer::Client::Trace(visitor);
  }

 private:
  Member<BytesConsumer> consumer_;
  Member<FetchDataLoader::Client> client_;

  String mime_type_;
  std::unique_ptr<BlobData> blob_data_;
};

class FetchDataLoaderAsArrayBuffer final : public FetchDataLoader,
                                           public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(FetchDataLoaderAsArrayBuffer)
 public:
  void Start(BytesConsumer* consumer,
             FetchDataLoader::Client* client) override {
    DCHECK(!client_);
    DCHECK(!raw_data_);
    DCHECK(!consumer_);
    client_ = client;
    raw_data_ = std::make_unique<ArrayBufferBuilder>();
    consumer_ = consumer;
    consumer_->SetClient(this);
    OnStateChange();
  }

  void Cancel() override { consumer_->Cancel(); }

  void OnStateChange() override {
    while (true) {
      const char* buffer;
      size_t available;
      auto result = consumer_->BeginRead(&buffer, &available);
      if (result == BytesConsumer::Result::kShouldWait)
        return;
      if (result == BytesConsumer::Result::kOk) {
        if (available > 0) {
          unsigned bytes_appended = raw_data_->Append(buffer, available);
          if (!bytes_appended) {
            auto unused = consumer_->EndRead(0);
            ALLOW_UNUSED_LOCAL(unused);
            consumer_->Cancel();
            client_->DidFetchDataLoadFailed();
            return;
          }
          DCHECK_EQ(bytes_appended, available);
        }
        result = consumer_->EndRead(available);
      }
      switch (result) {
        case BytesConsumer::Result::kOk:
          break;
        case BytesConsumer::Result::kShouldWait:
          NOTREACHED();
          return;
        case BytesConsumer::Result::kDone:
          client_->DidFetchDataLoadedArrayBuffer(
              DOMArrayBuffer::Create(raw_data_->ToArrayBuffer()));
          return;
        case BytesConsumer::Result::kError:
          client_->DidFetchDataLoadFailed();
          return;
      }
    }
  }

  String DebugName() const override { return "FetchDataLoaderAsArrayBuffer"; }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(consumer_);
    visitor->Trace(client_);
    FetchDataLoader::Trace(visitor);
    BytesConsumer::Client::Trace(visitor);
  }

 private:
  Member<BytesConsumer> consumer_;
  Member<FetchDataLoader::Client> client_;

  std::unique_ptr<ArrayBufferBuilder> raw_data_;
};

class FetchDataLoaderAsFailure final : public FetchDataLoader,
                                       public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(FetchDataLoaderAsFailure);

 public:
  void Start(BytesConsumer* consumer,
             FetchDataLoader::Client* client) override {
    DCHECK(!client_);
    DCHECK(!consumer_);
    client_ = client;
    consumer_ = consumer;
    consumer_->SetClient(this);
    OnStateChange();
  }

  void OnStateChange() override {
    while (true) {
      const char* buffer;
      size_t available;
      auto result = consumer_->BeginRead(&buffer, &available);
      if (result == BytesConsumer::Result::kShouldWait)
        return;
      if (result == BytesConsumer::Result::kOk)
        result = consumer_->EndRead(available);
      switch (result) {
        case BytesConsumer::Result::kOk:
          break;
        case BytesConsumer::Result::kShouldWait:
          NOTREACHED();
          return;
        case BytesConsumer::Result::kDone:
        case BytesConsumer::Result::kError:
          client_->DidFetchDataLoadFailed();
          return;
      }
    }
  }

  String DebugName() const override { return "FetchDataLoaderAsFailure"; }

  void Cancel() override { consumer_->Cancel(); }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(consumer_);
    visitor->Trace(client_);
    FetchDataLoader::Trace(visitor);
    BytesConsumer::Client::Trace(visitor);
  }

 private:
  Member<BytesConsumer> consumer_;
  Member<FetchDataLoader::Client> client_;
};

class FetchDataLoaderAsFormData final : public FetchDataLoader,
                                        public BytesConsumer::Client,
                                        public MultipartParser::Client {
  USING_GARBAGE_COLLECTED_MIXIN(FetchDataLoaderAsFormData);

 public:
  explicit FetchDataLoaderAsFormData(const String& multipart_boundary)
      : multipart_boundary_(multipart_boundary) {}

  void Start(BytesConsumer* consumer,
             FetchDataLoader::Client* client) override {
    DCHECK(!client_);
    DCHECK(!consumer_);
    DCHECK(!form_data_);
    DCHECK(!multipart_parser_);

    const CString multipart_boundary_utf8 = multipart_boundary_.Utf8();
    Vector<char> multipart_boundary_vector;
    multipart_boundary_vector.Append(multipart_boundary_utf8.data(),
                                     multipart_boundary_utf8.length());

    client_ = client;
    form_data_ = FormData::Create();
    multipart_parser_ =
        new MultipartParser(std::move(multipart_boundary_vector), this);
    consumer_ = consumer;
    consumer_->SetClient(this);
    OnStateChange();
  }

  void OnStateChange() override {
    while (true) {
      const char* buffer;
      size_t available;
      auto result = consumer_->BeginRead(&buffer, &available);
      if (result == BytesConsumer::Result::kShouldWait)
        return;
      if (result == BytesConsumer::Result::kOk) {
        const bool buffer_appended =
            multipart_parser_->AppendData(buffer, available);
        const bool multipart_receive_failed = multipart_parser_->IsCancelled();
        result = consumer_->EndRead(available);
        if (!buffer_appended || multipart_receive_failed)
          result = BytesConsumer::Result::kError;
      }
      switch (result) {
        case BytesConsumer::Result::kOk:
          break;
        case BytesConsumer::Result::kShouldWait:
          NOTREACHED();
          return;
        case BytesConsumer::Result::kDone:
          if (multipart_parser_->Finish()) {
            DCHECK(!multipart_parser_->IsCancelled());
            client_->DidFetchDataLoadedFormData(form_data_);
          } else {
            client_->DidFetchDataLoadFailed();
          }
          return;
        case BytesConsumer::Result::kError:
          client_->DidFetchDataLoadFailed();
          return;
      }
    }
  }

  String DebugName() const override { return "FetchDataLoaderAsFormData"; }

  void Cancel() override {
    consumer_->Cancel();
    multipart_parser_->Cancel();
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(consumer_);
    visitor->Trace(client_);
    visitor->Trace(form_data_);
    visitor->Trace(multipart_parser_);
    FetchDataLoader::Trace(visitor);
    BytesConsumer::Client::Trace(visitor);
    MultipartParser::Client::Trace(visitor);
  }

 private:
  void PartHeaderFieldsInMultipartReceived(
      const HTTPHeaderMap& header_fields) override {
    if (!current_entry_.Initialize(header_fields))
      multipart_parser_->Cancel();
  }

  void PartDataInMultipartReceived(const char* bytes, size_t size) override {
    if (!current_entry_.AppendBytes(bytes, size))
      multipart_parser_->Cancel();
  }

  void PartDataInMultipartFullyReceived() override {
    if (!current_entry_.Finish(form_data_))
      multipart_parser_->Cancel();
  }

  class Entry {
   public:
    bool Initialize(const HTTPHeaderMap& header_fields) {
      const ParsedContentDisposition disposition(
          header_fields.Get(HTTPNames::Content_Disposition));
      const String disposition_type = disposition.Type();
      filename_ = disposition.Filename();
      name_ = disposition.ParameterValueForName("name");
      blob_data_.reset();
      string_builder_.reset();
      if (disposition_type != "form-data" || name_.IsNull())
        return false;
      if (!filename_.IsNull()) {
        blob_data_ = BlobData::Create();
        const AtomicString& content_type =
            header_fields.Get(HTTPNames::Content_Type);
        blob_data_->SetContentType(content_type.IsNull() ? "text/plain"
                                                         : content_type);
      } else {
        if (!string_decoder_) {
          string_decoder_ = TextResourceDecoder::Create(
              TextResourceDecoderOptions::CreateAlwaysUseUTF8ForText());
        }
        string_builder_.reset(new StringBuilder);
      }
      return true;
    }

    bool AppendBytes(const char* bytes, size_t size) {
      if (blob_data_)
        blob_data_->AppendBytes(bytes, size);
      if (string_builder_) {
        string_builder_->Append(string_decoder_->Decode(bytes, size));
        if (string_decoder_->SawError())
          return false;
      }
      return true;
    }

    bool Finish(FormData* form_data) {
      if (blob_data_) {
        DCHECK(!string_builder_);
        const auto size = blob_data_->length();
        File* file =
            File::Create(filename_, InvalidFileTime(),
                         BlobDataHandle::Create(std::move(blob_data_), size));
        form_data->append(name_, file, filename_);
        return true;
      }
      DCHECK(!blob_data_);
      DCHECK(string_builder_);
      string_builder_->Append(string_decoder_->Flush());
      if (string_decoder_->SawError())
        return false;
      form_data->append(name_, string_builder_->ToString());
      return true;
    }

   private:
    std::unique_ptr<BlobData> blob_data_;
    String filename_;
    String name_;
    std::unique_ptr<StringBuilder> string_builder_;
    std::unique_ptr<TextResourceDecoder> string_decoder_;
  };

  Member<BytesConsumer> consumer_;
  Member<FetchDataLoader::Client> client_;
  Member<FormData> form_data_;
  Member<MultipartParser> multipart_parser_;

  Entry current_entry_;
  String multipart_boundary_;
};

class FetchDataLoaderAsString final : public FetchDataLoader,
                                      public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(FetchDataLoaderAsString);

 public:
  void Start(BytesConsumer* consumer,
             FetchDataLoader::Client* client) override {
    DCHECK(!client_);
    DCHECK(!decoder_);
    DCHECK(!consumer_);
    client_ = client;
    decoder_ = TextResourceDecoder::Create(
        TextResourceDecoderOptions::CreateAlwaysUseUTF8ForText());
    consumer_ = consumer;
    consumer_->SetClient(this);
    OnStateChange();
  }

  void OnStateChange() override {
    while (true) {
      const char* buffer;
      size_t available;
      auto result = consumer_->BeginRead(&buffer, &available);
      if (result == BytesConsumer::Result::kShouldWait)
        return;
      if (result == BytesConsumer::Result::kOk) {
        if (available > 0)
          builder_.Append(decoder_->Decode(buffer, available));
        result = consumer_->EndRead(available);
      }
      switch (result) {
        case BytesConsumer::Result::kOk:
          break;
        case BytesConsumer::Result::kShouldWait:
          NOTREACHED();
          return;
        case BytesConsumer::Result::kDone:
          builder_.Append(decoder_->Flush());
          client_->DidFetchDataLoadedString(builder_.ToString());
          return;
        case BytesConsumer::Result::kError:
          client_->DidFetchDataLoadFailed();
          return;
      }
    }
  }

  String DebugName() const override { return "FetchDataLoaderAsString"; }

  void Cancel() override { consumer_->Cancel(); }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(consumer_);
    visitor->Trace(client_);
    FetchDataLoader::Trace(visitor);
    BytesConsumer::Client::Trace(visitor);
  }

 private:
  Member<BytesConsumer> consumer_;
  Member<FetchDataLoader::Client> client_;

  std::unique_ptr<TextResourceDecoder> decoder_;
  StringBuilder builder_;
};

class FetchDataLoaderAsDataPipe final : public FetchDataLoader,
                                        public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(FetchDataLoaderAsDataPipe);

 public:
  FetchDataLoaderAsDataPipe(
      mojo::ScopedDataPipeProducerHandle out_data_pipe,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : out_data_pipe_(std::move(out_data_pipe)),
        data_pipe_watcher_(FROM_HERE,
                           mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                           std::move(task_runner)) {}
  ~FetchDataLoaderAsDataPipe() override {}

  void Start(BytesConsumer* consumer,
             FetchDataLoader::Client* client) override {
    DCHECK(!client_);
    DCHECK(!consumer_);
    data_pipe_watcher_.Watch(
        out_data_pipe_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
        WTF::BindRepeating(&FetchDataLoaderAsDataPipe::OnWritable,
                           WrapWeakPersistent(this)));
    data_pipe_watcher_.ArmOrNotify();
    client_ = client;
    consumer_ = consumer;
    consumer_->SetClient(this);
  }

  void OnWritable(MojoResult) { OnStateChange(); }

  // Implements BytesConsumer::Client.
  void OnStateChange() override {
    bool should_wait = false;
    while (!should_wait) {
      const char* buffer;
      size_t available;
      auto result = consumer_->BeginRead(&buffer, &available);
      if (result == BytesConsumer::Result::kShouldWait)
        return;
      if (result == BytesConsumer::Result::kOk) {
        DCHECK_GT(available, 0UL);
        uint32_t num_bytes = available;
        MojoResult mojo_result = out_data_pipe_->WriteData(
            buffer, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE);
        if (mojo_result == MOJO_RESULT_OK) {
          result = consumer_->EndRead(num_bytes);
        } else if (mojo_result == MOJO_RESULT_SHOULD_WAIT) {
          result = consumer_->EndRead(0);
          should_wait = true;
          data_pipe_watcher_.ArmOrNotify();
        } else {
          result = consumer_->EndRead(0);
          StopInternal();
          client_->DidFetchDataLoadFailed();
          return;
        }
      }
      switch (result) {
        case BytesConsumer::Result::kOk:
          break;
        case BytesConsumer::Result::kShouldWait:
          NOTREACHED();
          return;
        case BytesConsumer::Result::kDone:
          StopInternal();
          client_->DidFetchDataLoadedDataPipe();
          return;
        case BytesConsumer::Result::kError:
          StopInternal();
          client_->DidFetchDataLoadFailed();
          return;
      }
    }
  }

  String DebugName() const override { return "FetchDataLoaderAsDataPipe"; }

  void Cancel() override { StopInternal(); }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(consumer_);
    visitor->Trace(client_);
    FetchDataLoader::Trace(visitor);
    BytesConsumer::Client::Trace(visitor);
  }

 private:
  void StopInternal() {
    consumer_->Cancel();
    data_pipe_watcher_.Cancel();
    out_data_pipe_.reset();
  }

  Member<BytesConsumer> consumer_;
  Member<FetchDataLoader::Client> client_;

  mojo::ScopedDataPipeProducerHandle out_data_pipe_;
  mojo::SimpleWatcher data_pipe_watcher_;
};

}  // namespace

FetchDataLoader* FetchDataLoader::CreateLoaderAsBlobHandle(
    const String& mime_type) {
  return new FetchDataLoaderAsBlobHandle(mime_type);
}

FetchDataLoader* FetchDataLoader::CreateLoaderAsArrayBuffer() {
  return new FetchDataLoaderAsArrayBuffer();
}

FetchDataLoader* FetchDataLoader::CreateLoaderAsFailure() {
  return new FetchDataLoaderAsFailure();
}

FetchDataLoader* FetchDataLoader::CreateLoaderAsFormData(
    const String& multipartBoundary) {
  return new FetchDataLoaderAsFormData(multipartBoundary);
}

FetchDataLoader* FetchDataLoader::CreateLoaderAsString() {
  return new FetchDataLoaderAsString();
}

FetchDataLoader* FetchDataLoader::CreateLoaderAsDataPipe(
    mojo::ScopedDataPipeProducerHandle out_data_pipe,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  return new FetchDataLoaderAsDataPipe(std::move(out_data_pipe),
                                       std::move(task_runner));
}

}  // namespace blink
