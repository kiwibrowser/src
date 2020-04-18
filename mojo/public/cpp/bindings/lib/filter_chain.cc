// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/filter_chain.h"

#include <algorithm>

#include "base/logging.h"

namespace mojo {

FilterChain::FilterChain(MessageReceiver* sink) : sink_(sink) {
}

FilterChain::FilterChain(FilterChain&& other) : sink_(other.sink_) {
  other.sink_ = nullptr;
  filters_.swap(other.filters_);
}

FilterChain& FilterChain::operator=(FilterChain&& other) {
  std::swap(sink_, other.sink_);
  filters_.swap(other.filters_);
  return *this;
}

FilterChain::~FilterChain() {
}

void FilterChain::SetSink(MessageReceiver* sink) {
  DCHECK(!sink_);
  sink_ = sink;
}

bool FilterChain::Accept(Message* message) {
  DCHECK(sink_);
  for (auto& filter : filters_)
    if (!filter->Accept(message))
      return false;
  return sink_->Accept(message);
}

void FilterChain::Append(std::unique_ptr<MessageReceiver> filter) {
  filters_.emplace_back(std::move(filter));
}

}  // namespace mojo
