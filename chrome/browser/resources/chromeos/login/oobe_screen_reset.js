// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Device reset screen implementation.
 */

login.createScreen('ResetScreen', 'reset', function() {
  var USER_ACTION_CANCEL_RESET = 'cancel-reset';
  var USER_ACTION_RESTART_PRESSED = 'restart-pressed';
  var USER_ACTION_LEARN_MORE_PRESSED = 'learn-more-link';
  var USER_ACTION_SHOW_CONFIRMATION = 'show-confirmation';
  var USER_ACTION_POWERWASH_PRESSED = 'powerwash-pressed';
  var USER_ACTION_RESET_CONFIRM_DISMISSED = 'reset-confirm-dismissed';
  var CONTEXT_KEY_ROLLBACK_AVAILABLE = 'rollback-available';
  var CONTEXT_KEY_ROLLBACK_CHECKED = 'rollback-checked';
  var CONTEXT_KEY_TPM_FIRMWARE_UPDATE_AVAILABLE =
      'tpm-firmware-update-available';
  var CONTEXT_KEY_TPM_FIRMWARE_UPDATE_CHECKED = 'tpm-firmware-update-checked';
  var CONTEXT_KEY_TPM_FIRMWARE_UPDATE_EDITABLE = 'tpm-firmware-update-editable';
  var CONTEXT_KEY_IS_OFFICIAL_BUILD = 'is-official-build';
  var CONTEXT_KEY_IS_CONFIRMATIONAL_VIEW = 'is-confirmational-view';
  var CONTEXT_KEY_SCREEN_STATE = 'screen-state';

  return {

    /* Possible UI states of the reset screen. */
    RESET_SCREEN_UI_STATE: {
      REVERT_PROMISE: 'ui-state-revert-promise',
      RESTART_REQUIRED: 'ui-state-restart-required',
      POWERWASH_PROPOSAL: 'ui-state-powerwash-proposal',
      ROLLBACK_PROPOSAL: 'ui-state-rollback-proposal',
      ERROR: 'ui-state-error',
    },

    RESET_SCREEN_STATE: {
      RESTART_REQUIRED: 0,
      REVERT_PROMISE: 1,
      POWERWASH_PROPOSAL: 2,  // supports 2 ui-states
      ERROR: 3,
    },


    /** @override */
    decorate: function() {
      var self = this;

      this.declareUserAction(
          $('powerwash-help-link'),
          {action_id: USER_ACTION_LEARN_MORE_PRESSED, event: 'click'});
      this.declareUserAction(
          $('reset-confirm-dismiss'),
          {action_id: USER_ACTION_RESET_CONFIRM_DISMISSED, event: 'click'});
      this.declareUserAction(
          $('reset-confirm-commit'),
          {action_id: USER_ACTION_POWERWASH_PRESSED, event: 'click'});

      this.context.addObserver(CONTEXT_KEY_SCREEN_STATE, function(state) {
        if (Oobe.getInstance().currentScreen != this) {
          setTimeout(function() {
            Oobe.resetSigninUI(false);
            Oobe.showScreen({id: SCREEN_OOBE_RESET});
          }, 0);
        }
        if (state == self.RESET_SCREEN_STATE.RESTART_REQUIRED)
          self.ui_state = self.RESET_SCREEN_UI_STATE.RESTART_REQUIRED;
        if (state == self.RESET_SCREEN_STATE.REVERT_PROMISE)
          self.ui_state = self.RESET_SCREEN_UI_STATE.REVERT_PROMISE;
        else if (state == self.RESET_SCREEN_STATE.POWERWASH_PROPOSAL)
          self.ui_state = self.RESET_SCREEN_UI_STATE.POWERWASH_PROPOSAL;
        self.setDialogView_();
        if (state == self.RESET_SCREEN_STATE.REVERT_PROMISE) {
          announceAccessibleMessage(
              loadTimeData.getString('resetRevertSpinnerMessage'));
        }
        self.setTPMFirmwareUpdateView_();
      });

      this.context.addObserver(
          CONTEXT_KEY_IS_OFFICIAL_BUILD, function(isOfficial) {
            $('powerwash-help-link').setAttribute('hidden', !isOfficial);
            $('oobe-reset-md').isOfficial_ = isOfficial;
          });
      this.context.addObserver(
          CONTEXT_KEY_ROLLBACK_CHECKED, function(rollbackChecked) {
            self.setRollbackOptionView();
          });
      this.context.addObserver(
          CONTEXT_KEY_ROLLBACK_AVAILABLE, function(rollbackAvailable) {
            self.setRollbackOptionView();
          });
      this.context.addObserver(
          CONTEXT_KEY_TPM_FIRMWARE_UPDATE_CHECKED, function() {
            self.setTPMFirmwareUpdateView_();
          });
      this.context.addObserver(
          CONTEXT_KEY_TPM_FIRMWARE_UPDATE_EDITABLE, function() {
            self.setTPMFirmwareUpdateView_();
          });
      this.context.addObserver(
          CONTEXT_KEY_TPM_FIRMWARE_UPDATE_AVAILABLE, function() {
            self.setTPMFirmwareUpdateView_();
          });
      this.context.addObserver(
          CONTEXT_KEY_IS_CONFIRMATIONAL_VIEW, function(is_confirmational) {
            if (is_confirmational) {
              console.log(self.context.get(CONTEXT_KEY_SCREEN_STATE, 0));
              if (self.context.get(CONTEXT_KEY_SCREEN_STATE, 0) !=
                  self.RESET_SCREEN_STATE.POWERWASH_PROPOSAL)
                return;
              console.log(self);
              reset.ConfirmResetOverlay.getInstance().initializePage();
              if (!$('reset-confirm-overlay-md').hidden)
                $('reset-confirm-overlay-md').showModal();
            } else {
              $('overlay-reset').setAttribute('hidden', true);
              if ($('reset-confirm-overlay-md').open)
                $('reset-confirm-overlay-md').close();
            }
          });

      $('oobe-reset-md').screen = this;
    },

    /**
     * Header text of the screen.
     * @type {string}
     */
    get header() {
      return loadTimeData.getString('resetScreenTitle');
    },

    /**
     * Buttons in oobe wizard's button strip.
     * @type {array} Array of Buttons.
     */
    get buttons() {
      var buttons = [];
      var restartButton = this.ownerDocument.createElement('button');
      restartButton.id = 'reset-restart-button';
      restartButton.textContent = loadTimeData.getString('resetButtonRestart');
      this.declareUserAction(
          restartButton,
          {action_id: USER_ACTION_RESTART_PRESSED, event: 'click'});
      buttons.push(restartButton);

      // Button that leads to confirmation pop-up dialog.
      var toConfirmButton = this.ownerDocument.createElement('button');
      toConfirmButton.id = 'reset-toconfirm-button';
      toConfirmButton.textContent =
          loadTimeData.getString('resetButtonPowerwash');
      this.declareUserAction(
          toConfirmButton,
          {action_id: USER_ACTION_SHOW_CONFIRMATION, event: 'click'});
      buttons.push(toConfirmButton);

      var cancelButton = this.ownerDocument.createElement('button');
      cancelButton.id = 'reset-cancel-button';
      cancelButton.textContent = loadTimeData.getString('cancelButton');
      this.declareUserAction(
          cancelButton, {action_id: USER_ACTION_CANCEL_RESET, event: 'click'});
      buttons.push(cancelButton);

      return buttons;
    },

    /**
     * Returns a control which should receive an initial focus.
     */
    get defaultControl() {
      // choose
      if (this.isMDMode_())
        return $('oobe-reset-md');
      if (this.context.get(
              CONTEXT_KEY_SCREEN_STATE,
              this.RESET_SCREEN_STATE.RESTART_REQUIRED) ==
          this.RESET_SCREEN_STATE.RESTART_REQUIRED)
        return $('reset-restart-button');
      if (this.context.get(CONTEXT_KEY_IS_CONFIRMATIONAL_VIEW, false))
        return $('reset-confirm-commit');
      return $('reset-toconfirm-button');
    },

    /**
     * Cancels the reset and drops the user back to the login screen.
     */
    cancel: function() {
      if (this.context.get(CONTEXT_KEY_IS_CONFIRMATIONAL_VIEW, false)) {
        $('reset').send(
            login.Screen.CALLBACK_USER_ACTED,
            USER_ACTION_RESET_CONFIRM_DISMISSED);
        return;
      }
      this.send(login.Screen.CALLBACK_USER_ACTED, USER_ACTION_CANCEL_RESET);
    },

    /**
     * This method takes care of switching to material-design OOBE.
     * @private
     */
    setMDMode_: function() {
      var useMDOobe = this.isMDMode_();
      $('oobe-reset-md').hidden = !useMDOobe;
      $('reset-confirm-overlay-md').hidden = !useMDOobe;
      $('oobe-reset').hidden = useMDOobe;
      $('reset-confirm-overlay').hidden = useMDOobe;
      if (useMDOobe) {
        $('reset').setAttribute('md-mode', 'true');
        $('overlay-reset').setAttribute('md-mode', 'true');
      } else {
        $('reset').removeAttribute('md-mode');
        $('overlay-reset').removeAttribute('md-mode');
      }
    },

    /**
     * Returns if material-design flag is used.
     * @private
     */
    isMDMode_: function() {
      return loadTimeData.getString('newOobeUI') == 'on';
    },

    /**
     * Event handler that is invoked just before the screen in shown.
     * @param {Object} data Screen init payload.
     */
    onBeforeShow: function(data) {
      this.setMDMode_();
    },

    /**
     * Sets css style for corresponding state of the screen.
     * @private
     */
    setDialogView_: function(state) {
      state = this.ui_state;
      var resetOverlay = $('reset-confirm-overlay');
      this.classList.toggle(
          'revert-promise-view',
          state == this.RESET_SCREEN_UI_STATE.REVERT_PROMISE);
      this.classList.toggle(
          'restart-required-view',
          state == this.RESET_SCREEN_UI_STATE.RESTART_REQUIRED);
      this.classList.toggle(
          'powerwash-proposal-view',
          state == this.RESET_SCREEN_UI_STATE.POWERWASH_PROPOSAL);
      resetOverlay.classList.toggle(
          'powerwash-proposal-view',
          state == this.RESET_SCREEN_UI_STATE.POWERWASH_PROPOSAL);
      this.classList.toggle(
          'rollback-proposal-view',
          state == this.RESET_SCREEN_UI_STATE.ROLLBACK_PROPOSAL);
      resetOverlay.classList.toggle(
          'rollback-proposal-view',
          state == this.RESET_SCREEN_UI_STATE.ROLLBACK_PROPOSAL);
      var resetMd = $('oobe-reset-md');
      var resetOverlayMd = $('reset-confirm-overlay-md');
      if (state == this.RESET_SCREEN_UI_STATE.RESTART_REQUIRED) {
        resetMd.uiState_ = 'restart-required-view';
      }
      if (state == this.RESET_SCREEN_UI_STATE.POWERWASH_PROPOSAL) {
        resetMd.uiState_ = 'powerwash-proposal-view';
        resetOverlayMd.isPowerwashView_ = true;
      }
      if (state == this.RESET_SCREEN_UI_STATE.ROLLBACK_PROPOSAL) {
        resetMd.uiState_ = 'rollback-proposal-view';
        resetOverlayMd.isPowerwashView_ = false;
      }
      if (state == this.RESET_SCREEN_UI_STATE.REVERT_PROMISE) {
        resetMd.uiState_ = 'revert-promise-view';
      }
    },

    setRollbackOptionView: function() {
      if (this.context.get(CONTEXT_KEY_IS_CONFIRMATIONAL_VIEW, false))
        return;
      if (this.context.get(CONTEXT_KEY_SCREEN_STATE) !=
          this.RESET_SCREEN_STATE.POWERWASH_PROPOSAL)
        return;

      if (this.context.get(CONTEXT_KEY_ROLLBACK_AVAILABLE, false) &&
          this.context.get(CONTEXT_KEY_ROLLBACK_CHECKED, false)) {
        // show rollback option
        $('reset-toconfirm-button').textContent =
            loadTimeData.getString('resetButtonPowerwashAndRollback');
        this.ui_state = this.RESET_SCREEN_UI_STATE.ROLLBACK_PROPOSAL;
      } else {
        // hide rollback option
        $('reset-toconfirm-button').textContent =
            loadTimeData.getString('resetButtonPowerwash');
        this.ui_state = this.RESET_SCREEN_UI_STATE.POWERWASH_PROPOSAL;
      }
      this.setDialogView_();
      this.setTPMFirmwareUpdateView_();
    },

    setTPMFirmwareUpdateView_: function() {
      $('oobe-reset-md').tpmFirmwareUpdateAvailable_ =
          this.ui_state == this.RESET_SCREEN_UI_STATE.POWERWASH_PROPOSAL &&
          this.context.get(CONTEXT_KEY_TPM_FIRMWARE_UPDATE_AVAILABLE);
      $('oobe-reset-md').tpmFirmwareUpdateChecked_ =
          this.context.get(CONTEXT_KEY_TPM_FIRMWARE_UPDATE_CHECKED);
      $('oobe-reset-md').tpmFirmwareUpdateEditable_ =
          this.context.get(CONTEXT_KEY_TPM_FIRMWARE_UPDATE_EDITABLE);
    },

    onTPMFirmwareUpdateChanged_: function(value) {
      this.context.set(CONTEXT_KEY_TPM_FIRMWARE_UPDATE_CHECKED, value);
      this.commitContextChanges();
    }
  };
});
