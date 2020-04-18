// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/contextualsearch/unhandled_tap_web_contents_observer.h"

#include <utility>

#include "build/build_config.h"
#include "third_party/blink/public/public_buildflags.h"

#if BUILDFLAG(ENABLE_UNHANDLED_TAP)
#include "chrome/browser/android/contextualsearch/unhandled_tap_notifier_impl.h"
#endif  // BUILDFLAG(ENABLE_UNHANDLED_TAP)

namespace contextual_search {

UnhandledTapWebContentsObserver::UnhandledTapWebContentsObserver(
    content::WebContents* web_contents,
    float device_scale_factor,
    UnhandledTapCallback callback)
    : content::WebContentsObserver(web_contents) {
#if BUILDFLAG(ENABLE_UNHANDLED_TAP)
  registry_.AddInterface(
      base::BindRepeating(&contextual_search::CreateUnhandledTapNotifierImpl,
                          device_scale_factor, std::move(callback)));
#endif  // BUILDFLAG(ENABLE_UNHANDLED_TAP)
}

UnhandledTapWebContentsObserver::~UnhandledTapWebContentsObserver() {}

void UnhandledTapWebContentsObserver::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
#if BUILDFLAG(ENABLE_UNHANDLED_TAP)
  registry_.TryBindInterface(interface_name, interface_pipe);
#endif  // BUILDFLAG(ENABLE_UNHANDLED_TAP)
}

}  // namespace contextual_search
