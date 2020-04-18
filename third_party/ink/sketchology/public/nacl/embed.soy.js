// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// This file was automatically generated from embed.soy.
// Please don't edit this file by hand.

/**
 * @fileoverview Templates in namespace ink.soy.nacl.
 * @public
 */

goog.provide('ink.soy.nacl.canvasHTML');

goog.require('goog.soy.data.SanitizedContent');
goog.require('soy');
goog.require('soy.asserts');
goog.require('soydata.VERY_UNSAFE');


/**
 * @param {ink.soy.nacl.canvasHTML.Params} opt_data
 * @param {Object<string, *>=} opt_ijData
 * @param {Object<string, *>=} opt_ijData_deprecated
 * @return {!goog.soy.data.SanitizedHtml}
 * @suppress {checkTypes}
 */
ink.soy.nacl.canvasHTML = function(opt_data, opt_ijData, opt_ijData_deprecated) {
  opt_ijData = opt_ijData_deprecated || opt_ijData;
  /** @type {!goog.soy.data.SanitizedContent|string} */
  var manifestUrl = soy.asserts.assertType(goog.isString(opt_data.manifestUrl) || opt_data.manifestUrl instanceof goog.soy.data.SanitizedContent, 'manifestUrl', opt_data.manifestUrl, '!goog.soy.data.SanitizedContent|string');
  /** @type {!goog.soy.data.SanitizedContent|string} */
  var useMSAA = soy.asserts.assertType(goog.isString(opt_data.useMSAA) || opt_data.useMSAA instanceof goog.soy.data.SanitizedContent, 'useMSAA', opt_data.useMSAA, '!goog.soy.data.SanitizedContent|string');
  /** @type {!goog.soy.data.SanitizedContent|string} */
  var useSingleBuffer = soy.asserts.assertType(goog.isString(opt_data.useSingleBuffer) || opt_data.useSingleBuffer instanceof goog.soy.data.SanitizedContent, 'useSingleBuffer', opt_data.useSingleBuffer, '!goog.soy.data.SanitizedContent|string');
  /** @type {!goog.soy.data.SanitizedContent|string} */
  var sengineType = soy.asserts.assertType(goog.isString(opt_data.sengineType) || opt_data.sengineType instanceof goog.soy.data.SanitizedContent, 'sengineType', opt_data.sengineType, '!goog.soy.data.SanitizedContent|string');
  return soydata.VERY_UNSAFE.ordainSanitizedHtml(((goog.DEBUG && soy.$$debugSoyTemplateInfo) ? '<!--dta_of(ink.soy.nacl.canvasHTML, third_party/sketchology/public/nacl/embed.soy, 3)-->' : '') + '<style' + (opt_ijData && opt_ijData.csp_nonce ? ' nonce="' + soy.$$escapeHtmlAttribute(opt_ijData && opt_ijData.csp_nonce) + '"' : '') + '>\n    #ink-engine-hwoverlay {\n      display: none;\n      position: absolute;\n      width: 5px;\n      height: 5px;\n      left: 0px;\n      top: 0px;\n      /* Transforms and semi-transparent color are used to ensure the div\n       * prevents use of a hardware overlay for the underlying canvas element,\n       * despite future optimizations to the hardware overlay eligibility\n       * detection in ChromeOS.  See b/64569245 for details */\n      background-color: rgba(0, 0, 0, 0.01);\n      transform: translate3d(0.33, 0.14, 0);\n    }\n  </style><embed id="ink-engine" use_msaa="' + soy.$$escapeHtmlAttribute(useMSAA) + '" use_single_buffer="' + soy.$$escapeHtmlAttribute(useSingleBuffer) + '" src="' + soy.$$escapeHtmlAttribute(soy.$$filterNormalizeUri(manifestUrl)) + '" type="application/x-nacl" sengine_type="' + soy.$$escapeHtmlAttribute(sengineType) + '"><div id="ink-engine-hwoverlay"></div>' + ((goog.DEBUG && soy.$$debugSoyTemplateInfo) ? '<!--dta_cf(ink.soy.nacl.canvasHTML)-->' : ''));
};
/**
 * @typedef {{
 *  manifestUrl: (!goog.soy.data.SanitizedContent|string),
 *  useMSAA: (!goog.soy.data.SanitizedContent|string),
 *  useSingleBuffer: (!goog.soy.data.SanitizedContent|string),
 *  sengineType: (!goog.soy.data.SanitizedContent|string),
 * }}
 */
ink.soy.nacl.canvasHTML.Params;
if (goog.DEBUG) {
  ink.soy.nacl.canvasHTML.soyTemplateName = 'ink.soy.nacl.canvasHTML';
}
