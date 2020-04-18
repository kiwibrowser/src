// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settime', function() {
  /**
   * TimeSetter handles a dialog to check and set system time. It can also
   * include a timezone dropdown if timezoneId is provided.
   *
   * TimeSetter uses the system time to populate the controls initially and
   * update them as the system time or timezone changes, and notifies Chrome
   * when the user changes the time or timezone.
   * @constructor
   */
  function TimeSetter() {}

  cr.addSingletonGetter(TimeSetter);

  /** @const */ var BODY_PADDING_PX = 20;
  /** @const */ var LABEL_PADDING_PX = 5;

  TimeSetter.prototype = {
    /**
     * Performs initial setup.
     */
    initialize: function() {
      // Store values for reverting inputs when the user's date/time is invalid.
      this.prevValues_ = {};

      // The build time doesn't include a timezone, so subtract 1 day to get a
      // safe minimum date.
      this.minDate_ = new Date(loadTimeData.getValue('buildTime'));
      this.minDate_.setDate(this.minDate_.getDate() - 1);

      // Set the max date to the min date plus 20 years.
      this.maxDate_ = new Date(this.minDate_);
      this.maxDate_.setYear(this.minDate_.getFullYear() + 20);

      // Make sure the ostensible date is within this range.
      var now = new Date();
      if (now > this.maxDate_)
        this.maxDate_ = now;
      else if (now < this.minDate_)
        this.minDate_ = now;

      $('date').setAttribute('min', this.toHtmlValues_(this.minDate_).date);
      $('date').setAttribute('max', this.toHtmlValues_(this.maxDate_).date);

      this.updateTime_();

      // Show the timezone select if we have a timezone ID.
      var currentTimezoneId = loadTimeData.getValue('currentTimezoneId');
      if (currentTimezoneId) {
        this.setTimezone_(currentTimezoneId);
        $('timezone-select')
            .addEventListener(
                'change', this.onTimezoneChange_.bind(this), false);
        $('timezone').hidden = false;
      }

      this.sizeToFit_();

      $('time').addEventListener('blur', this.onTimeBlur_.bind(this), false);
      $('date').addEventListener('blur', this.onTimeBlur_.bind(this), false);

      $('set-time')
          .addEventListener('submit', this.onSubmit_.bind(this), false);
    },

    /**
     * Sets the current timezone.
     * @param {string} timezoneId The timezone ID to select.
     * @private
     */
    setTimezone_: function(timezoneId) {
      $('timezone-select').value = timezoneId;
      this.updateTime_();
    },

    /**
     * Updates the date/time controls to the current local time.
     * Called initially, then called again once a minute.
     * @private
     */
    updateTime_: function() {
      var now = new Date();

      // Only update time controls if neither is focused.
      if (document.activeElement.id != 'date' &&
          document.activeElement.id != 'time') {
        var htmlValues = this.toHtmlValues_(now);
        this.prevValues_.date = $('date').value = htmlValues.date;
        this.prevValues_.time = $('time').value = htmlValues.time;
      }

      window.clearTimeout(this.timeTimeout_);

      // Start timer to update these inputs every minute.
      var secondsRemaining = 60 - now.getSeconds();
      this.timeTimeout_ = window.setTimeout(
          this.updateTime_.bind(this), secondsRemaining * 1000);
    },

    /**
     * Sets the system time from the UI.
     * @private
     */
    applyTime_: function() {
      var date = $('date').valueAsDate;
      date.setMilliseconds(date.getMilliseconds() + $('time').valueAsNumber);

      // Add timezone offset to get real time.
      date.setMinutes(date.getMinutes() + date.getTimezoneOffset());

      var seconds = Math.floor(date / 1000);
      chrome.send('setTimeInSeconds', [seconds]);
    },

    /**
     * Called when focus is lost on date/time controls.
     * @param {Event} e The blur event.
     * @private
     */
    onTimeBlur_: function(e) {
      if (e.target.validity.valid && e.target.value) {
        // Make this the new fallback time in case of future invalid input.
        this.prevValues_[e.target.id] = e.target.value;
        this.applyTime_();
      } else {
        // Restore previous value.
        e.target.value = this.prevValues_[e.target.id];
      }
    },

    /**
     * @param {Event} e The change event.
     * @private
     */
    onTimezoneChange_: function(e) {
      chrome.send('setTimezone', [e.currentTarget.value]);
    },

    /**
     * Closes the dialog window.
     * @param {Event} e The submit event.
     * @private
     */
    onSubmit_: function(e) {
      e.preventDefault();
      chrome.send('dialogClose');
    },

    /**
     * Resizes the window if necessary to show the entire contents.
     * @private
     */
    sizeToFit_: function() {
      // Because of l10n, we should check that the vertical content can fit
      // within the window.
      if (window.innerHeight < document.body.scrollHeight) {
        // Resize window to fit scrollHeight and the title bar.
        var newHeight = document.body.scrollHeight + window.outerHeight -
            window.innerHeight;
        window.resizeTo(window.outerWidth, newHeight);
      }
    },

    /**
     * Builds date and time strings suitable for the values of HTML date and
     * time elements.
     * @param {Date} date The date object to represent.
     * @return {{date: string, time: string}} Date is an RFC 3339 formatted date
     *     and time is an HH:MM formatted time.
     * @private
     */
    toHtmlValues_: function(date) {
      // Get the current time and subtract the timezone offset, so the
      // JSON string is in local time.
      var localDate = new Date(date);
      localDate.setMinutes(date.getMinutes() - date.getTimezoneOffset());
      return {
        date: localDate.toISOString().slice(0, 10),
        time: localDate.toISOString().slice(11, 16)
      };
    },
  };

  TimeSetter.setTimezone = function(timezoneId) {
    TimeSetter.getInstance().setTimezone_(timezoneId);
  };

  TimeSetter.updateTime = function() {
    TimeSetter.getInstance().updateTime_();
  };

  return {TimeSetter: TimeSetter};
});

document.addEventListener('DOMContentLoaded', function() {
  settime.TimeSetter.getInstance().initialize();
});
