// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_AEC_DUMP_MESSAGES_H_
#define CONTENT_COMMON_MEDIA_AEC_DUMP_MESSAGES_H_

// IPC messages for the AEC dump.

#include "content/common/content_export.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START AecDumpMsgStart

// Messages sent from the browser to the renderer.

// The browser hands over a file handle to the consumer in the renderer
// identified by |id| to use for AEC dump.
IPC_MESSAGE_CONTROL2(AecDumpMsg_EnableAecDump,
                     int /* id */,
                     IPC::PlatformFileForTransit /* file_handle */)

// Tell the renderer to disable AEC dump in all consumers.
IPC_MESSAGE_CONTROL0(AecDumpMsg_DisableAecDump)

// TODO(hlundin): Rename file to reflect expanded use; http://crbug.com/709919.
IPC_MESSAGE_CONTROL1(AudioProcessingMsg_EnableAec3, bool /* enable */)

// Messages sent from the renderer to the browser.

// Registers a consumer with the browser. The consumer will then get a file
// handle when the dump is enabled.
IPC_MESSAGE_CONTROL1(AecDumpMsg_RegisterAecDumpConsumer,
                     int /* id */)

// Unregisters a consumer with the browser.
IPC_MESSAGE_CONTROL1(AecDumpMsg_UnregisterAecDumpConsumer,
                     int /* id */)

// Response to browser process that AEC3 was enabled.
IPC_MESSAGE_CONTROL0(AudioProcessingMsg_Aec3Enabled)

#endif  // CONTENT_COMMON_MEDIA_AEC_DUMP_MESSAGES_H_
