// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.UndoStateChangeEvent');

goog.require('goog.events');
goog.require('goog.events.Event');



/**
 * @param {boolean} canUndo Whether there is undo state available.
 * @param {boolean} canRedo Whether there is redo state available.
 * @constructor
 * @struct
 * @extends {goog.events.Event}
 */
ink.UndoStateChangeEvent = function(canUndo, canRedo) {
  ink.UndoStateChangeEvent.base(
      this, 'constructor',ink.UndoStateChangeEvent.EVENT_TYPE);
  this.canUndo = canUndo;
  this.canRedo = canRedo;
};
goog.inherits(ink.UndoStateChangeEvent, goog.events.Event);


/** @type {string} */
ink.UndoStateChangeEvent.EVENT_TYPE = goog.events.getUniqueId('undo-state');
