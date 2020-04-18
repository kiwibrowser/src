// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/plugin_array_buffer_var.h"

#include <stdlib.h>

#include <limits>

#include "base/memory/shared_memory.h"
#include "ppapi/c/dev/ppb_buffer_dev.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/plugin_globals.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_structs.h"
#include "ppapi/shared_impl/host_resource.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_buffer_api.h"

using base::SharedMemory;
using base::SharedMemoryHandle;
using ppapi::proxy::PluginGlobals;
using ppapi::proxy::PluginResourceTracker;

namespace ppapi {

PluginArrayBufferVar::PluginArrayBufferVar(uint32_t size_in_bytes)
    : buffer_(size_in_bytes),
      size_in_bytes_(size_in_bytes) {}

PluginArrayBufferVar::PluginArrayBufferVar(uint32_t size_in_bytes,
                                           SharedMemoryHandle plugin_handle)
    : plugin_handle_(plugin_handle), size_in_bytes_(size_in_bytes) {}

PluginArrayBufferVar::~PluginArrayBufferVar() {
  Unmap();

  if (shmem_.get() == NULL) {
    // The SharedMemory destuctor can't close the handle for us.
    if (SharedMemory::IsHandleValid(plugin_handle_))
      SharedMemory::CloseHandle(plugin_handle_);
  } else {
    // Delete SharedMemory, if we have one.
    shmem_.reset();
  }
}

void* PluginArrayBufferVar::Map() {
  if (shmem_.get())
    return shmem_->memory();
  if (SharedMemory::IsHandleValid(plugin_handle_)) {
    shmem_.reset(new SharedMemory(plugin_handle_, false));
    if (!shmem_->Map(size_in_bytes_)) {
      shmem_.reset();
      return NULL;
    }
    return shmem_->memory();
  }
  if (buffer_.empty())
    return NULL;
  return &(buffer_[0]);
}

void PluginArrayBufferVar::Unmap() {
  if (shmem_.get())
    shmem_->Unmap();
}

uint32_t PluginArrayBufferVar::ByteLength() {
  return size_in_bytes_;
}

bool PluginArrayBufferVar::CopyToNewShmem(
    PP_Instance instance,
    int* host_handle_id,
    SharedMemoryHandle* plugin_out_handle) {
  ppapi::proxy::PluginDispatcher* dispatcher =
      ppapi::proxy::PluginDispatcher::GetForInstance(instance);
  if (!dispatcher)
    return false;

  ppapi::proxy::SerializedHandle plugin_handle;
  dispatcher->Send(new PpapiHostMsg_SharedMemory_CreateSharedMemory(
      instance, ByteLength(), host_handle_id, &plugin_handle));
  if (!plugin_handle.IsHandleValid() || !plugin_handle.is_shmem() ||
      *host_handle_id == -1)
    return false;

  base::SharedMemoryHandle tmp_handle = plugin_handle.shmem();
  SharedMemory s(tmp_handle, false);
  if (!s.Map(ByteLength()))
    return false;
  memcpy(s.memory(), Map(), ByteLength());
  s.Unmap();

  // We don't need to keep the shared memory around on the plugin side;
  // we've already copied all our data into it. We'll make it invalid
  // just to be safe.
  *plugin_out_handle = base::SharedMemoryHandle();

  return true;
}

}  // namespace ppapi
