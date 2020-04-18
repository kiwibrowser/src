// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_FILEAPI_FILE_SYSTEM_MESSAGES_H_
#define CONTENT_COMMON_FILEAPI_FILE_SYSTEM_MESSAGES_H_

// IPC messages for the file system.

#include <stdint.h>

#include <string>
#include <vector>

#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "storage/common/fileapi/file_system_info.h"
#include "storage/common/fileapi/file_system_types.h"
#include "storage/common/quota/quota_limit_type.h"
#include "url/gurl.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START FileSystemMsgStart

IPC_STRUCT_TRAITS_BEGIN(filesystem::mojom::DirectoryEntry)
  IPC_STRUCT_TRAITS_MEMBER(name)
  IPC_STRUCT_TRAITS_MEMBER(type)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(storage::FileSystemInfo)
  IPC_STRUCT_TRAITS_MEMBER(name)
  IPC_STRUCT_TRAITS_MEMBER(root_url)
  IPC_STRUCT_TRAITS_MEMBER(mount_type)
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS_MAX_VALUE(filesystem::mojom::FsFileType,
                          filesystem::mojom::FsFileType::DIRECTORY)
IPC_ENUM_TRAITS_MAX_VALUE(storage::FileSystemType,
                          storage::FileSystemType::kFileSystemTypeLast)
IPC_ENUM_TRAITS_MAX_VALUE(storage::QuotaLimitType, storage::kQuotaLimitTypeLast)

// File system messages sent from the browser to the child process.

// WebFrameClient::openFileSystem response messages.
IPC_MESSAGE_CONTROL3(FileSystemMsg_DidOpenFileSystem,
                     int /* request_id */,
                     std::string /* name */,
                     GURL /* root_url */)

// WebFileSystem response messages.
IPC_MESSAGE_CONTROL4(FileSystemMsg_DidResolveURL,
                     int /* request_id */,
                     storage::FileSystemInfo /* filesystem_info */,
                     base::FilePath /* file_path */,
                     bool /* is_directory */)
IPC_MESSAGE_CONTROL1(FileSystemMsg_DidSucceed,
                     int /* request_id */)
IPC_MESSAGE_CONTROL2(FileSystemMsg_DidReadMetadata,
                     int /* request_id */,
                     base::File::Info)
IPC_MESSAGE_CONTROL3(FileSystemMsg_DidCreateSnapshotFile,
                     int /* request_id */,
                     base::File::Info,
                     base::FilePath /* true platform path */)
IPC_MESSAGE_CONTROL3(
    FileSystemMsg_DidReadDirectory,
    int /* request_id */,
    std::vector<filesystem::mojom::DirectoryEntry> /* entries */,
    bool /* has_more */)
IPC_MESSAGE_CONTROL3(FileSystemMsg_DidWrite,
                     int /* request_id */,
                     int64_t /* byte count */,
                     bool /* complete */)
IPC_MESSAGE_CONTROL2(FileSystemMsg_DidFail,
                     int /* request_id */,
                     base::File::Error /* error_code */)

// File system messages sent from the child process to the browser.

// WebFrameClient::openFileSystem() message.
IPC_MESSAGE_CONTROL3(FileSystemHostMsg_OpenFileSystem,
                     int /* request_id */,
                     GURL /* origin_url */,
                     storage::FileSystemType /* type */)

// WevFrameClient::resolveURL() message.
IPC_MESSAGE_CONTROL2(FileSystemHostMsg_ResolveURL,
                     int /* request_id */,
                     GURL /* filesystem_url */)

// WebFileSystem::move() message.
IPC_MESSAGE_CONTROL3(FileSystemHostMsg_Move,
                     int /* request_id */,
                     GURL /* src path */,
                     GURL /* dest path */)

// WebFileSystem::copy() message.
IPC_MESSAGE_CONTROL3(FileSystemHostMsg_Copy,
                     int /* request_id */,
                     GURL /* src path */,
                     GURL /* dest path */)

// WebFileSystem::remove() message.
IPC_MESSAGE_CONTROL3(FileSystemHostMsg_Remove,
                     int /* request_id */,
                     GURL /* path */,
                     bool /* recursive */)

// WebFileSystem::readMetadata() message.
IPC_MESSAGE_CONTROL2(FileSystemHostMsg_ReadMetadata,
                     int /* request_id */,
                     GURL /* path */)

// WebFileSystem::create() message.
IPC_MESSAGE_CONTROL5(FileSystemHostMsg_Create,
                     int /* request_id */,
                     GURL /* path */,
                     bool /* exclusive */,
                     bool /* is_directory */,
                     bool /* recursive */)

// WebFileSystem::exists() messages.
IPC_MESSAGE_CONTROL3(FileSystemHostMsg_Exists,
                     int /* request_id */,
                     GURL /* path */,
                     bool /* is_directory */)

// WebFileSystem::readDirectory() message.
IPC_MESSAGE_CONTROL2(FileSystemHostMsg_ReadDirectory,
                     int /* request_id */,
                     GURL /* path */)

// WebFileWriter::write() message.
IPC_MESSAGE_CONTROL4(FileSystemHostMsg_Write,
                     int /* request id */,
                     GURL /* file path */,
                     std::string /* blob uuid */,
                     int64_t /* position */)

// WebFileWriter::truncate() message.
IPC_MESSAGE_CONTROL3(FileSystemHostMsg_Truncate,
                     int /* request id */,
                     GURL /* file path */,
                     int64_t /* length */)

// Pepper's Touch() message.
IPC_MESSAGE_CONTROL4(FileSystemHostMsg_TouchFile,
                     int /* request_id */,
                     GURL /* path */,
                     base::Time /* last_access_time */,
                     base::Time /* last_modified_time */)

// WebFileWriter::cancel() message.
IPC_MESSAGE_CONTROL2(FileSystemHostMsg_CancelWrite,
                     int /* request id */,
                     int /* id of request to cancel */)

// WebFileSystem::createSnapshotFileAndReadMetadata() message.
IPC_MESSAGE_CONTROL2(FileSystemHostMsg_CreateSnapshotFile,
                     int /* request_id */,
                     GURL /* file_path */)

// Renderers are expected to send this message after having processed
// the FileSystemMsg_DidCreateSnapshotFile message. In particular,
// after having created a BlobDataHandle backed by the snapshot file.
IPC_MESSAGE_CONTROL1(FileSystemHostMsg_DidReceiveSnapshotFile,
                     int /* request_id */)

// For Pepper's URL loader.
IPC_SYNC_MESSAGE_CONTROL1_1(FileSystemHostMsg_SyncGetPlatformPath,
                            GURL /* file path */,
                            base::FilePath /* platform_path */)

#endif  // CONTENT_COMMON_FILEAPI_FILE_SYSTEM_MESSAGES_H_
