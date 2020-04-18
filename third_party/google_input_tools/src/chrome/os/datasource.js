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
goog.provide('i18n.input.chrome.DataSource');

goog.require('goog.events');
goog.require('goog.events.Event');
goog.require('goog.events.EventTarget');
goog.require('goog.functions');


goog.scope(function() {
var Event = goog.events.Event;
var EventTarget = goog.events.EventTarget;



/**
 * The data source.
 *
 * @param {number} numOfCanddiate The number of canddiate to fetch.
 * @param {function(string, !Array.<!Object>)} candidatesCallback .
 * @param {function(!Object)} gestureCallback .
 * @constructor
 * @extends {EventTarget}
 */
i18n.input.chrome.DataSource = function(numOfCanddiate, candidatesCallback,
    gestureCallback) {
  goog.base(this);

  /**
   * The number of candidates to fetch.
   *
   * @type {number}
   */
  this.numOfCandidate = numOfCanddiate;

  /** @protected {function(string, !Array.<!Object>)} */
  this.candidatesCallback = candidatesCallback;

  /** @protected {function(!Object)} */
  this.gestureCallback = gestureCallback;
};
var DataSource = i18n.input.chrome.DataSource;
goog.inherits(DataSource, EventTarget);


/**
 * The event type.
 *
 * @enum {string}
 */
DataSource.EventType = {
  CANDIDATES_BACK: goog.events.getUniqueId('cb'),
  GESTURES_BACK: goog.events.getUniqueId('gb'),
  READY: goog.events.getUniqueId('r')
};


/** @type {boolean} */
DataSource.prototype.ready = false;


/**
 * The correction level.
 *
 * @protected {number}
 */
DataSource.prototype.correctionLevel = 0;


/**
 * Whether user dict is enabled.
 *
 * @protected {boolean}
 */
DataSource.prototype.enableUserDict = false;


/**
 * The language code.
 *
 * @type {string}
 * @protected
 */
DataSource.prototype.language;


/**
 * Sets the langauge code.
 *
 * @param {string} language The language code.
 */
DataSource.prototype.setLanguage = function(language) {
  this.language = language;
};


/**
 * True if the datasource is ready.
 *
 * @return {boolean} .
 */
DataSource.prototype.isReady = function() {
  return this.ready;
};


/**
 * Creates the common payload for completion or prediction request.
 *
 * @return {!Object} The payload.
 * @protected
 */
DataSource.prototype.createCommonPayload = function() {
  return {
    'itc': this.getInputToolCode(),
    'num': this.numOfCandidate
  };
};


/**
 * Gets the input tool code.
 *
 * @return {string} .
 */
DataSource.prototype.getInputToolCode = function() {
  return this.language + '-t-i0-und';
};


/**
 * Sends completion request for a word.
 *
 * @param {string} word The word.
 * @param {string} context The context.
 */
DataSource.prototype.sendCompletionRequestForWord = goog.functions.NULL;


/**
 * Sends completion request.
 *
 * @param {string} query The query .
 * @param {string} context The context .
 * @param {!Object=} opt_spatialData .
 */
DataSource.prototype.sendCompletionRequest = goog.functions.NULL;


/**
 * Enables/disables user dictionary.
 *
 * @param {boolean} enabled
 */
DataSource.prototype.setEnableUserDict = function(enabled) {
  this.enableUserDict = enabled;
};


/**
 * Sends prediciton request.
 *
 * @param {string} context The context.
 */
DataSource.prototype.sendPredictionRequest = goog.functions.NULL;


/**
 * Sets the correction level.
 *
 * @param {number} level .
 */
DataSource.prototype.setCorrectionLevel = function(level) {
  this.correctionLevel = level;
};


/**
 * Changes frequency of word in the data source.
 *
 * @param {string} word The word to commit/change.
 * @param {number} frequency The change in frequency.
 */
DataSource.prototype.changeWordFrequency = goog.functions.NULL;


/**
 * Clears the data source.
 */
DataSource.prototype.clear = goog.functions.NULL;



/**
 * The candidates are fetched back.
 *
 * @param {string} source The source.
 * @param {!Array.<!Object>} candidates The candidates.
 * @constructor
 * @extends {Event}
 */
DataSource.CandidatesBackEvent = function(source, candidates) {
  DataSource.CandidatesBackEvent.base(
      this, 'constructor', DataSource.EventType.CANDIDATES_BACK);

  /**
   * The source.
   *
   * @type {string}
   */
  this.source = source;

  /**
   * The candidate list.
   *
   * @type {!Array.<!Object>}
   */
  this.candidates = candidates;
};
goog.inherits(DataSource.CandidatesBackEvent, Event);



/**
 * The gesture response is fetched back.
 *
 * @param {!{results: !Array<string>, commit: boolean}} response The response
 *     from the gesture decoder.
 * @constructor
 * @extends {Event}
 */
DataSource.GesturesBackEvent = function(response) {
  DataSource.GesturesBackEvent.base(
      this, 'constructor', DataSource.EventType.GESTURES_BACK);

  /**
   * The gesture response.
   *
   * @type {!{results: !Array<string>, commit: boolean}}
   */
  this.response = response;
};
goog.inherits(DataSource.GesturesBackEvent, Event);

});  // goog.scope
