// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_CROSS_THREAD_FUNCTIONAL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_CROSS_THREAD_FUNCTIONAL_H_

#include <type_traits>
#include "base/bind.h"
#include "third_party/blink/renderer/platform/cross_thread_copier.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

// CrossThreadBind() is Bind() for cross-thread task posting.
// CrossThreadBind() applies CrossThreadCopier to the arguments.
//
// Example:
//     void Func1(int, const String&);
//     f = CrossThreadBind(&Func1, 42, str);
// Func1(42, str2) will be called when |f()| is executed,
// where |str2| is a deep copy of |str| (created by str.IsolatedCopy()).
//
// CrossThreadBind(str) is similar to Bind(str.IsolatedCopy()), but the latter
// is NOT thread-safe due to temporary objects (https://crbug.com/390851).
//
// Don't (if you pass the task across threads):
//     Bind(&Func1, 42, str);
//     Bind(&Func1, 42, str.IsolatedCopy());

template <typename FunctionType, typename... Ps>
WTF::CrossThreadFunction<base::MakeUnboundRunType<FunctionType, Ps...>>
CrossThreadBind(FunctionType function, Ps&&... parameters) {
  static_assert(
      WTF::internal::CheckGCedTypeRestrictions<std::index_sequence_for<Ps...>,
                                               std::decay_t<Ps>...>::ok,
      "A bound argument uses a bad pattern.");
  using UnboundRunType = base::MakeUnboundRunType<FunctionType, Ps...>;
  return WTF::CrossThreadFunction<UnboundRunType>(
      base::Bind(function, CrossThreadCopier<std::decay_t<Ps>>::Copy(
                               std::forward<Ps>(parameters))...));
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_CROSS_THREAD_FUNCTIONAL_H_
