// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.
#include "build/build_config.h"

#include "ipc/ipc_message_macros.h"
#define IPC_MESSAGE_START IPCTestMsgStart

#if defined(OS_WIN)
#include "base/memory/shared_memory_handle.h"
#include "ipc/handle_win.h"

IPC_MESSAGE_CONTROL3(TestHandleWinMsg, int, IPC::HandleWin, int)
IPC_MESSAGE_CONTROL2(TestTwoHandleWinMsg, IPC::HandleWin, IPC::HandleWin)
IPC_MESSAGE_CONTROL1(TestSharedMemoryHandleMsg1, base::SharedMemoryHandle)
#endif  // defined(OS_WIN)

#if defined(OS_MACOSX)
#include "base/file_descriptor_posix.h"
#include "base/memory/shared_memory_handle.h"

IPC_MESSAGE_CONTROL3(TestSharedMemoryHandleMsg1,
                     int,
                     base::SharedMemoryHandle,
                     int)
IPC_MESSAGE_CONTROL2(TestSharedMemoryHandleMsg2,
                     base::SharedMemoryHandle,
                     base::SharedMemoryHandle)
IPC_MESSAGE_CONTROL4(TestSharedMemoryHandleMsg3,
                     base::FileDescriptor,
                     base::SharedMemoryHandle,
                     base::FileDescriptor,
                     base::SharedMemoryHandle)
IPC_MESSAGE_CONTROL1(TestSharedMemoryHandleMsg4, int)

#endif  // defined(OS_MACOSX)
