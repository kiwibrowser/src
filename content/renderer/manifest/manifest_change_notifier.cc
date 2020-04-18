// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/manifest/manifest_change_notifier.h"

#include <utility>

#include "base/bind.h"
#include "content/public/renderer/render_frame.h"
#include "content/renderer/manifest/manifest_manager.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace content {

ManifestChangeNotifier::ManifestChangeNotifier(RenderFrame* render_frame)
    : RenderFrameObserver(render_frame), weak_factory_(this) {}

ManifestChangeNotifier::~ManifestChangeNotifier() = default;

void ManifestChangeNotifier::DidChangeManifest() {
  // Manifests are not considered when the current page has a unique origin.
  if (!ManifestManager::CanFetchManifest(render_frame()))
    return;

  if (weak_factory_.HasWeakPtrs())
    return;

  // Changing the manifest URL can trigger multiple notifications; the manifest
  // URL update may involve removing the old manifest link before adding the new
  // one, triggering multiple calls to DidChangeManifest(). Coalesce changes
  // during a single event loop task to avoid sending spurious notifications to
  // the browser.
  //
  // During document load, coalescing is disabled to maintain relative ordering
  // of this notification and the favicon URL reporting.
  if (!render_frame()->GetWebFrame()->IsLoading()) {
    render_frame()
        ->GetTaskRunner(blink::TaskType::kInternalLoading)
        ->PostTask(FROM_HERE,
                   base::BindOnce(&ManifestChangeNotifier::ReportManifestChange,
                                  weak_factory_.GetWeakPtr()));
    return;
  }
  ReportManifestChange();
}

void ManifestChangeNotifier::OnDestruct() {
  delete this;
}

void ManifestChangeNotifier::ReportManifestChange() {
  auto manifest_url =
      render_frame()->GetWebFrame()->GetDocument().ManifestURL();
  if (manifest_url.IsNull()) {
    GetManifestChangeObserver().ManifestUrlChanged(base::nullopt);
  } else {
    GetManifestChangeObserver().ManifestUrlChanged(GURL(manifest_url));
  }
}

mojom::ManifestUrlChangeObserver&
ManifestChangeNotifier::GetManifestChangeObserver() {
  if (!manifest_change_observer_) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
        &manifest_change_observer_);
  }
  return *manifest_change_observer_;
}

}  // namespace content
