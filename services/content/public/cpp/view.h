// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CONTENT_PUBLIC_CPP_VIEW_H_
#define SERVICES_CONTENT_PUBLIC_CPP_VIEW_H_

#include <memory>

#include "base/callback.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/content/public/mojom/view.mojom.h"
#include "services/content/public/mojom/view_factory.mojom.h"

namespace content {

// A View is a navigable, top-level view of web content which applications can
// embed within their own UI.
//
// TODO(https://crbug.com/855092): Actually support UI embedding.
class COMPONENT_EXPORT(CONTENT_SERVICE_CPP) View : public mojom::ViewClient {
 public:
  // Constructs a new View using |factory|.
  explicit View(mojom::ViewFactory* factory);
  ~View() override;

  // Begins an attempt to asynchronously navigate this View to |url|.
  void Navigate(const GURL& url);

  void set_did_stop_loading_callback_for_testing(
      base::RepeatingClosure callback) {
    did_stop_loading_callback_ = std::move(callback);
  }

 private:
  // mojom::ViewClient:
  void DidStopLoading() override;

  mojom::ViewPtr view_;
  mojo::Binding<mojom::ViewClient> client_binding_;

  base::RepeatingClosure did_stop_loading_callback_;

  DISALLOW_COPY_AND_ASSIGN(View);
};

}  // namespace content

#endif  // SERVICES_CONTENT_PUBLIC_CPP_VIEW_H_
