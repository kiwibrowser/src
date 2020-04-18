// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_FILTER_CHAIN_H_
#define MOJO_PUBLIC_CPP_BINDINGS_FILTER_CHAIN_H_

#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/bindings_export.h"
#include "mojo/public/cpp/bindings/message.h"

namespace mojo {

class MOJO_CPP_BINDINGS_EXPORT FilterChain : public MessageReceiver {
 public:
  // Doesn't take ownership of |sink|. Therefore |sink| has to stay alive while
  // this object is alive.
  explicit FilterChain(MessageReceiver* sink = nullptr);

  FilterChain(FilterChain&& other);
  FilterChain& operator=(FilterChain&& other);
  ~FilterChain() override;

  template <typename FilterType, typename... Args>
  inline void Append(Args&&... args);

  void Append(std::unique_ptr<MessageReceiver> filter);

  // Doesn't take ownership of |sink|. Therefore |sink| has to stay alive while
  // this object is alive.
  void SetSink(MessageReceiver* sink);

  // MessageReceiver:
  bool Accept(Message* message) override;

 private:
  std::vector<std::unique_ptr<MessageReceiver>> filters_;

  MessageReceiver* sink_;

  DISALLOW_COPY_AND_ASSIGN(FilterChain);
};

template <typename FilterType, typename... Args>
inline void FilterChain::Append(Args&&... args) {
  Append(std::make_unique<FilterType>(std::forward<Args>(args)...));
}

template <>
inline void FilterChain::Append<PassThroughFilter>() {
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_FILTER_CHAIN_H_
