// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Interface on which |CommandHandler| depends.
 * @interface
 */
function CommandHandlerDeps(){};

/**
 * @type {ActionsController}
 */
CommandHandlerDeps.prototype.actionsController;

/**
 * @type {BackgroundWindow}
 */
CommandHandlerDeps.prototype.backgroundPage;

/**
 * @type {DialogType}
 */
CommandHandlerDeps.prototype.dialogType;

/**
 * @type {DirectoryModel}
 */
CommandHandlerDeps.prototype.directoryModel;

/**
 * @type {DirectoryTree}
 */
CommandHandlerDeps.prototype.directoryTree;

/**
 * @type {DirectoryTreeNamingController}
 */
CommandHandlerDeps.prototype.directoryTreeNamingController;

/**
 * @type {Document}
 */
CommandHandlerDeps.prototype.document;

/**
 * @type {FileFilter}
 */
CommandHandlerDeps.prototype.fileFilter;

/**
 * @type {FileOperationManager}
 */
CommandHandlerDeps.prototype.fileOperationManager;

/**
 * @type {FileTransferController}
 */
CommandHandlerDeps.prototype.fileTransferController;

/**
 * @type {FileSelectionHandler}
 */
CommandHandlerDeps.prototype.selectionHandler;

/**
 * @type {NamingController}
 */
CommandHandlerDeps.prototype.namingController;

/**
 * @type {ProvidersModel}
 */
CommandHandlerDeps.prototype.providersModel;

/**
 * @type {SpinnerController}
 */
CommandHandlerDeps.prototype.spinnerController;

/**
 * @type {TaskController}
 */
CommandHandlerDeps.prototype.taskController;

/**
 * @type {FileManagerUI}
 */
CommandHandlerDeps.prototype.ui;

/**
 * @type {VolumeManagerWrapper}
 */
CommandHandlerDeps.prototype.volumeManager;

/**
 * @return {DirectoryEntry|FakeEntry}
 */
CommandHandlerDeps.prototype.getCurrentDirectoryEntry = function() {};

/**
 * @return {FileSelection}
 */
CommandHandlerDeps.prototype.getSelection = function() {};
