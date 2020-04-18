// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/raster_implementation.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include <GLES3/gl3.h>
#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <limits>
#include <set>
#include <sstream>
#include <string>

#include "base/atomic_sequence_num.h"
#include "base/bits.h"
#include "base/compiler_specific.h"
#include "base/numerics/safe_math.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_allocator_dump.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "cc/paint/color_space_transfer_cache_entry.h"
#include "cc/paint/decode_stashing_image_provider.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_op_buffer_serializer.h"
#include "cc/paint/transfer_cache_entry.h"
#include "cc/paint/transfer_cache_serialize_helper.h"
#include "gpu/command_buffer/client/gpu_control.h"
#include "gpu/command_buffer/client/query_tracker.h"
#include "gpu/command_buffer/client/raster_cmd_helper.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/client/transfer_buffer.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/ipc/color/gfx_param_traits.h"

#if defined(GPU_CLIENT_DEBUG)
#define GPU_CLIENT_SINGLE_THREAD_CHECK() SingleThreadChecker checker(this);
#else  // !defined(GPU_CLIENT_DEBUG)
#define GPU_CLIENT_SINGLE_THREAD_CHECK()
#endif  // defined(GPU_CLIENT_DEBUG)

// TODO(backer): Update APIs to always write to the destination? See below.
//
// Check that destination pointers point to initialized memory.
// When the context is lost, calling GL function has no effect so if destination
// pointers point to initialized memory it can often lead to crash bugs. eg.
//
// If it was up to us we'd just always write to the destination but the OpenGL
// spec defines the behavior of OpenGL functions, not us. :-(
#if defined(GPU_DCHECK)
#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(v) GPU_DCHECK(v)
#define GPU_CLIENT_DCHECK(v) GPU_DCHECK(v)
#elif defined(DCHECK)
#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(v) DCHECK(v)
#define GPU_CLIENT_DCHECK(v) DCHECK(v)
#else
#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(v) ASSERT(v)
#define GPU_CLIENT_DCHECK(v) ASSERT(v)
#endif

#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION(type, ptr) \
  GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(          \
      ptr &&                                                     \
      (ptr[0] == static_cast<type>(0) || ptr[0] == static_cast<type>(-1)));

#define GPU_CLIENT_VALIDATE_DESTINATION_OPTIONAL_INITALIZATION(type, ptr) \
  GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(                   \
      !ptr ||                                                             \
      (ptr[0] == static_cast<type>(0) || ptr[0] == static_cast<type>(-1)));

using gpu::gles2::GLES2Util;

namespace gpu {
namespace raster {

namespace {

// Helper to copy data to the GPU service over the transfer cache.
class TransferCacheSerializeHelperImpl
    : public cc::TransferCacheSerializeHelper {
 public:
  explicit TransferCacheSerializeHelperImpl(ContextSupport* support)
      : support_(support) {}
  ~TransferCacheSerializeHelperImpl() final = default;

 private:
  bool LockEntryInternal(const EntryKey& key) final {
    return support_->ThreadsafeLockTransferCacheEntry(
        static_cast<uint32_t>(key.first), key.second);
  }

  void CreateEntryInternal(const cc::ClientTransferCacheEntry& entry) final {
    size_t size = entry.SerializedSize();
    void* data = support_->MapTransferCacheEntry(size);
    // TODO(piman): handle error (failed to allocate/map shm)
    DCHECK(data);
    bool succeeded = entry.Serialize(
        base::make_span(reinterpret_cast<uint8_t*>(data), size));
    DCHECK(succeeded);
    support_->UnmapAndCreateTransferCacheEntry(entry.UnsafeType(), entry.Id());
  }

  void FlushEntriesInternal(std::set<EntryKey> entries) final {
    std::vector<std::pair<uint32_t, uint32_t>> transformed;
    transformed.reserve(entries.size());
    for (const auto& e : entries)
      transformed.emplace_back(static_cast<uint32_t>(e.first), e.second);
    support_->UnlockTransferCacheEntries(transformed);
  }

  ContextSupport* const support_;

  DISALLOW_COPY_AND_ASSIGN(TransferCacheSerializeHelperImpl);
};

// Helper to copy PaintOps to the GPU service over the transfer buffer.
class PaintOpSerializer {
 public:
  PaintOpSerializer(size_t initial_size,
                    RasterImplementation* ri,
                    cc::DecodeStashingImageProvider* stashing_image_provider,
                    cc::TransferCacheSerializeHelper* transfer_cache_helper,
                    ClientFontManager* font_manager)
      : ri_(ri),
        buffer_(static_cast<char*>(ri_->MapRasterCHROMIUM(initial_size))),
        stashing_image_provider_(stashing_image_provider),
        transfer_cache_helper_(transfer_cache_helper),
        font_manager_(font_manager),
        free_bytes_(buffer_ ? initial_size : 0) {}

  ~PaintOpSerializer() {
    // Need to call SendSerializedData;
    DCHECK(!written_bytes_);
  }

  size_t Serialize(const cc::PaintOp* op,
                   const cc::PaintOp::SerializeOptions& options) {
    if (!valid())
      return 0;
    size_t size = op->Serialize(buffer_ + written_bytes_, free_bytes_, options);
    if (!size) {
      SendSerializedData();
      buffer_ = static_cast<char*>(ri_->MapRasterCHROMIUM(kBlockAlloc));
      if (!buffer_) {
        free_bytes_ = 0;
        return 0;
      }
      free_bytes_ = kBlockAlloc;
      size = op->Serialize(buffer_ + written_bytes_, free_bytes_, options);
    }
    DCHECK_LE(size, free_bytes_);

    written_bytes_ += size;
    free_bytes_ -= size;
    return size;
  }

  void SendSerializedData() {
    if (!valid())
      return;

    // Serialize fonts before sending raster commands.
    font_manager_->Serialize();
    ri_->UnmapRasterCHROMIUM(written_bytes_);
    // Now that we've issued the RasterCHROMIUM referencing the stashed
    // images, Reset the |stashing_image_provider_|, causing us to issue
    // unlock commands for these images.
    stashing_image_provider_->Reset();
    transfer_cache_helper_->FlushEntries();
    written_bytes_ = 0;
  }

  bool valid() const { return !!buffer_; }

 private:
  static constexpr GLsizeiptr kBlockAlloc = 512 * 1024;

  RasterImplementation* const ri_;
  char* buffer_;
  cc::DecodeStashingImageProvider* const stashing_image_provider_;
  cc::TransferCacheSerializeHelper* const transfer_cache_helper_;
  ClientFontManager* font_manager_;

  size_t written_bytes_ = 0;
  size_t free_bytes_ = 0;

  DISALLOW_COPY_AND_ASSIGN(PaintOpSerializer);
};

}  // namespace

RasterImplementation::SingleThreadChecker::SingleThreadChecker(
    RasterImplementation* raster_implementation)
    : raster_implementation_(raster_implementation) {
  CHECK_EQ(0, raster_implementation_->use_count_);
  ++raster_implementation_->use_count_;
}

RasterImplementation::SingleThreadChecker::~SingleThreadChecker() {
  --raster_implementation_->use_count_;
  CHECK_EQ(0, raster_implementation_->use_count_);
}

RasterImplementation::RasterImplementation(
    RasterCmdHelper* helper,
    TransferBufferInterface* transfer_buffer,
    bool bind_generates_resource,
    bool lose_context_when_out_of_memory,
    GpuControl* gpu_control)
    : ImplementationBase(helper, transfer_buffer, gpu_control),
      helper_(helper),
      active_texture_unit_(0),
      error_bits_(0),
      lose_context_when_out_of_memory_(lose_context_when_out_of_memory),
      use_count_(0),
      current_trace_stack_(0),
      aggressively_free_resources_(false),
      font_manager_(this, helper->command_buffer()),
      lost_(false),
      transfer_cache_(this) {
  DCHECK(helper);
  DCHECK(transfer_buffer);
  DCHECK(gpu_control);

  std::stringstream ss;
  ss << std::hex << this;
  this_in_hex_ = ss.str();
}

gpu::ContextResult RasterImplementation::Initialize(
    const SharedMemoryLimits& limits) {
  TRACE_EVENT0("gpu", "RasterImplementation::Initialize");

  auto result = ImplementationBase::Initialize(limits);
  if (result != gpu::ContextResult::kSuccess) {
    return result;
  }

  texture_units_ = std::make_unique<TextureUnit[]>(
      capabilities_.max_combined_texture_image_units);

  return gpu::ContextResult::kSuccess;
}

RasterImplementation::~RasterImplementation() {
  // Make sure the queries are finished otherwise we'll delete the
  // shared memory (mapped_memory_) which will free the memory used
  // by the queries. The GPU process when validating that memory is still
  // shared will fail and abort (ie, it will stop running).
  WaitForCmd();

  query_tracker_.reset();

  // Make sure the commands make it the service.
  WaitForCmd();
}

RasterCmdHelper* RasterImplementation::helper() const {
  return helper_;
}

IdAllocator* RasterImplementation::GetIdAllocator(IdNamespaces namespace_id) {
  switch (namespace_id) {
    case IdNamespaces::kQueries:
      return &query_id_allocator_;
    case IdNamespaces::kTextures:
      return &texture_id_allocator_;
    default:
      DCHECK(false);
      return nullptr;
  }
}

void RasterImplementation::OnGpuControlLostContext() {
  OnGpuControlLostContextMaybeReentrant();

  // This should never occur more than once.
  DCHECK(!lost_context_callback_run_);
  lost_context_callback_run_ = true;
  if (!lost_context_callback_.is_null()) {
    std::move(lost_context_callback_).Run();
  }
}

void RasterImplementation::OnGpuControlLostContextMaybeReentrant() {
  {
    base::AutoLock hold(lost_lock_);
    lost_ = true;
  }
}

void RasterImplementation::OnGpuControlErrorMessage(const char* message,
                                                    int32_t id) {
  if (!error_message_callback_.is_null())
    error_message_callback_.Run(message, id);
}

void RasterImplementation::OnGpuControlSwapBuffersCompleted(
    const SwapBuffersCompleteParams& params) {
  NOTREACHED();
}

void RasterImplementation::OnSwapBufferPresented(
    uint64_t swap_id,
    const gfx::PresentationFeedback& feedback) {
  NOTREACHED();
}

void RasterImplementation::SetAggressivelyFreeResources(
    bool aggressively_free_resources) {
  TRACE_EVENT1("gpu", "RasterImplementation::SetAggressivelyFreeResources",
               "aggressively_free_resources", aggressively_free_resources);
  aggressively_free_resources_ = aggressively_free_resources;

  if (aggressively_free_resources_ && helper_->HaveRingBuffer()) {
    // Flush will delete transfer buffer resources if
    // |aggressively_free_resources_| is true.
    Flush();
  } else {
    ShallowFlushCHROMIUM();
  }
}

void RasterImplementation::Swap(
    uint32_t /* flags */,
    SwapCompletedCallback /* swap_completed */,
    PresentationCallback /* presentation_callback */) {
  NOTREACHED();
}

void RasterImplementation::SwapWithBounds(
    const std::vector<gfx::Rect>& /* rects */,
    uint32_t /* flags */,
    SwapCompletedCallback /* swap_completed */,
    PresentationCallback /* presentation_callback */) {
  NOTREACHED();
}

void RasterImplementation::PartialSwapBuffers(
    const gfx::Rect& /* sub_buffer */,
    uint32_t /* flags */,
    SwapCompletedCallback /* swap_completed */,
    PresentationCallback /* presentation_callback */) {
  NOTREACHED();
}

void RasterImplementation::CommitOverlayPlanes(
    uint32_t /* flags */,
    SwapCompletedCallback /* swap_completed */,
    PresentationCallback /* presentation_callback */) {
  NOTREACHED();
}

void RasterImplementation::ScheduleOverlayPlane(
    int /* plane_z_order */,
    gfx::OverlayTransform /* plane_transform */,
    unsigned /* overlay_texture_id */,
    const gfx::Rect& /* display_bounds */,
    const gfx::RectF& /* uv_rect */,
    bool /* enable_blend */,
    unsigned /* gpu_fence_id */) {
  NOTREACHED();
}

uint64_t RasterImplementation::ShareGroupTracingGUID() const {
  NOTREACHED();
  return 0;
}

void RasterImplementation::SetErrorMessageCallback(
    base::RepeatingCallback<void(const char*, int32_t)> callback) {
  error_message_callback_ = std::move(callback);
}

void RasterImplementation::SetSnapshotRequested() {
  // Should only be called for real GL contexts.
  NOTREACHED();
}

bool RasterImplementation::ThreadSafeShallowLockDiscardableTexture(
    uint32_t texture_id) {
  NOTREACHED();
  return false;
}

void RasterImplementation::CompleteLockDiscardableTexureOnContextThread(
    uint32_t texture_id) {
  NOTREACHED();
}

bool RasterImplementation::ThreadsafeDiscardableTextureIsDeletedForTracing(
    uint32_t texture_id) {
  NOTREACHED();
  return false;
}

void* RasterImplementation::MapTransferCacheEntry(size_t serialized_size) {
  return transfer_cache_.MapEntry(mapped_memory_.get(), serialized_size);
}

void RasterImplementation::UnmapAndCreateTransferCacheEntry(uint32_t type,
                                                            uint32_t id) {
  transfer_cache_.UnmapAndCreateEntry(type, id);
}

bool RasterImplementation::ThreadsafeLockTransferCacheEntry(uint32_t type,
                                                            uint32_t id) {
  return transfer_cache_.LockEntry(type, id);
}

void RasterImplementation::UnlockTransferCacheEntries(
    const std::vector<std::pair<uint32_t, uint32_t>>& entries) {
  transfer_cache_.UnlockEntries(entries);
}

void RasterImplementation::DeleteTransferCacheEntry(uint32_t type,
                                                    uint32_t id) {
  transfer_cache_.DeleteEntry(type, id);
}

unsigned int RasterImplementation::GetTransferBufferFreeSize() const {
  return transfer_buffer_->GetFreeSize();
}

const std::string& RasterImplementation::GetLogPrefix() const {
  const std::string& prefix(debug_marker_manager_.GetMarker());
  return prefix.empty() ? this_in_hex_ : prefix;
}

GLenum RasterImplementation::GetError() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glGetError()");
  GLenum err = GetGLError();
  GPU_CLIENT_LOG("returned " << GLES2Util::GetStringError(err));
  return err;
}

void RasterImplementation::IssueBeginQuery(GLenum target,
                                           GLuint id,
                                           uint32_t sync_data_shm_id,
                                           uint32_t sync_data_shm_offset) {
  helper_->BeginQueryEXT(target, id, sync_data_shm_id, sync_data_shm_offset);
}

void RasterImplementation::IssueEndQuery(GLenum target, GLuint submit_count) {
  helper_->EndQueryEXT(target, submit_count);
}

void RasterImplementation::IssueQueryCounter(GLuint id,
                                             GLenum target,
                                             uint32_t sync_data_shm_id,
                                             uint32_t sync_data_shm_offset,
                                             GLuint submit_count) {
  NOTIMPLEMENTED();
}

void RasterImplementation::IssueSetDisjointValueSync(
    uint32_t sync_data_shm_id,
    uint32_t sync_data_shm_offset) {
  NOTIMPLEMENTED();
}

GLenum RasterImplementation::GetClientSideGLError() {
  if (error_bits_ == 0) {
    return GL_NO_ERROR;
  }

  GLenum error = GL_NO_ERROR;
  for (uint32_t mask = 1; mask != 0; mask = mask << 1) {
    if ((error_bits_ & mask) != 0) {
      error = GLES2Util::GLErrorBitToGLError(mask);
      break;
    }
  }
  error_bits_ &= ~GLES2Util::GLErrorToErrorBit(error);
  return error;
}

CommandBufferHelper* RasterImplementation::cmd_buffer_helper() {
  return helper_;
}

void RasterImplementation::IssueCreateTransferCacheEntry(
    GLuint entry_type,
    GLuint entry_id,
    GLuint handle_shm_id,
    GLuint handle_shm_offset,
    GLuint data_shm_id,
    GLuint data_shm_offset,
    GLuint data_size) {
  helper_->CreateTransferCacheEntryINTERNAL(entry_type, entry_id, handle_shm_id,
                                            handle_shm_offset, data_shm_id,
                                            data_shm_offset, data_size);
}

void RasterImplementation::IssueDeleteTransferCacheEntry(GLuint entry_type,
                                                         GLuint entry_id) {
  helper_->DeleteTransferCacheEntryINTERNAL(entry_type, entry_id);
}

void RasterImplementation::IssueUnlockTransferCacheEntry(GLuint entry_type,
                                                         GLuint entry_id) {
  helper_->UnlockTransferCacheEntryINTERNAL(entry_type, entry_id);
}

CommandBuffer* RasterImplementation::command_buffer() const {
  return helper_->command_buffer();
}

GLenum RasterImplementation::GetGLError() {
  TRACE_EVENT0("gpu", "RasterImplementation::GetGLError");
  // Check the GL error first, then our wrapped error.
  typedef cmds::GetError::Result Result;
  Result* result = GetResultAs<Result*>();
  // If we couldn't allocate a result the context is lost.
  if (!result) {
    return GL_NO_ERROR;
  }
  *result = GL_NO_ERROR;
  helper_->GetError(GetResultShmId(), GetResultShmOffset());
  WaitForCmd();
  GLenum error = *result;
  if (error == GL_NO_ERROR) {
    error = GetClientSideGLError();
  } else {
    // There was an error, clear the corresponding wrapped error.
    error_bits_ &= ~GLES2Util::GLErrorToErrorBit(error);
  }
  return error;
}

#if defined(RASTER_CLIENT_FAIL_GL_ERRORS)
void RasterImplementation::FailGLError(GLenum error) {
  if (error != GL_NO_ERROR) {
    NOTREACHED() << "Error";
  }
}
// NOTE: Calling GetGLError overwrites data in the result buffer.
void RasterImplementation::CheckGLError() {
  FailGLError(GetGLError());
}
#endif  // defined(RASTER_CLIENT_FAIL_GL_ERRORS)

void RasterImplementation::SetGLError(GLenum error,
                                      const char* function_name,
                                      const char* msg) {
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] Client Synthesized Error: "
                     << GLES2Util::GetStringError(error) << ": "
                     << function_name << ": " << msg);
  FailGLError(error);
  if (msg) {
    last_error_ = msg;
  }
  if (!error_message_callback_.is_null()) {
    std::string temp(GLES2Util::GetStringError(error) + " : " + function_name +
                     ": " + (msg ? msg : ""));
    error_message_callback_.Run(temp.c_str(), 0);
  }
  error_bits_ |= GLES2Util::GLErrorToErrorBit(error);

  if (error == GL_OUT_OF_MEMORY && lose_context_when_out_of_memory_) {
    helper_->LoseContextCHROMIUM(GL_GUILTY_CONTEXT_RESET_ARB,
                                 GL_UNKNOWN_CONTEXT_RESET_ARB);
  }
}

void RasterImplementation::SetGLErrorInvalidEnum(const char* function_name,
                                                 GLenum value,
                                                 const char* label) {
  SetGLError(
      GL_INVALID_ENUM, function_name,
      (std::string(label) + " was " + GLES2Util::GetStringEnum(value)).c_str());
}

bool RasterImplementation::GetIntegervHelper(GLenum pname, GLint* params) {
  switch (pname) {
    case GL_ACTIVE_TEXTURE:
      *params = active_texture_unit_ + GL_TEXTURE0;
      return true;
    case GL_MAX_TEXTURE_SIZE:
      *params = capabilities_.max_texture_size;
      return true;
    case GL_TEXTURE_BINDING_2D:
      *params = texture_units_[active_texture_unit_].bound_texture_2d;
      return true;
    default:
      return false;
  }
}

bool RasterImplementation::GetQueryObjectValueHelper(const char* function_name,
                                                     GLuint id,
                                                     GLenum pname,
                                                     GLuint64* params) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] GetQueryObjectValueHelper(" << id
                     << ", " << GLES2Util::GetStringQueryObjectParameter(pname)
                     << ", " << static_cast<const void*>(params) << ")");

  gles2::QueryTracker::Query* query = query_tracker_->GetQuery(id);
  if (!query) {
    SetGLError(GL_INVALID_OPERATION, function_name, "unknown query id");
    return false;
  }

  if (query->Active()) {
    SetGLError(GL_INVALID_OPERATION, function_name,
               "query active. Did you call glEndQueryEXT?");
    return false;
  }

  if (query->NeverUsed()) {
    SetGLError(GL_INVALID_OPERATION, function_name,
               "Never used. Did you call glBeginQueryEXT?");
    return false;
  }

  bool valid_value = false;
  switch (pname) {
    case GL_QUERY_RESULT_EXT:
      if (!query->CheckResultsAvailable(helper_)) {
        helper_->WaitForToken(query->token());
        if (!query->CheckResultsAvailable(helper_)) {
          FinishHelper();
          CHECK(query->CheckResultsAvailable(helper_));
        }
      }
      *params = query->GetResult();
      valid_value = true;
      break;
    case GL_QUERY_RESULT_AVAILABLE_EXT:
      *params = query->CheckResultsAvailable(helper_);
      valid_value = true;
      break;
    default:
      SetGLErrorInvalidEnum(function_name, pname, "pname");
      break;
  }
  GPU_CLIENT_LOG("  " << *params);
  CheckGLError();
  return valid_value;
}

void RasterImplementation::Flush() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glFlush()");
  // Insert the cmd to call glFlush
  helper_->Flush();
  FlushHelper();
}

void RasterImplementation::IssueShallowFlush() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glShallowFlushCHROMIUM()");
  FlushHelper();
}

void RasterImplementation::ShallowFlushCHROMIUM() {
  IssueShallowFlush();
}

void RasterImplementation::FlushHelper() {
  // Flush our command buffer
  // (tell the service to execute up to the flush cmd.)
  helper_->CommandBufferHelper::Flush();

  if (aggressively_free_resources_)
    FreeEverything();
}

void RasterImplementation::OrderingBarrierCHROMIUM() {
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glOrderingBarrierCHROMIUM");
  // Flush command buffer at the GPU channel level.  May be implemented as
  // Flush().
  helper_->CommandBufferHelper::OrderingBarrier();
}

void RasterImplementation::Finish() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  FinishHelper();
}

void RasterImplementation::FinishHelper() {
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glFinish()");
  TRACE_EVENT0("gpu", "RasterImplementation::Finish");
  // Insert the cmd to call glFinish
  helper_->Finish();
  // Finish our command buffer
  // (tell the service to execute up to the Finish cmd and wait for it to
  // execute.)
  helper_->CommandBufferHelper::Finish();

  if (aggressively_free_resources_)
    FreeEverything();
}

void RasterImplementation::GenQueriesEXTHelper(GLsizei /* n */,
                                               const GLuint* /* queries */) {}

void RasterImplementation::DeleteTexturesHelper(GLsizei n,
                                                const GLuint* textures) {
  helper_->DeleteTexturesImmediate(n, textures);
  for (GLsizei ii = 0; ii < n; ++ii) {
    texture_id_allocator_.FreeID(textures[ii]);
  }
  UnbindTexturesHelper(n, textures);
}

void RasterImplementation::UnbindTexturesHelper(GLsizei n,
                                                const GLuint* textures) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    for (GLint tt = 0; tt < capabilities_.max_combined_texture_image_units;
         ++tt) {
      TextureUnit& unit = texture_units_[tt];
      if (textures[ii] == unit.bound_texture_2d) {
        unit.bound_texture_2d = 0;
      }
    }
  }
}

GLenum RasterImplementation::GetGraphicsResetStatusKHR() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glGetGraphicsResetStatusKHR()");

  base::AutoLock hold(lost_lock_);
  if (lost_)
    return GL_UNKNOWN_CONTEXT_RESET_KHR;
  return GL_NO_ERROR;
}

void RasterImplementation::DeleteQueriesEXTHelper(GLsizei n,
                                                  const GLuint* queries) {
  IdAllocator* id_allocator = GetIdAllocator(IdNamespaces::kQueries);
  for (GLsizei ii = 0; ii < n; ++ii) {
    query_tracker_->RemoveQuery(queries[ii]);
    id_allocator->FreeID(queries[ii]);
  }

  helper_->DeleteQueriesEXTImmediate(n, queries);
}

void RasterImplementation::BeginQueryEXT(GLenum target, GLuint id) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] BeginQueryEXT("
                     << GLES2Util::GetStringQueryTarget(target) << ", " << id
                     << ")");

  switch (target) {
    case GL_COMMANDS_COMPLETED_CHROMIUM:
      if (!capabilities_.sync_query) {
        SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT",
                   "not enabled for commands completed queries");
        return;
      }
      break;
    default:
      SetGLError(GL_INVALID_ENUM, "glBeginQueryEXT", "unknown query target");
      return;
  }

  // if any outstanding queries INV_OP
  if (query_tracker_->GetCurrentQuery(target)) {
    SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT",
               "query already in progress");
    return;
  }

  if (id == 0) {
    SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT", "id is 0");
    return;
  }

  if (!GetIdAllocator(IdNamespaces::kQueries)->InUse(id)) {
    SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT", "invalid id");
    return;
  }

  if (query_tracker_->BeginQuery(id, target, this))
    CheckGLError();
}

void RasterImplementation::EndQueryEXT(GLenum target) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] EndQueryEXT("
                     << GLES2Util::GetStringQueryTarget(target) << ")");
  if (query_tracker_->EndQuery(target, this))
    CheckGLError();
}

void RasterImplementation::GetQueryObjectuivEXT(GLuint id,
                                                GLenum pname,
                                                GLuint* params) {
  GLuint64 result = 0;
  if (GetQueryObjectValueHelper("glGetQueryObjectuivEXT", id, pname, &result))
    *params = base::saturated_cast<GLuint>(result);
}

void RasterImplementation::GenSyncTokenCHROMIUM(GLbyte* sync_token) {
  if (!sync_token) {
    SetGLError(GL_INVALID_VALUE, "glGenSyncTokenCHROMIUM", "empty sync_token");
    return;
  }

  uint64_t fence_sync = gpu_control_->GenerateFenceSyncRelease();
  helper_->InsertFenceSyncCHROMIUM(fence_sync);
  helper_->CommandBufferHelper::OrderingBarrier();
  gpu_control_->EnsureWorkVisible();

  // Copy the data over after setting the data to ensure alignment.
  SyncToken sync_token_data(gpu_control_->GetNamespaceID(),
                            gpu_control_->GetCommandBufferID(), fence_sync);
  sync_token_data.SetVerifyFlush();
  memcpy(sync_token, &sync_token_data, sizeof(sync_token_data));
}

void RasterImplementation::GenUnverifiedSyncTokenCHROMIUM(GLbyte* sync_token) {
  if (!sync_token) {
    SetGLError(GL_INVALID_VALUE, "glGenUnverifiedSyncTokenCHROMIUM",
               "empty sync_token");
    return;
  }

  uint64_t fence_sync = gpu_control_->GenerateFenceSyncRelease();
  helper_->InsertFenceSyncCHROMIUM(fence_sync);
  helper_->CommandBufferHelper::OrderingBarrier();

  // Copy the data over after setting the data to ensure alignment.
  SyncToken sync_token_data(gpu_control_->GetNamespaceID(),
                            gpu_control_->GetCommandBufferID(), fence_sync);
  memcpy(sync_token, &sync_token_data, sizeof(sync_token_data));
}

void RasterImplementation::VerifySyncTokensCHROMIUM(GLbyte** sync_tokens,
                                                    GLsizei count) {
  bool requires_synchronization = false;
  for (GLsizei i = 0; i < count; ++i) {
    if (sync_tokens[i]) {
      SyncToken sync_token;
      memcpy(&sync_token, sync_tokens[i], sizeof(sync_token));

      if (sync_token.HasData() && !sync_token.verified_flush()) {
        if (!GetVerifiedSyncTokenForIPC(sync_token, &sync_token)) {
          SetGLError(GL_INVALID_VALUE, "glVerifySyncTokensCHROMIUM",
                     "Cannot verify sync token using this context.");
          return;
        }
        requires_synchronization = true;
        DCHECK(sync_token.verified_flush());
      }

      // Set verify bit on empty sync tokens too.
      sync_token.SetVerifyFlush();

      memcpy(sync_tokens[i], &sync_token, sizeof(sync_token));
    }
  }

  // Ensure all the fence syncs are visible on GPU service.
  if (requires_synchronization)
    gpu_control_->EnsureWorkVisible();
}

void RasterImplementation::WaitSyncTokenCHROMIUM(
    const GLbyte* sync_token_data) {
  if (!sync_token_data)
    return;

  // Copy the data over before data access to ensure alignment.
  SyncToken sync_token, verified_sync_token;
  memcpy(&sync_token, sync_token_data, sizeof(SyncToken));

  if (!sync_token.HasData())
    return;

  if (!GetVerifiedSyncTokenForIPC(sync_token, &verified_sync_token)) {
    SetGLError(GL_INVALID_VALUE, "glWaitSyncTokenCHROMIUM",
               "Cannot wait on sync_token which has not been verified");
    return;
  }

  helper_->WaitSyncTokenCHROMIUM(
      static_cast<GLint>(sync_token.namespace_id()),
      sync_token.command_buffer_id().GetUnsafeValue(),
      sync_token.release_count());

  // Enqueue sync token in flush after inserting command so that it's not
  // included in an automatic flush.
  gpu_control_->WaitSyncTokenHint(verified_sync_token);
}

namespace {

bool CreateImageValidInternalFormat(GLenum internalformat,
                                    const Capabilities& capabilities) {
  switch (internalformat) {
    case GL_ATC_RGB_AMD:
    case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:
      return capabilities.texture_format_atc;
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return capabilities.texture_format_dxt1;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return capabilities.texture_format_dxt5;
    case GL_ETC1_RGB8_OES:
      return capabilities.texture_format_etc1;
    case GL_R16_EXT:
      return capabilities.texture_norm16;
    case GL_RGB10_A2_EXT:
      return capabilities.image_xr30;
    case GL_RED:
    case GL_RG_EXT:
    case GL_RGB:
    case GL_RGBA:
    case GL_RGB_YCBCR_422_CHROMIUM:
    case GL_RGB_YCBCR_420V_CHROMIUM:
    case GL_RGB_YCRCB_420_CHROMIUM:
    case GL_BGRA_EXT:
      return true;
    default:
      return false;
  }
}

}  // namespace

GLuint RasterImplementation::CreateImageCHROMIUMHelper(ClientBuffer buffer,
                                                       GLsizei width,
                                                       GLsizei height,
                                                       GLenum internalformat) {
  if (width <= 0) {
    SetGLError(GL_INVALID_VALUE, "glCreateImageCHROMIUM", "width <= 0");
    return 0;
  }

  if (height <= 0) {
    SetGLError(GL_INVALID_VALUE, "glCreateImageCHROMIUM", "height <= 0");
    return 0;
  }

  if (!CreateImageValidInternalFormat(internalformat, capabilities_)) {
    SetGLError(GL_INVALID_VALUE, "glCreateImageCHROMIUM", "invalid format");
    return 0;
  }

  // CreateImage creates a fence sync so we must flush first to ensure all
  // previously created fence syncs are flushed first.
  FlushHelper();

  int32_t image_id =
      gpu_control_->CreateImage(buffer, width, height, internalformat);
  if (image_id < 0) {
    SetGLError(GL_OUT_OF_MEMORY, "glCreateImageCHROMIUM", "image_id < 0");
    return 0;
  }
  return image_id;
}

GLuint RasterImplementation::CreateImageCHROMIUM(ClientBuffer buffer,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLenum internalformat) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glCreateImageCHROMIUM(" << width
                     << ", " << height << ", "
                     << GLES2Util::GetStringImageInternalFormat(internalformat)
                     << ")");
  GLuint image_id =
      CreateImageCHROMIUMHelper(buffer, width, height, internalformat);
  CheckGLError();
  return image_id;
}

void RasterImplementation::DestroyImageCHROMIUMHelper(GLuint image_id) {
  // Flush the command stream to make sure all pending commands
  // that may refer to the image_id are executed on the service side.
  helper_->CommandBufferHelper::Flush();
  gpu_control_->DestroyImage(image_id);
}

void RasterImplementation::DestroyImageCHROMIUM(GLuint image_id) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glDestroyImageCHROMIUM("
                     << image_id << ")");
  DestroyImageCHROMIUMHelper(image_id);
  CheckGLError();
}

void* RasterImplementation::MapRasterCHROMIUM(GLsizeiptr size) {
  if (size < 0) {
    SetGLError(GL_INVALID_VALUE, "glMapRasterCHROMIUM", "negative size");
    return nullptr;
  }
  if (raster_mapped_buffer_) {
    SetGLError(GL_INVALID_OPERATION, "glMapRasterCHROMIUM", "already mapped");
    return nullptr;
  }
  raster_mapped_buffer_.emplace(size, helper_, transfer_buffer_);
  if (!raster_mapped_buffer_->valid()) {
    SetGLError(GL_INVALID_OPERATION, "glMapRasterCHROMIUM", "size too big");
    raster_mapped_buffer_ = base::nullopt;
    return nullptr;
  }
  return raster_mapped_buffer_->address();
}

void* RasterImplementation::MapFontBuffer(size_t size) {
  if (size < 0) {
    SetGLError(GL_INVALID_VALUE, "glMapFontBufferCHROMIUM", "negative size");
    return nullptr;
  }
  if (font_mapped_buffer_) {
    SetGLError(GL_INVALID_OPERATION, "glMapFontBufferCHROMIUM",
               "already mapped");
    return nullptr;
  }
  if (!raster_mapped_buffer_) {
    SetGLError(GL_INVALID_OPERATION, "glMapFontBufferCHROMIUM",
               "mapped font buffer with no raster buffer");
    return nullptr;
  }

  font_mapped_buffer_.emplace(size, helper_, mapped_memory_.get());
  if (!font_mapped_buffer_->valid()) {
    SetGLError(GL_INVALID_OPERATION, "glMapFontBufferCHROMIUM", "size too big");
    font_mapped_buffer_ = base::nullopt;
    return nullptr;
  }
  return font_mapped_buffer_->address();
}

void RasterImplementation::UnmapRasterCHROMIUM(GLsizeiptr written_size) {
  if (written_size < 0) {
    SetGLError(GL_INVALID_VALUE, "glUnmapRasterCHROMIUM",
               "negative written_size");
    return;
  }
  if (!raster_mapped_buffer_) {
    SetGLError(GL_INVALID_OPERATION, "glUnmapRasterCHROMIUM", "not mapped");
    return;
  }
  DCHECK(raster_mapped_buffer_->valid());
  if (written_size == 0) {
    raster_mapped_buffer_->Discard();
    raster_mapped_buffer_ = base::nullopt;
    return;
  }
  raster_mapped_buffer_->Shrink(written_size);

  GLuint font_shm_id = 0u;
  GLuint font_shm_offset = 0u;
  GLsizeiptr font_shm_size = 0u;
  if (font_mapped_buffer_) {
    font_shm_id = font_mapped_buffer_->shm_id();
    font_shm_offset = font_mapped_buffer_->offset();
    font_shm_size = font_mapped_buffer_->size();
  }
  helper_->RasterCHROMIUM(raster_mapped_buffer_->shm_id(),
                          raster_mapped_buffer_->offset(), written_size,
                          font_shm_id, font_shm_offset, font_shm_size);
  raster_mapped_buffer_ = base::nullopt;
  font_mapped_buffer_ = base::nullopt;
  CheckGLError();
}

// Include the auto-generated part of this file. We split this because it means
// we can easily edit the non-auto generated parts right here in this file
// instead of having to edit some template or the code generator.
#include "gpu/command_buffer/client/raster_implementation_impl_autogen.h"

void RasterImplementation::SetColorSpaceMetadata(GLuint texture_id,
                                                 GLColorSpace color_space) {
#if defined(__native_client__)
  // Including gfx::ColorSpace would bring Skia and a lot of other code into
  // NaCl's IRT.
  SetGLError(GL_INVALID_VALUE, "RasterImplementation::SetColorSpaceMetadata",
             "not supported");
#else
  gfx::ColorSpace* gfx_color_space =
      reinterpret_cast<gfx::ColorSpace*>(color_space);
  base::Pickle color_space_data;
  IPC::ParamTraits<gfx::ColorSpace>::Write(&color_space_data, *gfx_color_space);

  ScopedTransferBufferPtr buffer(color_space_data.size(), helper_,
                                 transfer_buffer_);
  if (!buffer.valid() || buffer.size() < color_space_data.size()) {
    SetGLError(GL_OUT_OF_MEMORY, "RasterImplementation::SetColorSpaceMetadata",
               "out of memory");
    return;
  }
  memcpy(buffer.address(), color_space_data.data(), color_space_data.size());
  helper_->SetColorSpaceMetadata(texture_id, buffer.shm_id(), buffer.offset(),
                                 color_space_data.size());
#endif
}

void RasterImplementation::GenMailbox(GLbyte* mailbox) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glGenMailboxCHROMIUM("
                     << static_cast<const void*>(mailbox) << ")");
  TRACE_EVENT0("gpu", "RasterImplementation::GenMailboxCHROMIUM");

  gpu::Mailbox result = gpu::Mailbox::Generate();
  memcpy(mailbox, result.name, sizeof(result.name));
}

void RasterImplementation::ProduceTextureDirect(GLuint texture,
                                                const GLbyte* data) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glProduceTextureDirectCHROMIUM("
                     << static_cast<const void*>(data) << ")");
  const Mailbox& mailbox = *reinterpret_cast<const Mailbox*>(data);
  DCHECK(mailbox.Verify()) << "ProduceTextureDirectCHROMIUM was passed a "
                              "mailbox that was not generated by "
                              "GenMailboxCHROMIUM.";
  helper_->ProduceTextureDirectImmediate(texture, data);
  CheckGLError();
}

GLuint RasterImplementation::CreateAndConsumeTexture(
    bool use_buffer,
    gfx::BufferUsage buffer_usage,
    viz::ResourceFormat format,
    const GLbyte* mailbox) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glCreateAndConsumeTexture("
                     << use_buffer << ", "
                     << static_cast<uint32_t>(buffer_usage) << ", "
                     << static_cast<uint32_t>(format) << ", "
                     << static_cast<const void*>(mailbox) << ")");
  GLuint client_id = texture_id_allocator_.AllocateID();
  helper_->CreateAndConsumeTextureINTERNALImmediate(
      client_id, use_buffer, buffer_usage, format, mailbox);
  GPU_CLIENT_LOG("returned " << client_id);
  CheckGLError();
  return client_id;
}

void RasterImplementation::BeginRasterCHROMIUM(
    GLuint texture_id,
    GLuint sk_color,
    GLuint msaa_sample_count,
    GLboolean can_use_lcd_text,
    GLint color_type,
    const cc::RasterColorSpace& raster_color_space) {
  DCHECK(!raster_properties_);

  TransferCacheSerializeHelperImpl transfer_cache_serialize_helper(this);
  if (!transfer_cache_serialize_helper.LockEntry(
          cc::TransferCacheEntryType::kColorSpace,
          raster_color_space.color_space_id)) {
    transfer_cache_serialize_helper.CreateEntry(
        cc::ClientColorSpaceTransferCacheEntry(raster_color_space));
  }
  transfer_cache_serialize_helper.AssertLocked(
      cc::TransferCacheEntryType::kColorSpace,
      raster_color_space.color_space_id);

  helper_->BeginRasterCHROMIUM(texture_id, sk_color, msaa_sample_count,
                               can_use_lcd_text, color_type,
                               raster_color_space.color_space_id);
  transfer_cache_serialize_helper.FlushEntries();

  raster_properties_.emplace(sk_color, can_use_lcd_text,
                             raster_color_space.color_space.ToSkColorSpace());
}

void RasterImplementation::RasterCHROMIUM(const cc::DisplayItemList* list,
                                          cc::ImageProvider* provider,
                                          const gfx::Size& content_size,
                                          const gfx::Rect& full_raster_rect,
                                          const gfx::Rect& playback_rect,
                                          const gfx::Vector2dF& post_translate,
                                          GLfloat post_scale,
                                          bool requires_clear) {
  if (std::abs(post_scale) < std::numeric_limits<float>::epsilon())
    return;

  gfx::Rect query_rect =
      gfx::ScaleToEnclosingRect(playback_rect, 1.f / post_scale);
  std::vector<size_t> offsets = list->rtree_.Search(query_rect);
  if (offsets.empty())
    return;

  // TODO(enne): Tune these numbers
  // TODO(enne): Convert these types here and in transfer buffer to be size_t.
  static constexpr unsigned int kMinAlloc = 16 * 1024;
  unsigned int free_size = std::max(GetTransferBufferFreeSize(), kMinAlloc);

  // This section duplicates RasterSource::PlaybackToCanvas setup preamble.
  cc::PaintOpBufferSerializer::Preamble preamble;
  preamble.content_size = content_size;
  preamble.full_raster_rect = full_raster_rect;
  preamble.playback_rect = playback_rect;
  preamble.post_translation = post_translate;
  preamble.post_scale = gfx::SizeF(post_scale, post_scale);
  preamble.requires_clear = requires_clear;
  preamble.background_color = raster_properties_->background_color;

  // Wrap the provided provider in a stashing provider so that we can delay
  // unrefing images until we have serialized dependent commands.
  cc::DecodeStashingImageProvider stashing_image_provider(provider);

  // TODO(enne): Don't access private members of DisplayItemList.
  TransferCacheSerializeHelperImpl transfer_cache_serialize_helper(this);
  PaintOpSerializer op_serializer(free_size, this, &stashing_image_provider,
                                  &transfer_cache_serialize_helper,
                                  &font_manager_);
  cc::PaintOpBufferSerializer::SerializeCallback serialize_cb =
      base::BindRepeating(&PaintOpSerializer::Serialize,
                          base::Unretained(&op_serializer));

  cc::PaintOpBufferSerializer serializer(
      serialize_cb, &stashing_image_provider, &transfer_cache_serialize_helper,
      font_manager_.strike_server(), raster_properties_->color_space.get(),
      raster_properties_->can_use_lcd_text);
  serializer.Serialize(&list->paint_op_buffer_, &offsets, preamble);
  // TODO(piman): raise error if !serializer.valid()?
  op_serializer.SendSerializedData();
}

void RasterImplementation::EndRasterCHROMIUM() {
  DCHECK(raster_properties_);

  raster_properties_.reset();
  helper_->EndRasterCHROMIUM();
}

void RasterImplementation::BeginGpuRaster() {
  NOTREACHED();
}
void RasterImplementation::EndGpuRaster() {
  NOTREACHED();
}

void RasterImplementation::TraceBeginCHROMIUM(const char* category_name,
                                              const char* trace_name) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glTraceBeginCHROMIUM("
                     << category_name << ", " << trace_name << ")");
  SetBucketAsCString(kResultBucketId, category_name);
  SetBucketAsCString(kResultBucketId + 1, trace_name);
  helper_->TraceBeginCHROMIUM(kResultBucketId, kResultBucketId + 1);
  helper_->SetBucketSize(kResultBucketId, 0);
  helper_->SetBucketSize(kResultBucketId + 1, 0);
  current_trace_stack_++;
}

void RasterImplementation::TraceEndCHROMIUM() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glTraceEndCHROMIUM("
                     << ")");
  if (current_trace_stack_ == 0) {
    SetGLError(GL_INVALID_OPERATION, "glTraceEndCHROMIUM",
               "missing begin trace");
    return;
  }
  helper_->TraceEndCHROMIUM();
  current_trace_stack_--;
}

RasterImplementation::RasterProperties::RasterProperties(
    SkColor background_color,
    bool can_use_lcd_text,
    sk_sp<SkColorSpace> color_space)
    : background_color(background_color),
      can_use_lcd_text(can_use_lcd_text),
      color_space(std::move(color_space)) {}

RasterImplementation::RasterProperties::~RasterProperties() = default;

}  // namespace raster
}  // namespace gpu
