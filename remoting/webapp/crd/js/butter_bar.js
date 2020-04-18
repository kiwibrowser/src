// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * ButterBar class that is used to show a butter bar with deprecation messages.
 * Each message is displayed for at most one week.
 */

/** @suppress {duplicate} */
var remoting = remoting || {};

(function() {

'use strict';

/**
 * @param {function(!remoting.ChromotingEvent)} writeLogFunction Callback for
 *    reporting telemetry events to the backend.
 * @constructor
 */
remoting.ButterBar = function(writeLogFunction) {
  /** @private @const */
  this.messages_ = [
    {id: /*i18n-content*/'WEBSITE_INVITE_BETA', dismissable: true},
    {id: /*i18n-content*/'WEBSITE_INVITE_STABLE', dismissable: true},
    {id: /*i18n-content*/'WEBSITE_INVITE_DEPRECATION_1', dismissable: true},
    {id: /*i18n-content*/'WEBSITE_INVITE_DEPRECATION_2', dismissable: false},
  ];
  // TODO(jamiewalch): Read the message index using metricsPrivate.
  this.currentMessage_ = -1;
  /** @private {!Element} */
  this.root_ = document.getElementById(remoting.ButterBar.kId_);
  /** @private {!Element} */
  this.message_ = document.getElementById(remoting.ButterBar.kMessageId_);
  /** @private {!Element} */
  this.dismiss_ = document.getElementById(remoting.ButterBar.kDismissId_);
  /** @private {function(!remoting.ChromotingEvent)} */
  this.writeLogFunction_ = writeLogFunction;
}

/** @return {!remoting.ButterBar} */
remoting.ButterBar.create  = function() {
  return new remoting.ButterBar(remoting.TelemetryEventWriter.Client.write);
};

remoting.ButterBar.prototype.init = function() {
  var result = new base.Deferred();
  var xhr = new remoting.Xhr({
    method: 'GET',
    url: remoting.ButterBar.kMessageIndexUrl_,
    acceptJson: true,
    headers: { 'Content-Type': 'text/plain' },
  });
  var promises = [
    xhr.start(),
    remoting.identity.getEmail(),
  ];
  Promise.all(promises).then(function (results) {
    /** @type {!remoting.Xhr.Response} */
    var response = results[0];
    /** @type {string} */
    var email = results[1];
    if (email.toLocaleLowerCase().endsWith('@google.com')) {
      result.resolve();
      return;
    }
    try {
      var responseObj = response.getJson();
      /** @type {number} The index of the message to display. */
      var index = base.assertNumber(responseObj.index);
      /** @type {string} The URL of the website. */
      var url = base.escapeHTML(base.assertString(responseObj.url));
      /** @type {number} The percentage of users to whom to show the message. */
      var percent = base.assertNumber(responseObj.percent);
      if (isFinite(index)) {
        var hash = remoting.ButterBar.hash_(email);
        if (100 * hash / 127 <= percent) {
          this.currentMessage_ =
              Math.min(Number(index), this.messages_.length - 1);
        }
      }
      if (this.currentMessage_ > -1) {
        chrome.storage.sync.get(
            [remoting.ButterBar.kStorageKey_],
            (syncValues) => {
              this.onStateLoaded_(syncValues, String(url));
              result.resolve();
            });
      } else {
        result.resolve();
      }
    } catch (err) {
      console.error('Error parsing JSON:', response.getText(), err);
      result.resolve();
    }
  }.bind(this));
  return result.promise();
}

/**
 * Shows butter bar with the current message.
 *
 * @param {string} url
 * @private
 */
remoting.ButterBar.prototype.show_ = function(url) {
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;
  this.reportTelemetry_(MigrationEvent.DEPRECATION_NOTICE_IMPRESSION);

  var messageId = this.messages_[this.currentMessage_].id;
  var substitutions = [`<a href="${url}" target="_blank">`, '</a>'];
  var dismissable = this.messages_[this.currentMessage_].dismissable;
  l10n.localizeElementFromTag(this.message_, messageId, substitutions, true);

  var anchorTags = this.message_.getElementsByTagName('a');
  if (anchorTags.length == 1) {
    anchorTags[0].addEventListener(
        'click', this.onLinkClicked_.bind(this), false);
  }
  if (dismissable) {
    this.dismiss_.addEventListener('click', this.dismiss.bind(this), false);
  } else {
    this.dismiss_.hidden = true;
    this.root_.classList.add('red');
  }
  this.root_.hidden = false;
}

/**
 * @param {remoting.ChromotingEvent.ChromotingDotComMigration.Event} eventType
 * @private
 */
remoting.ButterBar.prototype.reportTelemetry_ = function(eventType) {
  var event =
      new remoting.ChromotingEvent(
          remoting.ChromotingEvent.Type.CHROMOTING_DOT_COM_MIGRATION);
  event.chromoting_dot_com_migration =
      new remoting.ChromotingEvent.ChromotingDotComMigration(
          eventType, this.getMigrationPhase_());
  event.role = remoting.ChromotingEvent.Role.CLIENT;
  this.writeLogFunction_(event);
};

/**
 * @return {remoting.ChromotingEvent.ChromotingDotComMigration.Phase}
 * @private
 */
remoting.ButterBar.prototype.getMigrationPhase_ = function() {
  var Phase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase
  switch (this.currentMessage_) {
    case 0:
      return Phase.BETA;
    case 1:
      return Phase.STABLE;
    case 2:
      return Phase.DEPRECATION_1;
    case 3:
      return Phase.DEPRECATION_2;
    default:
      return Phase.UNSPECIFIED_PHASE;
  }
};

/**
 * @param {Object} syncValues
 * @param {string} url The website URL.
 * @private
 */
remoting.ButterBar.prototype.onStateLoaded_ = function(syncValues, url) {
  /** @type {!Object|undefined} */
  var messageState = syncValues[remoting.ButterBar.kStorageKey_];
  if (!messageState) {
    messageState = {
      index: -1,
      timestamp: remoting.ButterBar.now_(),
      hidden: false,
    }
  }

  // Show the current message unless it was explicitly dismissed or if it was
  // first shown more than a week ago. If it is marked as not dismissable, show
  // it unconditionally.
  var elapsed = remoting.ButterBar.now_() - messageState.timestamp;
  var show =
      this.currentMessage_ > messageState.index ||
      !this.messages_[this.currentMessage_].dismissable ||
      (!messageState.hidden && elapsed <= remoting.ButterBar.kTimeout_);

  if (show) {
    this.show_(url);
    // If this is the first time this message is being displayed, update the
    // saved state.
    if (this.currentMessage_ > messageState.index) {
      var value = {};
      value[remoting.ButterBar.kStorageKey_] = {
        index: this.currentMessage_,
        timestamp: remoting.ButterBar.now_(),
        hidden: false
      };
      chrome.storage.sync.set(value);
    }
  }
};

/**
 * Click handler of the migration link.
 *
 * @param {Event} e
 * @private
 */
remoting.ButterBar.prototype.onLinkClicked_ = function(e) {
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;
  this.reportTelemetry_(MigrationEvent.DEPRECATION_NOTICE_CLICKED);
};

/**
 * Hide the butter bar request and record the message that was being displayed.
 */
remoting.ButterBar.prototype.dismiss = function() {
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;
  this.reportTelemetry_(MigrationEvent.DEPRECATION_NOTICE_DISMISSAL);
  var value = {};
  value[remoting.ButterBar.kStorageKey_] = {
    index: this.currentMessage_,
    timestamp: remoting.ButterBar.now_(),
    hidden: true
  };
  chrome.storage.sync.set(value);

  this.root_.hidden = true;
};

/**
 * Get current time for testing. Note that this is called in the then() clause
 * of a Promise, which Sinon doesn't handle correctly.
 *
 * @return {number} The current time in milliseconds since the epoch.
 * @private
 */
remoting.ButterBar.now_ = function() {
  return Date.now();
};

/**
 * @param {string} str
 * @return {number} a simple hash of the |str| in the range [0, 127]
 * @private
 */
remoting.ButterBar.hash_ = function(str) {
  var hash = 0;
  for (var i = 0; i < str.length; i++) {
    var chr = str.charCodeAt(i);
    hash ^= chr;
  }
  return hash & 0x7f;
};

/** @const @private */
remoting.ButterBar.kId_ = 'butter-bar';

/** @const @private */
remoting.ButterBar.kMessageId_ = 'butter-bar-message';
/** @const @private */
remoting.ButterBar.kDismissId_ = 'butter-bar-dismiss';

/** @const @private */
remoting.ButterBar.kStorageKey_ = 'message-state';

/** @const @private */
remoting.ButterBar.kMessageIndexUrl_ =
    'https://www.gstatic.com/chromoting/website_invite_message_index.json';

/** @const @private */
remoting.ButterBar.kTimeout_ = 7 * 24 * 60 * 60 * 1000;   // 1 week

})();
