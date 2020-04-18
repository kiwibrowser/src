// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"

#include <stdint.h>

#include <utility>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/process_memory_dump.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

class BitmapData : public base::RefCountedThreadSafe<BitmapData> {
 public:
  explicit BitmapData(size_t buffer_size) : buffer_size(buffer_size) {}
  std::unique_ptr<base::SharedMemory> memory;
  size_t buffer_size;

 private:
  friend class base::RefCountedThreadSafe<BitmapData>;
  ~BitmapData() {}
  DISALLOW_COPY_AND_ASSIGN(BitmapData);
};

namespace {

class ServerSharedBitmap : public SharedBitmap {
 public:
  ServerSharedBitmap(scoped_refptr<BitmapData> bitmap_data,
                     const SharedBitmapId& id)
      : SharedBitmap(static_cast<uint8_t*>(bitmap_data->memory->memory()),
                     id,
                     0 /* sequence_number */),
        bitmap_data_(std::move(bitmap_data)) {}

  ~ServerSharedBitmap() override {
  }

  // SharedBitmap implementation.
  base::UnguessableToken GetCrossProcessGUID() const override {
    return bitmap_data_->memory->mapped_id();
  }

 private:
  scoped_refptr<BitmapData> bitmap_data_;
};

}  // namespace

base::LazyInstance<ServerSharedBitmapManager>::DestructorAtExit
    g_shared_memory_manager = LAZY_INSTANCE_INITIALIZER;

ServerSharedBitmapManager::ServerSharedBitmapManager() = default;

ServerSharedBitmapManager::~ServerSharedBitmapManager() {
  DCHECK(handle_map_.empty());
}

ServerSharedBitmapManager* ServerSharedBitmapManager::current() {
  return g_shared_memory_manager.Pointer();
}

std::unique_ptr<SharedBitmap> ServerSharedBitmapManager::GetSharedBitmapFromId(
    const gfx::Size& size,
    ResourceFormat format,
    const SharedBitmapId& id) {
  base::AutoLock lock(lock_);
  auto it = handle_map_.find(id);
  if (it == handle_map_.end())
    return nullptr;

  BitmapData* data = it->second.get();

  size_t bitmap_size;
  if (!ResourceSizes::MaybeSizeInBytes(size, format, &bitmap_size) ||
      bitmap_size > data->buffer_size)
    return nullptr;

  if (!data->memory->memory()) {
    return nullptr;
  }

  return std::make_unique<ServerSharedBitmap>(data, id);
}

bool ServerSharedBitmapManager::ChildAllocatedSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const SharedBitmapId& id) {
  base::SharedMemoryHandle memory_handle;
  size_t buffer_size;
  MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(buffer), &memory_handle, &buffer_size, nullptr);
  DCHECK_EQ(result, MOJO_RESULT_OK);

  auto data = base::MakeRefCounted<BitmapData>(buffer_size);
  data->memory = std::make_unique<base::SharedMemory>(memory_handle, false);
  // Map the memory to get a pointer to it, then close it to free up the fd so
  // it can be reused. This doesn't unmap the memory. Some OS have a very
  // limited number of fds and this avoids consuming them all.
  data->memory->Map(data->buffer_size);
  data->memory->Close();

  base::AutoLock lock(lock_);
  if (handle_map_.find(id) != handle_map_.end())
    return false;
  handle_map_[id] = std::move(data);
  return true;
}

bool ServerSharedBitmapManager::ChildAllocatedSharedBitmapForTest(
    size_t buffer_size,
    const base::SharedMemoryHandle& memory_handle,
    const SharedBitmapId& id) {
  auto data = base::MakeRefCounted<BitmapData>(buffer_size);
  data->memory = std::make_unique<base::SharedMemory>(memory_handle, false);
  if (!data->memory->Map(data->buffer_size))
    return false;
  data->memory->Close();

  base::AutoLock lock(lock_);
  if (handle_map_.find(id) != handle_map_.end())
    return false;
  handle_map_[id] = std::move(data);
  return true;
}

void ServerSharedBitmapManager::ChildDeletedSharedBitmap(
    const SharedBitmapId& id) {
  base::AutoLock lock(lock_);
  handle_map_.erase(id);
}

bool ServerSharedBitmapManager::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  base::AutoLock lock(lock_);

  for (const auto& pair : handle_map_) {
    const SharedBitmapId& id = pair.first;
    BitmapData* data = pair.second.get();

    std::string dump_str = base::StringPrintf(
        "sharedbitmap/%s", base::HexEncode(id.name, sizeof(id.name)).c_str());
    base::trace_event::MemoryAllocatorDump* dump =
        pmd->CreateAllocatorDump(dump_str);
    if (!dump)
      return false;

    dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                    base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                    data->buffer_size);

    if (data->memory) {
      // Resources from a client have shared memory, and we use the guid from
      // that.
      base::UnguessableToken shared_memory_guid = data->memory->mapped_id();
      DCHECK(!shared_memory_guid.is_empty());
      pmd->CreateSharedMemoryOwnershipEdge(dump->guid(), shared_memory_guid,
                                           0 /* importance*/);
    } else {
      // Otherwise, resources were allocated locally for in-process use, and
      // there is no shared memory. Instead make up a GUID for them.
      auto guid = GetSharedBitmapGUIDForTracing(id);
      pmd->CreateSharedGlobalAllocatorDump(guid);
      pmd->AddOwnershipEdge(dump->guid(), guid);
    }
  }

  return true;
}

size_t ServerSharedBitmapManager::AllocatedBitmapCount() const {
  base::AutoLock lock(lock_);
  return handle_map_.size();
}

void ServerSharedBitmapManager::FreeSharedMemoryFromMap(
    const SharedBitmapId& id) {
  base::AutoLock lock(lock_);
  handle_map_.erase(id);
}

}  // namespace viz
