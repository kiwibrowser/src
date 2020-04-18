// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.embed.Config');

goog.require('goog.ui.Component');
goog.require('ink.util');
goog.require('protos.research.ink.InkEvent');



/**
 * @constructor
 * @struct
 */
ink.embed.Config = function() {
  /**
   * The parent element to render into (required).
   * @type {?Element}
   */
  this.parentEl = null;

  /**
   * The parent component to set (optional);
   * TODO(esrauch): This is only necessary because of the cross-package events
   * that are currently going in both directions. Remove this from the config
   * after this is cleaned up to avoid Whiteboard events being listened to
   * directly in Embed code.
   * @type {?goog.ui.Component}
   */
  this.parentComponent = null;

  /**
   * If true, allows ink to show its own error dialogs for certain cases.
   * @type {boolean}
   */
  this.allowDialogs = false;

  /**
   * Path to NaCl binary.
   *
   * If you are using the Native Client build, you must specify the url for the
   * Ink Native Client NMF file.
   *
   * @type {?string}
   */
  this.nativeClientManifestUrl = null;

  /**
   * The source of the embedder.
   *
   * From //logs/proto/research/ink/ink_event.proto
   *
   * @type {protos.research.ink.InkEvent.Host}
   */
  this.logsHost = protos.research.ink.InkEvent.Host.UNKNOWN_HOST;

  /**
   * The type of the document the SEngine should be constructed with.
   *
   * For Brix documents, this should be PASSTHROUGH_DOCUMENT.
   *
   * @type {ink.util.SEngineType}
   */
  this.sengineType =
      ink.util.SEngineType.PASSTHROUGH_DOCUMENT;
};
