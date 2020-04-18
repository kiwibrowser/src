// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('settings');

/**
 * The steps in the fingerprint setup flow.
 * @enum {number}
 */
settings.FingerprintSetupStep = {
  LOCATE_SCANNER: 1,  // The user needs to locate the scanner.
  MOVE_FINGER: 2,     // The user needs to move finger around the scanner.
  READY: 3            // The scanner has read the fingerprint successfully.
};

(function() {

/**
 * The duration in ms of a fingerprint icon flash when a user touches the
 * fingerprint sensor during an enroll session.
 * @type {number}
 */
const FLASH_DURATION_MS = 300;

/**
 * The amount of milliseconds after a successful but not completed scan before a
 * message shows up telling the user to scan their finger again.
 * @type {number}
 */
const SHOW_TAP_SENSOR_MESSAGE_DELAY_MS = 2000;

/**
 * The estimated amount of complete scans needed to enroll a fingerprint. Used
 * to help us estimate the progress of an enroll session.
 * TODO(xiaoyinh@): This will be replaced by percentage of completion in the
 * future.
 * @type {number}
 */
const SUCCESSFUL_SCANS_TO_COMPLETE = 15;

Polymer({
  is: 'settings-setup-fingerprint-dialog',

  behaviors: [I18nBehavior, WebUIListenerBehavior],

  properties: {
    /**
     * The problem message to display.
     * @private
     */
    problemMessage_: {
      type: String,
      value: '',
    },

    /**
     * The setup phase we are on.
     * @type {!settings.FingerprintSetupStep}
     * @private
     */
    step_: {type: Number, value: settings.FingerprintSetupStep.LOCATE_SCANNER},
  },

  /**
   * The number of scans that have been received during setup. This is used to
   * approximate the progress of the setup.
   * @type {number}
   * @private
   */
  receivedScanCount_: 0,

  /**
   * A message shows after the user has not scanned a finger during setup. This
   * is the set timeout id.
   * @type {number}
   * @private
   */
  tapSensorMessageTimeoutId_: 0,

  /** @private {?settings.FingerprintBrowserProxy}*/
  browserProxy_: null,

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'on-fingerprint-scan-received', this.onScanReceived_.bind(this));
    this.browserProxy_ = settings.FingerprintBrowserProxyImpl.getInstance();

    this.$.arc.clearCanvas();
    this.$.arc.drawBackgroundCircle();
    this.$.arc.drawShadow(10, 0, 0);
    this.browserProxy_.startEnroll();
    this.$.dialog.showModal();
  },

  /**
   * Closes the dialog.
   */
  close: function() {
    if (this.$.dialog.open)
      this.$.dialog.close();

    // Note: Reset resets |step_| back to the default, so handle anything that
    // checks |step_| before resetting.
    if (this.step_ == settings.FingerprintSetupStep.READY)
      this.fire('add-fingerprint');
    else
      this.browserProxy_.cancelCurrentEnroll();

    this.reset_();
  },

  /** private */
  clearSensorMessageTimeout_: function() {
    if (this.tapSensorMessageTimeoutId_ != 0) {
      clearTimeout(this.tapSensorMessageTimeoutId_);
      this.tapSensorMessageTimeoutId_ = 0;
    }
  },

  /**
   * Resets the dialog to its start state. Call this when the dialog gets
   * closed.
   * @private
   */
  reset_: function() {
    this.step_ = settings.FingerprintSetupStep.LOCATE_SCANNER;
    this.receivedScanCount_ = 0;
    this.$.arc.clearCanvas();
    this.clearSensorMessageTimeout_();
  },

  /**
   * Closes the dialog.
   * @private
   */
  onClose_: function() {
    if (this.$.dialog.open)
      this.$.dialog.close();
  },

  /**
   * Advances steps, shows problems and animates the progress as needed based on
   * scan results.
   * @param {!settings.FingerprintScan} scan
   * @private
   */
  onScanReceived_: function(scan) {
    switch (this.step_) {
      case settings.FingerprintSetupStep.LOCATE_SCANNER:
      case settings.FingerprintSetupStep.MOVE_FINGER:
        if (this.step_ == settings.FingerprintSetupStep.LOCATE_SCANNER) {
          // Clear canvas because there will be shadows present at this step.
          this.$.arc.clearCanvas();
          this.$.arc.drawBackgroundCircle();

          this.step_ = settings.FingerprintSetupStep.MOVE_FINGER;
          this.receivedScanCount_ = 0;
        }
        const slice = 2 * Math.PI / SUCCESSFUL_SCANS_TO_COMPLETE;
        if (scan.isComplete) {
          this.problemMessage_ = '';
          this.step_ = settings.FingerprintSetupStep.READY;
          this.$.arc.animateProgress(
              this.receivedScanCount_ * slice, 2 * Math.PI);
          this.clearSensorMessageTimeout_();
        } else {
          this.setProblem_(scan.result);
          if (scan.result == settings.FingerprintResultType.SUCCESS) {
            this.problemMessage_ = '';
            // Flash the fingerprint icon blue so that users get some feedback
            // when a successful scan has been registered.
            this.$.image.animate(
                {
                  fill: ['var(--google-blue-700)', 'var(--google-grey-500)'],
                  opacity: [0.7, 1.0],
                },
                FLASH_DURATION_MS);
            this.$.arc.animateProgress(
                this.receivedScanCount_ * slice,
                (this.receivedScanCount_ + 1) * slice);
            this.receivedScanCount_++;
          }
        }
        break;
      case settings.FingerprintSetupStep.READY:
        break;
      default:
        assertNotReached();
        break;
    }
  },

  /**
   * Sets the instructions based on which phase of the fingerprint setup we are
   * on.
   * @param {!settings.FingerprintSetupStep} step The current step the
   *     fingerprint setup is on.
   * @private
   */
  getInstructionMessage_: function(step) {
    switch (step) {
      case settings.FingerprintSetupStep.LOCATE_SCANNER:
        return this.i18n('configureFingerprintInstructionLocateScannerStep');
      case settings.FingerprintSetupStep.MOVE_FINGER:
        return this.i18n('configureFingerprintInstructionMoveFingerStep');
      case settings.FingerprintSetupStep.READY:
        return this.i18n('configureFingerprintInstructionReadyStep');
    }
    assertNotReached();
  },

  /**
   * Set the problem message based on the result from the fingerprint scanner.
   * @param {!settings.FingerprintResultType} scanResult The result the
   *     fingerprint scanner gives.
   * @private
   */
  setProblem_: function(scanResult) {
    this.clearSensorMessageTimeout_();
    switch (scanResult) {
      case settings.FingerprintResultType.SUCCESS:
        this.problemMessage_ = '';
        this.tapSensorMessageTimeoutId_ = setTimeout(() => {
          this.problemMessage_ = this.i18n('configureFingerprintLiftFinger');
        }, SHOW_TAP_SENSOR_MESSAGE_DELAY_MS);
        break;
      case settings.FingerprintResultType.PARTIAL:
        this.problemMessage_ = this.i18n('configureFingerprintPartialData');
        break;
      case settings.FingerprintResultType.INSUFFICIENT:
        this.problemMessage_ =
            this.i18n('configureFingerprintInsufficientData');
        break;
      case settings.FingerprintResultType.SENSOR_DIRTY:
        this.problemMessage_ = this.i18n('configureFingerprintSensorDirty');
        break;
      case settings.FingerprintResultType.TOO_SLOW:
        this.problemMessage_ = this.i18n('configureFingerprintTooSlow');
        break;
      case settings.FingerprintResultType.TOO_FAST:
        this.problemMessage_ = this.i18n('configureFingerprintTooFast');
        break;
      case settings.FingerprintResultType.IMMOBILE:
        this.problemMessage_ = this.i18n('configureFingerprintImmobile');
        break;
      default:
        assertNotReached();
        break;
    }
  },

  /**
   * Displays the text of the close button based on which phase of the
   * fingerprint setup we are on.
   * @param {!settings.FingerprintSetupStep} step The current step the
   *     fingerprint setup is on.
   * @private
   */
  getCloseButtonText_: function(step) {
    if (step == settings.FingerprintSetupStep.READY)
      return this.i18n('done');

    return this.i18n('cancel');
  },

  /**
   * @param {!settings.FingerprintSetupStep} step
   * @private
   */
  getCloseButtonClass_: function(step) {
    if (step == settings.FingerprintSetupStep.READY)
      return 'action-button';

    return 'cancel-button';
  },

  /**
   * @param {!settings.FingerprintSetupStep} step
   * @private
   */
  hideAddAnother_: function(step) {
    return step != settings.FingerprintSetupStep.READY;
  },

  /**
   * Enrolls the finished fingerprint and sets the dialog back to step one to
   * prepare to enroll another fingerprint.
   * @private
   */
  onAddAnotherFingerprint_: function() {
    this.fire('add-fingerprint');
    this.reset_();
    this.$.arc.drawBackgroundCircle();
    this.$.arc.drawShadow(10, 0, 0);
    this.browserProxy_.startEnroll();
  },
});
})();
