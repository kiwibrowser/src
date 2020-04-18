// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MANIFEST_MANIFEST_CHANGE_NOTIFIER_H_
#define CONTENT_RENDERER_MANIFEST_MANIFEST_CHANGE_NOTIFIER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/manifest_observer.mojom.h"
#include "content/public/renderer/render_frame_observer.h"

namespace content {

class ManifestChangeNotifier : public RenderFrameObserver {
 public:
  explicit ManifestChangeNotifier(RenderFrame* render_frame);
  ~ManifestChangeNotifier() override;

 private:
  // RenderFrameObserver implementation.
  void DidChangeManifest() override;
  void OnDestruct() override;

  void ReportManifestChange();
  mojom::ManifestUrlChangeObserver& GetManifestChangeObserver();

  mojom::ManifestUrlChangeObserverAssociatedPtr manifest_change_observer_;

  base::WeakPtrFactory<ManifestChangeNotifier> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ManifestChangeNotifier);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MANIFEST_MANIFEST_CHANGE_NOTIFIER_H_
