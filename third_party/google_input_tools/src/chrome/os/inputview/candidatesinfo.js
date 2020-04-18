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
goog.provide('i18n.input.chrome.inputview.CandidatesInfo');



/**
 * The candidates information.
 *
 * @param {string} source .
 * @param {!Array.<!Object>} candidates .
 * @constructor
 */
i18n.input.chrome.inputview.CandidatesInfo = function(source, candidates) {
  /** @type {string} */
  this.source = source;

  /** @type {!Array.<!Object>} */
  this.candidates = candidates;
};


/**
 * Gets an empty suggestions instance.
 *
 * @return {!i18n.input.chrome.inputview.CandidatesInfo} .
 */
i18n.input.chrome.inputview.CandidatesInfo.getEmpty = function() {
  return new i18n.input.chrome.inputview.CandidatesInfo('', []);
};

