// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/codec_image_group.h"

#include "base/sequenced_task_runner.h"
#include "media/gpu/android/avda_surface_bundle.h"

namespace media {

CodecImageGroup::CodecImageGroup(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    scoped_refptr<AVDASurfaceBundle> surface_bundle)
    : surface_bundle_(std::move(surface_bundle)), weak_this_factory_(this) {
  // If the surface bundle has an overlay, then register for destruction
  // callbacks.  We thread-hop to the right thread, which means that we might
  // find out about destruction asynchronously.  Remember that the wp will be
  // cleared on |task_runner|.
  if (surface_bundle_->overlay) {
    surface_bundle_->overlay->AddSurfaceDestroyedCallback(base::BindOnce(
        [](scoped_refptr<base::SequencedTaskRunner> task_runner,
           base::OnceCallback<void(AndroidOverlay*)> cb,
           AndroidOverlay* overlay) -> void {
          task_runner->PostTask(FROM_HERE,
                                base::BindOnce(std::move(cb), overlay));
        },
        std::move(task_runner),
        base::BindOnce(&CodecImageGroup::OnSurfaceDestroyed,
                       weak_this_factory_.GetWeakPtr())));
  }

  // TODO(liberato): if there's no overlay, should we clear |surface_bundle_|?
  // be sure not to call SurfaceDestroyed if !surface_bundle_ in that case when
  // adding a new image.
}

CodecImageGroup::~CodecImageGroup() {}

void CodecImageGroup::SetDestructionCb(
    CodecImage::DestructionCb destruction_cb) {
  destruction_cb_ = std::move(destruction_cb);
}

void CodecImageGroup::AddCodecImage(CodecImage* image) {
  // If somebody adds an image after the surface has been destroyed, fail the
  // image immediately.  This can happen due to thread hopping.
  if (!surface_bundle_) {
    image->SurfaceDestroyed();
    return;
  }

  images_.insert(image);

  // Bind a strong ref to |this| so that the callback will prevent us from being
  // destroyed until the CodecImage is destroyed.
  image->SetDestructionCb(
      base::BindRepeating(&CodecImageGroup::OnCodecImageDestroyed,
                          scoped_refptr<CodecImageGroup>(this)));
}

void CodecImageGroup::OnCodecImageDestroyed(CodecImage* image) {
  images_.erase(image);
  if (destruction_cb_)
    destruction_cb_.Run(image);
}

void CodecImageGroup::OnSurfaceDestroyed(AndroidOverlay* overlay) {
  for (CodecImage* image : images_)
    image->SurfaceDestroyed();

  // While this might cause |surface_bundle_| to be deleted, it's okay because
  // it's a RefCountedDeleteOnSequence.
  surface_bundle_ = nullptr;
}

}  // namespace media
