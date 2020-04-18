/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/wtf/typed_arrays/array_buffer.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/wtf/typed_arrays/array_buffer_view.h"

namespace WTF {

bool ArrayBuffer::Transfer(ArrayBufferContents& result) {
  DCHECK(!IsShared());
  scoped_refptr<ArrayBuffer> keep_alive(this);

  if (!contents_.Data()) {
    result.Neuter();
    return false;
  }

  bool all_views_are_neuterable = true;
  for (ArrayBufferView* i = first_view_; i; i = i->next_view_) {
    if (!i->IsNeuterable())
      all_views_are_neuterable = false;
  }

  if (all_views_are_neuterable) {
    contents_.Transfer(result);

    while (first_view_) {
      ArrayBufferView* current = first_view_;
      RemoveView(current);
      current->Neuter();
    }

    is_neutered_ = true;
  } else {
    // TODO(https://crbug.com/763038): See original bug at
    // https://crbug.com/254728. Copying the buffer instead of transferring is
    // not spec compliant but was added for a WebAudio bug fix. The only time
    // this branch is taken is when attempting to transfer an AudioBuffer's
    // channel data ArrayBuffer.
    contents_.CopyTo(result);
    if (!result.Data())
      return false;
  }

  return true;
}

bool ArrayBuffer::ShareContentsWith(ArrayBufferContents& result) {
  DCHECK(IsShared());
  scoped_refptr<ArrayBuffer> keep_alive(this);

  if (!contents_.DataShared()) {
    result.Neuter();
    return false;
  }

  contents_.ShareWith(result);
  return true;
}

void ArrayBuffer::AddView(ArrayBufferView* view) {
  view->buffer_ = this;
  view->prev_view_ = nullptr;
  view->next_view_ = first_view_;
  if (first_view_)
    first_view_->prev_view_ = view;
  first_view_ = view;
}

void ArrayBuffer::RemoveView(ArrayBufferView* view) {
  DCHECK_EQ(this, view->buffer_.get());
  if (view->next_view_)
    view->next_view_->prev_view_ = view->prev_view_;
  if (view->prev_view_)
    view->prev_view_->next_view_ = view->next_view_;
  if (first_view_ == view)
    first_view_ = view->next_view_;
  view->prev_view_ = view->next_view_ = nullptr;
}

}  // namespace WTF
