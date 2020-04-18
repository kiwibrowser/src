// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/testing/sim/sim_web_view_client.h"

#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace blink {

SimWebViewClient::SimWebViewClient(WebLayerTreeView& layer_tree_view)
    : visually_non_empty_layout_count_(0),
      finished_parsing_layout_count_(0),
      finished_loading_layout_count_(0),
      layer_tree_view_(&layer_tree_view) {}

void SimWebViewClient::DidMeaningfulLayout(
    WebMeaningfulLayout meaningful_layout) {
  switch (meaningful_layout) {
    case WebMeaningfulLayout::kVisuallyNonEmpty:
      visually_non_empty_layout_count_++;
      break;
    case WebMeaningfulLayout::kFinishedParsing:
      finished_parsing_layout_count_++;
      break;
    case WebMeaningfulLayout::kFinishedLoading:
      finished_loading_layout_count_++;
      break;
  }
}

WebView* SimWebViewClient::CreateView(WebLocalFrame* opener,
                                      const WebURLRequest&,
                                      const WebWindowFeatures&,
                                      const WebString& name,
                                      WebNavigationPolicy,
                                      bool,
                                      WebSandboxFlags) {
  return web_view_helper_.InitializeWithOpener(opener);
}

}  // namespace blink
