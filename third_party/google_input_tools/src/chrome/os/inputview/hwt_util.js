// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//

/**
 * @fileoverview Defines the class i18n.input.hwt.util.
 * @author fengyuan@google.com (Feng Yuan)
 */

goog.provide('i18n.input.hwt.util');


goog.require('goog.array');
goog.require('goog.events.EventType');
goog.require('i18n.input.common.dom');


/**
 * Listens MOUSEUP on page scope.
 *
 * @param {goog.events.EventHandler} eventHandler The event handler.
 * @param {Document} topDocument The top document.
 * @param {goog.events.EventType | !Array.<goog.events.EventType>} eventType
 *     The event type to listen.
 * @param {Function} callback The callback method.
 */
i18n.input.hwt.util.listenPageEvent = function(eventHandler, topDocument,
    eventType, callback) {
  // Ideally we'd just listen for MOUSEUP on the canvas, but that
  // doesn't work if the mouseup event happens outside the canvas,
  // and in particular if it happens in an iframe.  So we have to
  // listen on the document and any embedded iframes.  We'll also
  // use this handler to cancel auto-repeat when holding down
  // the backspace button, in case the user releases the button
  // elsewhere.
  eventHandler.listen(topDocument, goog.events.EventType.MOUSEUP,
      callback, true);
  goog.array.forEach(i18n.input.common.dom.getSameDomainDocuments(topDocument),
      function(frameDoc) {
        try {
          // In IE and FF3.5, some iframe is not allowed to access.
          // It throws exception when access the iframe property.
          eventHandler.listen(frameDoc, goog.events.EventType.MOUSEUP,
              callback, true);
        } catch (e) {}
      });
};


/**
 * The default candidate map of handwriting pad.
 * Key: language code.
 * Value: candidate list.
 *
 * @type {!Object.<string, !Array.<string>>}
 */
i18n.input.hwt.util.DEFAULT_CANDIDATE_MAP = {
  '': [',', '.', '?', '!', ':', '\'', '"', ';', '@'],
  'es': [',', '.', '¿', '?', '¡', '!', ':', '\'', '"'],
  'ja': ['，', '。', '？', '！', '：', '「', '」', '；'],
  'zh-Hans': ['，', '。', '？', '！', '：', '“', '”', '；'],
  'zh-Hant': ['，', '。', '？', '！', '：', '「', '」', '；']
};

