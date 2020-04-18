// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/client_font_manager.h"

namespace gpu {
namespace raster {

namespace {

class Serializer {
 public:
  Serializer(char* memory, size_t memory_size)
      : memory_(memory), memory_size_(memory_size) {}
  ~Serializer() = default;

  template <typename T>
  void Write(const T* val) {
    static_assert(base::is_trivially_copyable<T>::value, "");
    WriteData(val, sizeof(T), alignof(T));
  }

  void WriteData(const void* input, size_t bytes, size_t alignment) {
    AlignMemory(bytes, alignment);
    if (bytes == 0)
      return;

    memcpy(memory_, input, bytes);
    memory_ += bytes;
    bytes_written_ += bytes;
  }

 private:
  void AlignMemory(size_t size, size_t alignment) {
    // Due to the math below, alignment must be a power of two.
    DCHECK_GT(alignment, 0u);
    DCHECK_EQ(alignment & (alignment - 1), 0u);

    uintptr_t memory = reinterpret_cast<uintptr_t>(memory_);
    size_t padding = ((memory + alignment - 1) & ~(alignment - 1)) - memory;
    DCHECK_LE(bytes_written_ + size + padding, memory_size_);

    memory_ += padding;
    bytes_written_ += padding;
  }

  char* memory_ = nullptr;
  size_t memory_size_ = 0u;
  size_t bytes_written_ = 0u;
};

}  // namespace

ClientFontManager::ClientFontManager(Client* client,
                                     CommandBuffer* command_buffer)
    : client_(client), command_buffer_(command_buffer), strike_server_(this) {}

ClientFontManager::~ClientFontManager() = default;

SkDiscardableHandleId ClientFontManager::createHandle() {
  SkDiscardableHandleId handle_id = ++last_allocated_handle_id_;
  discardable_handle_map_[handle_id] =
      client_discardable_manager_.CreateHandle(command_buffer_);

  // Handles start with a ref-count.
  locked_handles_.insert(handle_id);
  return handle_id;
}

bool ClientFontManager::lockHandle(SkDiscardableHandleId handle_id) {
  // Already locked.
  if (locked_handles_.find(handle_id) != locked_handles_.end())
    return true;

  auto it = discardable_handle_map_.find(handle_id);
  DCHECK(it != discardable_handle_map_.end());
  bool locked = client_discardable_manager_.LockHandle(it->second);
  if (locked) {
    locked_handles_.insert(handle_id);
    return true;
  }

  discardable_handle_map_.erase(it);
  return false;
}

void ClientFontManager::Serialize() {
  // TODO(khushalsagar): May be skia can track the size required so we avoid
  // this copy.
  std::vector<uint8_t> strike_data;
  strike_server_.writeStrikeData(&strike_data);

  const size_t num_handles_created =
      last_allocated_handle_id_ - last_serialized_handle_id_;
  if (strike_data.size() == 0u && num_handles_created == 0u &&
      locked_handles_.size() == 0u) {
    // No font data to serialize.
    return;
  }

  // Size requires for serialization.
  size_t bytes_required =
      // Skia data size.
      +sizeof(size_t) + alignof(size_t) + strike_data.size() +
      alignof(std::max_align_t)
      // num of handles created + SerializableHandles.
      + sizeof(size_t) + alignof(size_t) +
      num_handles_created * sizeof(SerializableSkiaHandle) +
      alignof(SerializableSkiaHandle) +
      // num of handles locked + DiscardableHandleIds.
      +sizeof(size_t) + alignof(size_t) +
      locked_handles_.size() * sizeof(SkDiscardableHandleId) +
      alignof(SkDiscardableHandleId);

  // Allocate memory.
  void* memory = client_->MapFontBuffer(bytes_required);
  if (!memory) {
    // We are likely in a context loss situation if mapped memory allocation
    // for font buffer failed.
    return;
  }
  Serializer serializer(reinterpret_cast<char*>(memory), bytes_required);

  // Serialize all new handles.
  serializer.Write<size_t>(&num_handles_created);
  for (SkDiscardableHandleId handle_id = last_serialized_handle_id_ + 1;
       handle_id <= last_allocated_handle_id_; handle_id++) {
    auto it = discardable_handle_map_.find(handle_id);
    DCHECK(it != discardable_handle_map_.end());
    auto client_handle = client_discardable_manager_.GetHandle(it->second);
    DCHECK(client_handle.IsValid());
    SerializableSkiaHandle handle(handle_id, client_handle.shm_id(),
                                  client_handle.byte_offset());
    serializer.Write<SerializableSkiaHandle>(&handle);
  }

  // Serialize all locked handle ids, so the raster unlocks them when done.
  const size_t num_locked_handles = locked_handles_.size();
  serializer.Write<size_t>(&num_locked_handles);
  for (auto handle_id : locked_handles_)
    serializer.Write<SkDiscardableHandleId>(&handle_id);

  // Serialize skia data.
  const size_t skia_data_size = strike_data.size();
  serializer.Write<size_t>(&skia_data_size);
  serializer.WriteData(strike_data.data(), strike_data.size(),
                       alignof(std::max_align_t));

  // Reset all state for what has been serialized.
  last_serialized_handle_id_ = last_allocated_handle_id_;
  locked_handles_.clear();
  return;
}

}  // namespace raster
}  // namespace gpu
