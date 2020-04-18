// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Namespace for custom types.
 * @namespace
 */
unpacker.types = {};

/** @typedef {string} */
unpacker.types.FileSystemId;

/** @typedef {number} */
unpacker.types.RequestId;

/** @typedef {number} */
unpacker.types.CompressorId;

/** @typedef {number} */
unpacker.types.EntryId;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onUnmountRequested
 * @typedef {!Object<{fileSystemId: !unpacker.types.FileSystemId,
 *                    requestId: !unpacker.types.RequestId}>}
 */
unpacker.types.UnmountRequestedOptions;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onGetMetadataRequested
 * @typedef {!Object<{fileSystemId: !unpacker.types.FileSystemId,
 *                    requestId: !unpacker.types.RequestId,
 *                    entryPath: string,
 *                    thumbnail: boolean}>}
 */
unpacker.types.GetMetadataRequestedOptions;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onReadDirectoryRequested
 * @typedef {!Object<{fileSystemId: !unpacker.types.FileSystemId,
 *                    requestId: !unpacker.types.RequestId,
 *                    directoryPath: string}>}
 */
unpacker.types.ReadDirectoryRequestedOptions;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onOpenFileRequested
 * @typedef {!Object<{fileSystemId: !unpacker.types.FileSystemId,
 *                    requestId: !unpacker.types.RequestId,
 *                    filePath: string,
 *                    mode: !OpenFileMode}>}
 */
unpacker.types.OpenFileRequestedOptions;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onCloseFileRequested
 * @typedef {!Object<{fileSystemId: !unpacker.types.FileSystemId,
 *                    requestId: !unpacker.types.RequestId,
 *                    openRequestId: !unpacker.types.RequestId}>}
 */
unpacker.types.CloseFileRequestedOptions;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onReadFileRequested
 * @typedef {!Object<{fileSystemId: !unpacker.types.FileSystemId,
 *                    requestId: !unpacker.types.RequestId,
 *                    openRequestId: !unpacker.types.RequestId,
 *                    offset: number,
 *                    length: number}>}
 */
unpacker.types.ReadFileRequestedOptions;
