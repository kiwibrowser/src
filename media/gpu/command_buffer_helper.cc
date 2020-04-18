// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/command_buffer_helper.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "gpu/command_buffer/common/scheduling_priority.h"
#include "gpu/command_buffer/service/decoder_context.h"
#include "gpu/command_buffer/service/scheduler.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/ipc/service/command_buffer_stub.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "media/gpu/gles2_decoder_helper.h"
#include "ui/gl/gl_context.h"

namespace media {

namespace {

class CommandBufferHelperImpl
    : public CommandBufferHelper,
      public gpu::CommandBufferStub::DestructionObserver {
 public:
  explicit CommandBufferHelperImpl(gpu::CommandBufferStub* stub) : stub_(stub) {
    DVLOG(1) << __func__;
    DCHECK(stub_->channel()->task_runner()->BelongsToCurrentThread());

    stub_->AddDestructionObserver(this);
    wait_sequence_id_ = stub_->channel()->scheduler()->CreateSequence(
        gpu::SchedulingPriority::kNormal);
    decoder_helper_ = GLES2DecoderHelper::Create(stub_->decoder_context());
  }

  gl::GLContext* GetGLContext() override {
    DVLOG(2) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    if (!decoder_helper_)
      return nullptr;

    return decoder_helper_->GetGLContext();
  }

  bool MakeContextCurrent() override {
    DVLOG(2) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    return decoder_helper_ && decoder_helper_->MakeContextCurrent();
  }

  bool IsContextCurrent() const override {
    DVLOG(2) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    if (!stub_)
      return false;

    gl::GLContext* context = stub_->decoder_context()->GetGLContext();
    if (!context)
      return false;

    return context->IsCurrent(nullptr);
  }

  GLuint CreateTexture(GLenum target,
                       GLenum internal_format,
                       GLsizei width,
                       GLsizei height,
                       GLenum format,
                       GLenum type) override {
    DVLOG(2) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    DCHECK(stub_->decoder_context()->GetGLContext()->IsCurrent(nullptr));

    scoped_refptr<gpu::gles2::TextureRef> texture_ref =
        decoder_helper_->CreateTexture(target, internal_format, width, height,
                                       format, type);
    GLuint service_id = texture_ref->service_id();
    texture_refs_[service_id] = std::move(texture_ref);
    return service_id;
  }

  void DestroyTexture(GLuint service_id) override {
    DVLOG(2) << __func__ << "(" << service_id << ")";
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    DCHECK(stub_->decoder_context()->GetGLContext()->IsCurrent(nullptr));
    DCHECK(texture_refs_.count(service_id));

    texture_refs_.erase(service_id);
  }

  void SetCleared(GLuint service_id) override {
    DVLOG(2) << __func__ << "(" << service_id << ")";
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    if (!decoder_helper_)
      return;

    DCHECK(texture_refs_.count(service_id));
    decoder_helper_->SetCleared(texture_refs_[service_id].get());
  }

  bool BindImage(GLuint service_id,
                 gl::GLImage* image,
                 bool can_bind_to_sampler) override {
    DVLOG(2) << __func__ << "(" << service_id << ")";
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    if (!decoder_helper_)
      return false;

    DCHECK(texture_refs_.count(service_id));
    decoder_helper_->BindImage(texture_refs_[service_id].get(), image,
                               can_bind_to_sampler);
    return true;
  }

  gpu::Mailbox CreateMailbox(GLuint service_id) override {
    DVLOG(2) << __func__ << "(" << service_id << ")";
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    if (!decoder_helper_)
      return gpu::Mailbox();

    DCHECK(texture_refs_.count(service_id));
    return decoder_helper_->CreateMailbox(texture_refs_[service_id].get());
  }

  void WaitForSyncToken(gpu::SyncToken sync_token,
                        base::OnceClosure done_cb) override {
    DVLOG(2) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    if (!stub_)
      return;

    // TODO(sandersd): Do we need to keep a ref to |this| while there are
    // pending waits? If we destruct while they are pending, they will never
    // run.
    stub_->channel()->scheduler()->ScheduleTask(
        gpu::Scheduler::Task(wait_sequence_id_, std::move(done_cb),
                             std::vector<gpu::SyncToken>({sync_token})));
  }

  void SetWillDestroyStubCB(WillDestroyStubCB will_destroy_stub_cb) override {
    DCHECK(!will_destroy_stub_cb_);
    will_destroy_stub_cb_ = std::move(will_destroy_stub_cb);
  }

 private:
  ~CommandBufferHelperImpl() override {
    DVLOG(1) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    if (!stub_)
      return;

    // Try to drop TextureRefs with the context current, so that the platform
    // textures can be deleted.
    //
    // Note: Since we don't know what stack we are on, it might not be safe to
    // change the context. In practice we can be reasonably sure that our last
    // owner isn't doing work in a different context.
    //
    // TODO(sandersd): We should restore the previous context.
    if (!texture_refs_.empty() && MakeContextCurrent())
      texture_refs_.clear();

    DestroyStub();
  }

  void OnWillDestroyStub(bool have_context) override {
    DVLOG(1) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    // If we don't have a context, then tell the textures.
    if (!have_context) {
      for (auto iter : texture_refs_)
        iter.second->ForceContextLost();
    }

    // In case |will_destroy_stub_cb_| drops the last reference to |this|, make
    // sure that we're around a bit longer.
    scoped_refptr<CommandBufferHelper> thiz(this);

    if (will_destroy_stub_cb_)
      std::move(will_destroy_stub_cb_).Run(have_context);

    // OnWillDestroyStub() is called with the context current if possible. Drop
    // the TextureRefs now while the platform textures can still be deleted.
    texture_refs_.clear();

    DestroyStub();
  }

  void DestroyStub() {
    DVLOG(3) << __func__;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    decoder_helper_ = nullptr;

    // If the last reference to |this| is in a |done_cb|, destroying the wait
    // sequence can delete |this|. Clearing |stub_| first prevents DestroyStub()
    // being called twice.
    gpu::CommandBufferStub* stub = stub_;
    stub_ = nullptr;

    stub->RemoveDestructionObserver(this);
    stub->channel()->scheduler()->DestroySequence(wait_sequence_id_);
  }

  gpu::CommandBufferStub* stub_;
  // Wait tasks are scheduled on our own sequence so that we can't inadvertently
  // block the command buffer.
  gpu::SequenceId wait_sequence_id_;
  // TODO(sandersd): Merge GLES2DecoderHelper implementation into this class.
  std::unique_ptr<GLES2DecoderHelper> decoder_helper_;
  std::map<GLuint, scoped_refptr<gpu::gles2::TextureRef>> texture_refs_;

  WillDestroyStubCB will_destroy_stub_cb_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(CommandBufferHelperImpl);
};

}  // namespace

// static
scoped_refptr<CommandBufferHelper> CommandBufferHelper::Create(
    gpu::CommandBufferStub* stub) {
  return base::MakeRefCounted<CommandBufferHelperImpl>(stub);
}

}  // namespace media
