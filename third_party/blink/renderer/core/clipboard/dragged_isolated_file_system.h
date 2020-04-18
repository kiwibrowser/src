// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CLIPBOARD_DRAGGED_ISOLATED_FILE_SYSTEM_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CLIPBOARD_DRAGGED_ISOLATED_FILE_SYSTEM_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class DataObject;

class CORE_EXPORT DraggedIsolatedFileSystem {
 public:
  DraggedIsolatedFileSystem() = default;
  virtual ~DraggedIsolatedFileSystem() = default;

  using FileSystemIdPreparationCallback = void (*)(DataObject*);
  static void Init(FileSystemIdPreparationCallback);

  static void PrepareForDataObject(DataObject*);

 private:
  static FileSystemIdPreparationCallback prepare_callback_;

  DISALLOW_COPY_AND_ASSIGN(DraggedIsolatedFileSystem);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CLIPBOARD_DRAGGED_ISOLATED_FILE_SYSTEM_H_
