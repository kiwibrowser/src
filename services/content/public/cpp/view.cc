// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/content/public/cpp/view.h"

namespace content {

View::View(mojom::ViewFactory* factory) : client_binding_(this) {
  mojom::ViewClientPtr client;
  client_binding_.Bind(mojo::MakeRequest(&client));
  factory->CreateView(mojom::ViewParams::New(), mojo::MakeRequest(&view_),
                      std::move(client));
}

View::~View() = default;

void View::Navigate(const GURL& url) {
  view_->Navigate(url);
}

void View::DidStopLoading() {
  if (did_stop_loading_callback_)
    did_stop_loading_callback_.Run();
}

}  // namespace content
