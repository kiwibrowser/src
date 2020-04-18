// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/compositor_mutator_impl.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/graphics/compositor_animator.h"
#include "third_party/blink/renderer/platform/graphics/compositor_mutator_client.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"

namespace blink {

CompositorMutatorImpl::CompositorMutatorImpl()
    : client_(nullptr), weak_factory_(this) {
  // By default layout tests run without threaded compositing. See
  // https://crbug.com/770028 For these situations we run on the Main thread.
  if (Platform::Current()->CompositorThread())
    mutator_queue_ = Platform::Current()->CompositorThread()->GetTaskRunner();
  else
    mutator_queue_ = Platform::Current()->MainThread()->GetTaskRunner();
}

CompositorMutatorImpl::~CompositorMutatorImpl() {}

// static
std::unique_ptr<CompositorMutatorClient> CompositorMutatorImpl::CreateClient(
    base::WeakPtr<CompositorMutatorImpl>* weak_interface,
    scoped_refptr<base::SingleThreadTaskRunner>* queue) {
  DCHECK(IsMainThread());
  auto mutator = std::make_unique<CompositorMutatorImpl>();
  // This is allowed since we own the class for the duration of creation.
  *weak_interface = mutator->weak_factory_.GetWeakPtr();
  *queue = mutator->GetTaskRunner();
  return std::make_unique<CompositorMutatorClient>(std::move(mutator));
}

void CompositorMutatorImpl::Mutate(
    std::unique_ptr<CompositorMutatorInputState> state) {
  TRACE_EVENT0("cc", "CompositorMutatorImpl::mutate");
  DCHECK(mutator_queue_->BelongsToCurrentThread());
  if (animators_.IsEmpty())
    return;
  DCHECK(!animator_queue_->BelongsToCurrentThread());
  DCHECK(client_);

  using Outputs = Vector<std::unique_ptr<CompositorMutatorOutputState>>;
  Outputs outputs;
  outputs.ReserveInitialCapacity(animators_.size());

  WaitableEvent done;
  PostCrossThreadTask(
      *animator_queue_, FROM_HERE,
      CrossThreadBind(
          [](const CompositorAnimators* animators,
             std::unique_ptr<CompositorMutatorInputState> state,
             std::unique_ptr<AutoSignal> completion, Outputs* output) {
            for (CompositorAnimator* animator : *animators)
              output->push_back(animator->Mutate(*state));
          },
          CrossThreadUnretained(&animators_), WTF::Passed(std::move(state)),
          WTF::Passed(std::make_unique<AutoSignal>(&done)),
          CrossThreadUnretained(&outputs)));
  // At some point the AutoSignal(&done) gets destroyed and unblocks us.
  done.Wait();

  for (auto& output : outputs) {
    client_->SetMutationUpdate(std::move(output));
  }
}

void CompositorMutatorImpl::RegisterCompositorAnimator(
    CrossThreadPersistent<CompositorAnimator> animator,
    scoped_refptr<base::SingleThreadTaskRunner> queue) {
  TRACE_EVENT0("cc", "CompositorMutatorImpl::RegisterCompositorAnimator");

  DCHECK(animator);
  DCHECK(mutator_queue_->BelongsToCurrentThread());
  if (animators_.IsEmpty()) {
    animator_queue_ = std::move(queue);
  } else {
    DCHECK_EQ(queue, animator_queue_);
  }

  animators_.insert(animator);
}

void CompositorMutatorImpl::UnregisterCompositorAnimator(
    CrossThreadPersistent<CompositorAnimator> animator) {
  TRACE_EVENT0("cc", "CompositorMutatorImpl::UnregisterCompositorAnimator");

  DCHECK(animator);
  DCHECK(mutator_queue_->BelongsToCurrentThread());

  animators_.erase(animator);
}

bool CompositorMutatorImpl::HasAnimators() {
  return !animators_.IsEmpty();
}

CompositorMutatorImpl::AutoSignal::AutoSignal(WaitableEvent* event)
    : event_(event) {
  DCHECK(event);
}

CompositorMutatorImpl::AutoSignal::~AutoSignal() {
  event_->Signal();
}

}  // namespace blink
