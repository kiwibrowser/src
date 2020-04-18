// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_SURFACE_TRANSPORT_DIB_H_
#define UI_SURFACE_TRANSPORT_DIB_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "build/build_config.h"
#include "ui/surface/surface_export.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include <memory>

class SkCanvas;

// -----------------------------------------------------------------------------
// A TransportDIB is a block of memory that is used to transport pixels
// between processes: from the renderer process to the browser, and
// between renderer and plugin processes.
// -----------------------------------------------------------------------------
class SURFACE_EXPORT TransportDIB {
 public:
  ~TransportDIB();

  // A Handle is the type which can be sent over the wire so that the remote
  // side can map the transport DIB.
  typedef base::SharedMemoryHandle Handle;

  // Map the referenced transport DIB.  The caller owns the returned object.
  // Returns NULL on failure.
  static TransportDIB* Map(Handle transport_dib);

  // Create a new |TransportDIB| with a handle to the shared memory. This
  // always returns a valid pointer. The DIB is not mapped.
  static TransportDIB* CreateWithHandle(Handle handle);

  // Returns true if the handle is valid.
  static bool is_valid_handle(Handle dib);

  // Returns a canvas using the memory of this TransportDIB. The returned
  // pointer will be owned by the caller. The bitmap will be of the given size,
  // which should fit inside this memory. Bitmaps returned will be either
  // opaque or have premultiplied alpha.
  //
  // On POSIX, this |TransportDIB| will be mapped if not already. On Windows,
  // this |TransportDIB| will NOT be mapped and should not be mapped prior,
  // because PlatformCanvas will map the file internally.
  //
  // Will return NULL on allocation failure. This could be because the image
  // is too large to map into the current process' address space.
  std::unique_ptr<SkCanvas> GetPlatformCanvas(int w, int h, bool opaque);

  // Map the DIB into the current process if it is not already. This is used to
  // map a DIB that has already been created. Returns true if the DIB is mapped.
  bool Map();

  // Return a pointer to the shared memory.
  void* memory() const;

  // Return the maximum size of the shared memory. This is not the amount of
  // data which is valid, you have to know that via other means, this is simply
  // the maximum amount that /could/ be valid.
  size_t size() const { return size_; }

  // Returns a pointer to the SharedMemory object that backs the transport dib.
  base::SharedMemory* shared_memory();

 private:
  TransportDIB();

  // Verifies that the dib can hold a canvas of the requested dimensions.
  bool VerifyCanvasSize(int w, int h);

  explicit TransportDIB(base::SharedMemoryHandle dib);
  base::SharedMemory shared_memory_;
  size_t size_;  // length, in bytes

  DISALLOW_COPY_AND_ASSIGN(TransportDIB);
};

#endif  // UI_SURFACE_TRANSPORT_DIB_H_
