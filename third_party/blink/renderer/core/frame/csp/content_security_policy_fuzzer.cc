// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"

#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/testing/blink_fuzzer_test_support.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// Intentionally leaked during fuzzing.
// See testing/libfuzzer/efficient_fuzzer.md.
DummyPageHolder* g_page_holder = nullptr;

int LLVMFuzzerInitialize(int* argc, char*** argv) {
  static BlinkFuzzerTestSupport test_support = BlinkFuzzerTestSupport();
  // Scope cannot be created before BlinkFuzzerTestSupport because it requires
  // that Oilpan be initialized to access blink::ThreadState::Current.
  LEAK_SANITIZER_DISABLED_SCOPE;
  g_page_holder = DummyPageHolder::Create().release();
  return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  String header = String::FromUTF8(data, size);
  unsigned hash = header.IsNull() ? 0 : header.Impl()->GetHash();

  // Construct and initialize a policy from the string.
  ContentSecurityPolicy* csp = ContentSecurityPolicy::Create();
  csp->DidReceiveHeader(header,
                        hash & 0x01 ? kContentSecurityPolicyHeaderTypeEnforce
                                    : kContentSecurityPolicyHeaderTypeReport,
                        hash & 0x02 ? kContentSecurityPolicyHeaderSourceHTTP
                                    : kContentSecurityPolicyHeaderSourceMeta);
  g_page_holder->GetDocument().InitContentSecurityPolicy(csp);

  // Force a garbage collection.
  // Specify namespace explicitly. Otherwise it conflicts on Mac OS X with:
  // CoreServices.framework/Frameworks/CarbonCore.framework/Headers/Threads.h.
  blink::ThreadState::Current()->CollectGarbage(
      BlinkGC::kNoHeapPointersOnStack, BlinkGC::kAtomicMarking,
      BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);

  return 0;
}

}  // namespace blink

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  return blink::LLVMFuzzerInitialize(argc, argv);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return blink::LLVMFuzzerTestOneInput(data, size);
}
