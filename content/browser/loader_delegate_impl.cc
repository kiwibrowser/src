// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader_delegate_impl.h"

#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/navigator.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"

namespace content {

namespace {

}  // namespace

LoaderDelegateImpl::~LoaderDelegateImpl() {}

void LoaderDelegateImpl::LoadStateChanged(
    WebContents* web_contents,
    const std::string& host,
    const net::LoadStateWithParam& load_state,
    uint64_t upload_position,
    uint64_t upload_size) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  static_cast<WebContentsImpl*>(web_contents)
      ->LoadStateChanged(host, load_state, upload_position, upload_size);
}

}  // namespace content
