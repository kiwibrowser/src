/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Google, Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/frame_loader_state_machine.h"

#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

FrameLoaderStateMachine::FrameLoaderStateMachine()
    : state_(kCreatingInitialEmptyDocument) {}

bool FrameLoaderStateMachine::CommittedFirstRealDocumentLoad() const {
  return state_ >= kCommittedFirstRealLoad;
}

bool FrameLoaderStateMachine::CreatingInitialEmptyDocument() const {
  return state_ == kCreatingInitialEmptyDocument;
}

bool FrameLoaderStateMachine::CommittedMultipleRealLoads() const {
  return state_ == kCommittedMultipleRealLoads;
}

bool FrameLoaderStateMachine::IsDisplayingInitialEmptyDocument() const {
  return state_ >= kDisplayingInitialEmptyDocument &&
         state_ < kCommittedFirstRealLoad;
}

void FrameLoaderStateMachine::AdvanceTo(State state) {
  DCHECK_LT(state_, state);
  state_ = state;
}

String FrameLoaderStateMachine::ToString() const {
  switch (state_) {
    case kCreatingInitialEmptyDocument:
      return "CreatingInitialEmptyDocument";
    case kDisplayingInitialEmptyDocument:
      return "DisplayingInitialEmptyDocument";
    case kCommittedFirstRealLoad:
      return "CommittedFirstRealLoad";
    case kCommittedMultipleRealLoads:
      return "CommittedMultipleRealLoads";
    default:
      NOTREACHED();
  }
  return "";
}

}  // namespace blink
