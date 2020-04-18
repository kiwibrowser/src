// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/wtf/text/text_codec.h"

#include "third_party/blink/renderer/platform/testing/blink_fuzzer_test_support.h"
#include "third_party/blink/renderer/platform/testing/fuzzed_data_provider.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding_registry.h"

using namespace blink;

// TODO(jsbell): This fuzzes code in wtf/ but has dependencies on platform/,
// so it must live in the latter directory. Once wtf/ moves into platform/wtf
// this should move there as well.

WTF::FlushBehavior kFlushBehavior[] = {WTF::kDoNotFlush, WTF::kFetchEOF,
                                       WTF::kDataEOF};

WTF::UnencodableHandling kUnencodableHandlingOptions[] = {
    WTF::kEntitiesForUnencodables, WTF::kURLEncodedEntitiesForUnencodables,
    WTF::kCSSEncodedEntitiesForUnencodables};

class TextCodecFuzzHarness {};

// Fuzzer for WTF::TextCodec.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static BlinkFuzzerTestSupport test_support = BlinkFuzzerTestSupport();
  // The fuzzer picks 3 bytes off the end of the data to initialize metadata, so
  // abort if the input is smaller than that.
  if (size < 3)
    return 0;

  // TODO(csharrison): When crbug.com/701825 is resolved, add the rest of the
  // text codecs.

  // Initializes the codec map.
  static const WTF::TextEncoding encoding = WTF::TextEncoding(
#if defined(UTF_8)
      "UTF-8"
#elif defined(WINDOWS_1252)
      "windows-1252"
#endif
      "");

  FuzzedDataProvider fuzzedData(data, size);

  // Initialize metadata using the fuzzed data.
  bool stopOnError = fuzzedData.ConsumeBool();
  WTF::UnencodableHandling unencodableHandling =
      fuzzedData.PickValueInArray(kUnencodableHandlingOptions);
  WTF::FlushBehavior flushBehavior =
      fuzzedData.PickValueInArray(kFlushBehavior);

  // Now, use the rest of the fuzzy data to stress test decoding and encoding.
  const CString byteString = fuzzedData.ConsumeRemainingBytes();
  std::unique_ptr<TextCodec> codec = NewTextCodec(encoding);

  // Treat as bytes-off-the-wire.
  bool sawError;
  const String decoded = codec->Decode(byteString.data(), byteString.length(),
                                       flushBehavior, stopOnError, sawError);

  // Treat as blink 8-bit string (latin1).
  if (size % sizeof(LChar) == 0) {
    std::unique_ptr<TextCodec> codec = NewTextCodec(encoding);
    codec->Encode(reinterpret_cast<const LChar*>(byteString.data()),
                  byteString.length() / sizeof(LChar), unencodableHandling);
  }

  // Treat as blink 16-bit string (utf-16) if there are an even number of bytes.
  if (size % sizeof(UChar) == 0) {
    std::unique_ptr<TextCodec> codec = NewTextCodec(encoding);
    codec->Encode(reinterpret_cast<const UChar*>(byteString.data()),
                  byteString.length() / sizeof(UChar), unencodableHandling);
  }

  if (decoded.IsNull())
    return 0;

  // Round trip the bytes (aka encode the decoded bytes).
  if (decoded.Is8Bit()) {
    codec->Encode(decoded.Characters8(), decoded.length(), unencodableHandling);
  } else {
    codec->Encode(decoded.Characters16(), decoded.length(),
                  unencodableHandling);
  }
  return 0;
}
