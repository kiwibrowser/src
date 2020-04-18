/*
* Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTANCE_COUNTERS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTANCE_COUNTERS_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/atomics.h"

namespace blink {

#define INSTANCE_COUNTERS_LIST(V) \
  V(AudioHandler)                 \
  V(Document)                     \
  V(Frame)                        \
  V(JSEventListener)              \
  V(LayoutObject)                 \
  V(MediaKeySession)              \
  V(MediaKeys)                    \
  V(Node)                         \
  V(Resource)                     \
  V(ScriptPromise)                \
  V(PausableObject)               \
  V(V8PerContextData)             \
  V(WorkerGlobalScope)            \
  V(UACSSResource)                \
  V(RTCPeerConnection)            \
  V(ResourceFetcher)              \
  V(AdSubframe)                   \
  V(DetachedScriptState)

class InstanceCounters {
  STATIC_ONLY(InstanceCounters);

 public:
  enum CounterType {
#define DECLARE_INSTANCE_COUNTER(name) k##name##Counter,
    INSTANCE_COUNTERS_LIST(DECLARE_INSTANCE_COUNTER)
#undef DECLARE_INSTANCE_COUNTER
        kCounterTypeLength
  };

  static inline void IncrementCounter(CounterType type) {
    // There are lots of nodes created. Atomic barriers or locks
    // should be avoided for the sake of performance. See crbug.com/641019
    if (type == kNodeCounter) {
      DCHECK(IsMainThread());
      ++counters_[kNodeCounter];
    } else {
      AtomicIncrement(&counters_[type]);
    }
  }

  static inline void DecrementCounter(CounterType type) {
    if (type == kNodeCounter) {
      DCHECK(IsMainThread());
      --counters_[kNodeCounter];
    } else {
      AtomicDecrement(&counters_[type]);
    }
  }

  PLATFORM_EXPORT static int CounterValue(CounterType);

 private:
  PLATFORM_EXPORT static int counters_[];
};

}  // namespace blink

#endif  // !defined(InstanceCounters_h)
