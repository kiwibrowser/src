/*
 * Copyright (C) 2008, 2010 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "third_party/blink/renderer/platform/wtf/wtf_thread_data.h"

#include "third_party/blink/renderer/platform/wtf/stack_util.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_table.h"
#include "third_party/blink/renderer/platform/wtf/text/text_codec_icu.h"

namespace WTF {

ThreadSpecific<WTFThreadData>* WTFThreadData::static_data_;

WTFThreadData::WTFThreadData()
    : atomic_string_table_(new AtomicStringTable),
      cached_converter_icu_(new ICUConverterWrapper),
      thread_id_(internal::CurrentThreadSyscall()) {}

WTFThreadData::~WTFThreadData() = default;

void WTFThreadData::Initialize() {
  DCHECK(!WTFThreadData::static_data_);
  WTFThreadData::static_data_ = new ThreadSpecific<WTFThreadData>;
  WtfThreadData();
}

#if defined(OS_WIN) && defined(COMPILER_MSVC)
size_t WTFThreadData::ThreadStackSize() {
  // Needed to bootstrap WTFThreadData on Windows, because this value is needed
  // before the main thread data is fully initialized.
  if (!WTFThreadData::static_data_->IsSet())
    return internal::ThreadStackSize();

  WTFThreadData& data = WtfThreadData();
  if (!data.thread_stack_size_)
    data.thread_stack_size_ = internal::ThreadStackSize();
  return data.thread_stack_size_;
}
#endif

}  // namespace WTF
