// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

const FILE_SYSTEM_ID = 'id';
const REQUEST_ID = 10;
const ENCODING = 'CP1250';
const ARCHIVE_SIZE = 5000;
const CHUNK_BUFFER = new ArrayBuffer(5);
const CHUNK_OFFSET = 150;
const CLOSE_VOLUME_REQUEST_ID = '-1';
const INDEX = 123;
const OPEN_REQUEST_ID = 7;
const OFFSET = 50;
const LENGTH = 200;

function testCreateReadMetadataRequest() {
  const readMetadataRequest = unpacker.request.createReadMetadataRequest(
      FILE_SYSTEM_ID, REQUEST_ID, ENCODING, ARCHIVE_SIZE);

  assertEquals(
      unpacker.request.Operation.READ_METADATA,
      readMetadataRequest[unpacker.request.Key.OPERATION]);
  assertEquals(
      FILE_SYSTEM_ID, readMetadataRequest[unpacker.request.Key.FILE_SYSTEM_ID]);
  assertEquals(
      REQUEST_ID.toString(),
      readMetadataRequest[unpacker.request.Key.REQUEST_ID]);
  assertEquals(ENCODING, readMetadataRequest[unpacker.request.Key.ENCODING]);
  assertEquals(
      ARCHIVE_SIZE.toString(),
      readMetadataRequest[unpacker.request.Key.ARCHIVE_SIZE]);
}

function testCreateReadChunkDoneResponse() {
  const readChunkDoneResponse = unpacker.request.createReadChunkDoneResponse(
      FILE_SYSTEM_ID, REQUEST_ID, CHUNK_BUFFER, CHUNK_OFFSET);

  assertEquals(
      unpacker.request.Operation.READ_CHUNK_DONE,
      readChunkDoneResponse[unpacker.request.Key.OPERATION]);
  assertEquals(
      FILE_SYSTEM_ID,
      readChunkDoneResponse[unpacker.request.Key.FILE_SYSTEM_ID]);
  assertEquals(
      REQUEST_ID.toString(),
      readChunkDoneResponse[unpacker.request.Key.REQUEST_ID]);
  assertEquals(
      CHUNK_BUFFER, readChunkDoneResponse[unpacker.request.Key.CHUNK_BUFFER]);
  assertEquals(
      CHUNK_OFFSET.toString(),
      readChunkDoneResponse[unpacker.request.Key.OFFSET]);
}

function testCreateReadChunkErrorResponse() {
  const readChunkErrorResponse = unpacker.request.createReadChunkErrorResponse(
      FILE_SYSTEM_ID, REQUEST_ID, CHUNK_BUFFER);

  assertEquals(
      unpacker.request.Operation.READ_CHUNK_ERROR,
      readChunkErrorResponse[unpacker.request.Key.OPERATION]);
  assertEquals(
      FILE_SYSTEM_ID,
      readChunkErrorResponse[unpacker.request.Key.FILE_SYSTEM_ID]);
  assertEquals(
      REQUEST_ID.toString(),
      readChunkErrorResponse[unpacker.request.Key.REQUEST_ID]);
}

function testCreateCloseVolumeRequest() {
  const closeVolumeRequest =
      unpacker.request.createCloseVolumeRequest(FILE_SYSTEM_ID);
  assertEquals(
      unpacker.request.Operation.CLOSE_VOLUME,
      closeVolumeRequest[unpacker.request.Key.OPERATION]);
  assertEquals(
      FILE_SYSTEM_ID, closeVolumeRequest[unpacker.request.Key.FILE_SYSTEM_ID]);
  assertEquals(
      CLOSE_VOLUME_REQUEST_ID,
      closeVolumeRequest[unpacker.request.Key.REQUEST_ID]);
}

function testCreateOpenFileRequest() {
  const openFileRequest = unpacker.request.createOpenFileRequest(
      FILE_SYSTEM_ID, REQUEST_ID, INDEX, ENCODING, ARCHIVE_SIZE);

  assertEquals(
      unpacker.request.Operation.OPEN_FILE,
      openFileRequest[unpacker.request.Key.OPERATION]);
  assertEquals(
      FILE_SYSTEM_ID, openFileRequest[unpacker.request.Key.FILE_SYSTEM_ID]);
  assertEquals(
      REQUEST_ID.toString(), openFileRequest[unpacker.request.Key.REQUEST_ID]);
  assertEquals(INDEX.toString(), openFileRequest[unpacker.request.Key.INDEX]);
  assertEquals(ENCODING, openFileRequest[unpacker.request.Key.ENCODING]);
  assertEquals(
      ARCHIVE_SIZE.toString(),
      openFileRequest[unpacker.request.Key.ARCHIVE_SIZE]);
}

function testCreateCloseFileRequest() {
  const closeFileRequest = unpacker.request.createCloseFileRequest(
      FILE_SYSTEM_ID, REQUEST_ID, OPEN_REQUEST_ID);

  assertEquals(
      unpacker.request.Operation.CLOSE_FILE,
      closeFileRequest[unpacker.request.Key.OPERATION]);
  assertEquals(
      FILE_SYSTEM_ID, closeFileRequest[unpacker.request.Key.FILE_SYSTEM_ID]);
  assertEquals(
      REQUEST_ID.toString(), closeFileRequest[unpacker.request.Key.REQUEST_ID]);
  assertEquals(
      OPEN_REQUEST_ID.toString(),
      closeFileRequest[unpacker.request.Key.OPEN_REQUEST_ID]);
}

function testCreateReadFileRequest() {
  const readFileRequest = unpacker.request.createReadFileRequest(
      FILE_SYSTEM_ID, REQUEST_ID, OPEN_REQUEST_ID, OFFSET, LENGTH);

  assertEquals(
      unpacker.request.Operation.READ_FILE,
      readFileRequest[unpacker.request.Key.OPERATION]);
  assertEquals(
      FILE_SYSTEM_ID, readFileRequest[unpacker.request.Key.FILE_SYSTEM_ID]);
  assertEquals(
      REQUEST_ID.toString(), readFileRequest[unpacker.request.Key.REQUEST_ID]);
  assertEquals(
      OPEN_REQUEST_ID.toString(),
      readFileRequest[unpacker.request.Key.OPEN_REQUEST_ID]);
  assertEquals(OFFSET.toString(), readFileRequest[unpacker.request.Key.OFFSET]);
  assertEquals(LENGTH.toString(), readFileRequest[unpacker.request.Key.LENGTH]);
}
