// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// This file was automatically generated from main.soy.
// Please don't edit this file by hand.

/**
 * @fileoverview Templates in namespace ink.soy.
 * @public
 */

goog.provide('ink.soy.embedContent');

goog.require('soy');
goog.require('soydata.VERY_UNSAFE');


/**
 * @param {Object<string, *>=} opt_data
 * @param {Object<string, *>=} opt_ijData
 * @param {Object<string, *>=} opt_ijData_deprecated
 * @return {!goog.soy.data.SanitizedHtml}
 * @suppress {checkTypes}
 */
ink.soy.embedContent = function(opt_data, opt_ijData, opt_ijData_deprecated) {
  opt_ijData = opt_ijData_deprecated || opt_ijData;
  return soydata.VERY_UNSAFE.ordainSanitizedHtml(((goog.DEBUG && soy.$$debugSoyTemplateInfo) ? '<!--dta_of(ink.soy.embedContent, research/ink/web/js/main.soy, 7)-->' : '') + '<div id="canvas-parent"><style' + (opt_ijData && opt_ijData.csp_nonce ? ' nonce="' + soy.$$escapeHtmlAttribute(opt_ijData && opt_ijData.csp_nonce) + '"' : '') + '>\n        #canvas-parent {\n          height: 100%;\n          position: relative;\n          width: 100%;\n        }\n        #layer-container {\n          height: 100%;\n          position: relative;\n          width: 100%;\n        }\n        #ink-engine {\n          height: 100%;\n          left: 0;\n          position: absolute;\n          top: 0;\n          width: 100%;\n          touch-action: none;\n        }\n        .above-ink-canvas {\n          display: none;\n        }\n      </style><div class="above-ink-canvas"></div><div id="layer-container"></div><div class="below-ink-canvas"></div></div>' + ((goog.DEBUG && soy.$$debugSoyTemplateInfo) ? '<!--dta_cf(ink.soy.embedContent)-->' : ''));
};
if (goog.DEBUG) {
  ink.soy.embedContent.soyTemplateName = 'ink.soy.embedContent';
}
