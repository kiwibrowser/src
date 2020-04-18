/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/fileapi/blob.h"

#include <memory>
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/blob_property_bag.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/url/dom_url.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/blob/blob_registry.h"
#include "third_party/blink/renderer/platform/blob/blob_url.h"

namespace blink {

class BlobURLRegistry final : public URLRegistry {
 public:
  // SecurityOrigin is passed together with KURL so that the registry can
  // save it for entries from whose KURL the origin is not recoverable by
  // using BlobURL::getOrigin().
  void RegisterURL(SecurityOrigin*, const KURL&, URLRegistrable*) override;
  void UnregisterURL(const KURL&) override;

  static URLRegistry& Registry();
};

void BlobURLRegistry::RegisterURL(SecurityOrigin* origin,
                                  const KURL& public_url,
                                  URLRegistrable* registrable_object) {
  DCHECK_EQ(&registrable_object->Registry(), this);
  Blob* blob = static_cast<Blob*>(registrable_object);
  BlobRegistry::RegisterPublicBlobURL(origin, public_url,
                                      blob->GetBlobDataHandle());
}

void BlobURLRegistry::UnregisterURL(const KURL& public_url) {
  BlobRegistry::RevokePublicBlobURL(public_url);
}

URLRegistry& BlobURLRegistry::Registry() {
  // This is called on multiple threads.
  // (This code assumes it is safe to register or unregister URLs on
  // BlobURLRegistry (that is implemented by the embedder) on
  // multiple threads.)
  DEFINE_THREAD_SAFE_STATIC_LOCAL(BlobURLRegistry, instance, ());
  return instance;
}

Blob::Blob(scoped_refptr<BlobDataHandle> data_handle)
    : blob_data_handle_(std::move(data_handle)) {}

Blob::~Blob() = default;

// static
Blob* Blob::Create(
    ExecutionContext* context,
    const HeapVector<ArrayBufferOrArrayBufferViewOrBlobOrUSVString>& blob_parts,
    const BlobPropertyBag& options,
    ExceptionState& exception_state) {
  DCHECK(options.hasType());

  DCHECK(options.hasEndings());
  bool normalize_line_endings_to_native = options.endings() == "native";
  if (normalize_line_endings_to_native)
    UseCounter::Count(context, WebFeature::kFileAPINativeLineEndings);

  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->SetContentType(NormalizeType(options.type()));

  PopulateBlobData(blob_data.get(), blob_parts,
                   normalize_line_endings_to_native);

  long long blob_size = blob_data->length();
  return new Blob(BlobDataHandle::Create(std::move(blob_data), blob_size));
}

Blob* Blob::Create(const unsigned char* data,
                   size_t bytes,
                   const String& content_type) {
  DCHECK(data);

  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->SetContentType(content_type);
  blob_data->AppendBytes(data, bytes);
  long long blob_size = blob_data->length();

  return new Blob(BlobDataHandle::Create(std::move(blob_data), blob_size));
}

// static
void Blob::PopulateBlobData(
    BlobData* blob_data,
    const HeapVector<ArrayBufferOrArrayBufferViewOrBlobOrUSVString>& parts,
    bool normalize_line_endings_to_native) {
  for (const auto& item : parts) {
    if (item.IsArrayBuffer()) {
      DOMArrayBuffer* array_buffer = item.GetAsArrayBuffer();
      blob_data->AppendBytes(array_buffer->Data(), array_buffer->ByteLength());
    } else if (item.IsArrayBufferView()) {
      DOMArrayBufferView* array_buffer_view =
          item.GetAsArrayBufferView().View();
      blob_data->AppendBytes(array_buffer_view->BaseAddress(),
                             array_buffer_view->byteLength());
    } else if (item.IsBlob()) {
      item.GetAsBlob()->AppendTo(*blob_data);
    } else if (item.IsUSVString()) {
      blob_data->AppendText(item.GetAsUSVString(),
                            normalize_line_endings_to_native);
    } else {
      NOTREACHED();
    }
  }
}

// static
void Blob::ClampSliceOffsets(long long size, long long& start, long long& end) {
  DCHECK_NE(size, -1);

  // Convert the negative value that is used to select from the end.
  if (start < 0)
    start = start + size;
  if (end < 0)
    end = end + size;

  // Clamp the range if it exceeds the size limit.
  if (start < 0)
    start = 0;
  if (end < 0)
    end = 0;
  if (start >= size) {
    start = 0;
    end = 0;
  } else if (end < start) {
    end = start;
  } else if (end > size) {
    end = size;
  }
}

Blob* Blob::slice(long long start,
                  long long end,
                  const String& content_type,
                  ExceptionState& exception_state) const {
  long long size = this->size();
  ClampSliceOffsets(size, start, end);

  long long length = end - start;
  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->SetContentType(NormalizeType(content_type));
  blob_data->AppendBlob(blob_data_handle_, start, length);
  return Blob::Create(BlobDataHandle::Create(std::move(blob_data), length));
}

void Blob::AppendTo(BlobData& blob_data) const {
  blob_data.AppendBlob(blob_data_handle_, 0, blob_data_handle_->size());
}

URLRegistry& Blob::Registry() const {
  return BlobURLRegistry::Registry();
}

mojom::blink::BlobPtr Blob::AsMojoBlob() {
  return blob_data_handle_->CloneBlobPtr();
}

// static
String Blob::NormalizeType(const String& type) {
  if (type.IsNull())
    return g_empty_string;
  const size_t length = type.length();
  if (length > 65535)
    return g_empty_string;
  if (type.Is8Bit()) {
    const LChar* chars = type.Characters8();
    for (size_t i = 0; i < length; ++i) {
      if (chars[i] < 0x20 || chars[i] > 0x7e)
        return g_empty_string;
    }
  } else {
    const UChar* chars = type.Characters16();
    for (size_t i = 0; i < length; ++i) {
      if (chars[i] < 0x0020 || chars[i] > 0x007e)
        return g_empty_string;
    }
  }
  return type.DeprecatedLower();
}

}  // namespace blink
