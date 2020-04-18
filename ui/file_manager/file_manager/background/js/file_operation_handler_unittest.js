// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

// Mock items.
var fileOperationManager = null;
var progressCenter = null;

// Test target.
var handler = null;

// Set up the test components.
function setUp() {
  // Set up string assets.
  loadTimeData.data = {
    COPY_FILE_NAME: 'Copying $1...',
    COPY_TARGET_EXISTS_ERROR: '$1 is already exists.',
    COPY_FILESYSTEM_ERROR: 'Copy filesystem error: $1',
    FILE_ERROR_GENERIC: 'File error generic.',
    COPY_UNEXPECTED_ERROR: 'Copy unexpected error: $1'
  };

  // Make ProgressCenterHandler.
  fileOperationManager = new MockFileOperationManager();
  progressCenter = new MockProgressCenter();
  handler = new FileOperationHandler(fileOperationManager, progressCenter);
}

// Test for success copy.
function testCopySuccess() {
  // Dispatch an event.
  fileOperationManager.dispatchEvent({
    type: 'copy-progress',
    taskId: 'TASK_ID',
    reason: fileOperationUtil.EventRouter.EventType.BEGIN,
    status: {
      operationType: 'COPY',
      numRemainingItems: 1,
      processingEntryName: 'sample.txt',
      totalBytes: 200,
      processedBytes: 0
    }
  });

  // Check the updated item.
  var item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.PROGRESSING, item.state);
  assertEquals('TASK_ID', item.id);
  assertEquals('Copying sample.txt...', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);

  // Dispatch an event.
  fileOperationManager.dispatchEvent({
    type: 'copy-progress',
    taskId: 'TASK_ID',
    reason: fileOperationUtil.EventRouter.EventType.SUCCESS,
    status: {
      operationType: 'COPY'
    }
  });

  // Check the updated item.
  item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.COMPLETED, item.state);
  assertEquals('TASK_ID', item.id);
  assertEquals('', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(100, item.progressRateInPercent);
}

// Test for copy cancel.
function testCopyCancel() {
  // Dispatch an event.
  fileOperationManager.dispatchEvent({
    type: 'copy-progress',
    taskId: 'TASK_ID',
    reason: fileOperationUtil.EventRouter.EventType.BEGIN,
    status: {
      operationType: 'COPY',
      numRemainingItems: 1,
      processingEntryName: 'sample.txt',
      totalBytes: 200,
      processedBytes: 0
    }
  });

  // Check the updated item.
  var item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.PROGRESSING, item.state);
  assertEquals('Copying sample.txt...', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);

  // Dispatch an event.
  fileOperationManager.cancelEvent = {
    type: 'copy-progress',
    taskId: 'TASK_ID',
    reason: fileOperationUtil.EventRouter.EventType.CANCELED,
    status: {
      operationType: 'COPY'
    }
  };
  item.cancelCallback();

  // Check the updated item.
  item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.CANCELED, item.state);
  assertEquals('', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}

// Test for copy target exists error.
function testCopyTargetExistsError() {
  // Dispatch an event.
  fileOperationManager.dispatchEvent({
    type: 'copy-progress',
    taskId: 'TASK_ID',
    reason: fileOperationUtil.EventRouter.EventType.ERROR,
    status: {
      operationType: 'COPY'
    },
    error: {
      code: util.FileOperationErrorType.TARGET_EXISTS,
      data: {name: 'sample.txt'}
    }
  });

  // Check the updated item.
  var item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.ERROR, item.state);
  assertEquals('sample.txt is already exists.', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}

// Test for copy file system error.
function testCopyFileSystemError() {
  // Dispatch an event.
  fileOperationManager.dispatchEvent({
    type: 'copy-progress',
    taskId: 'TASK_ID',
    reason: fileOperationUtil.EventRouter.EventType.ERROR,
    status: {
      operationType: 'COPY'
    },
    error: {
      code: util.FileOperationErrorType.FILESYSTEM_ERROR,
      data: {code: ''}
    }
  });

  // Check the updated item.
  var item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.ERROR, item.state);
  assertEquals('Copy filesystem error: File error generic.', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}

// Test for copy unexpected error.
function testCopyUnexpectedError() {
  // Dispatch an event.
  fileOperationManager.dispatchEvent({
    type: 'copy-progress',
    taskId: 'TASK_ID',
    reason: fileOperationUtil.EventRouter.EventType.ERROR,
    status: {
      operationType: 'COPY'
    },
    error: {
      code: 'Unexpected',
      data: {name: 'sample.txt'}
    }
  });

  // Check the updated item.
  var item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.ERROR, item.state);
  assertEquals('Copy unexpected error: Unexpected', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}
