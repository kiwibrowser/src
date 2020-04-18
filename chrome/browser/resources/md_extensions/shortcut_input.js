// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @enum {number} */
const ShortcutError = {
  NO_ERROR: 0,
  INCLUDE_START_MODIFIER: 1,
  TOO_MANY_MODIFIERS: 2,
  NEED_CHARACTER: 3,
};

cr.define('extensions', function() {
  'use strict';

  // The UI to display and manage keyboard shortcuts set for extension commands.
  const ShortcutInput = Polymer({
    is: 'extensions-shortcut-input',

    properties: {
      /** @type {!extensions.KeyboardShortcutDelegate} */
      delegate: Object,

      item: {
        type: String,
        value: '',
      },

      commandName: {
        type: String,
        value: '',
      },

      shortcut: {
        type: String,
        value: '',
      },

      /** @private */
      capturing_: {
        type: Boolean,
        value: false,
      },

      /** @private {!ShortcutError} */
      error_: {
        type: Number,
        value: 0,
      },

      /** @private */
      pendingShortcut_: {
        type: String,
        value: '',
      },
    },

    /** @override */
    ready: function() {
      const node = this.$['input'];
      node.addEventListener('mouseup', this.startCapture_.bind(this));
      node.addEventListener('blur', this.endCapture_.bind(this));
      node.addEventListener('focus', this.startCapture_.bind(this));
      node.addEventListener('keydown', this.onKeyDown_.bind(this));
      node.addEventListener('keyup', this.onKeyUp_.bind(this));
    },

    /** @private */
    startCapture_: function() {
      if (this.capturing_)
        return;
      this.capturing_ = true;
      this.delegate.setShortcutHandlingSuspended(true);
    },

    /** @private */
    endCapture_: function() {
      if (!this.capturing_)
        return;
      this.pendingShortcut_ = '';
      this.capturing_ = false;
      const input = this.$.input;
      input.blur();
      input.invalid = false;
      this.delegate.setShortcutHandlingSuspended(false);
    },

    /**
     * @param {!KeyboardEvent} e
     * @private
     */
    onKeyDown_: function(e) {
      if (e.keyCode == extensions.Key.Escape) {
        if (!this.capturing_) {
          // If we're not currently capturing, allow escape to propagate.
          return;
        }
        // Otherwise, escape cancels capturing.
        this.endCapture_();
        e.preventDefault();
        e.stopPropagation();
        return;
      }
      if (e.keyCode == extensions.Key.Tab) {
        // Allow tab propagation for keyboard navigation.
        return;
      }

      if (!this.capturing_)
        this.startCapture_();

      this.handleKey_(e);
    },

    /**
     * @param {!KeyboardEvent} e
     * @private
     */
    onKeyUp_: function(e) {
      if (e.keyCode == extensions.Key.Escape || e.keyCode == extensions.Key.Tab)
        return;

      this.handleKey_(e);
    },

    /**
     * @param {!ShortcutError} error
     * @param {string} includeStartModifier
     * @param {string} tooManyModifiers
     * @param {string} needCharacter
     * @return {string} UI string.
     * @private
     */
    getErrorString_: function(
        error, includeStartModifier, tooManyModifiers, needCharacter) {
      if (error == ShortcutError.TOO_MANY_MODIFIERS)
        return tooManyModifiers;
      if (error == ShortcutError.NEED_CHARACTER)
        return needCharacter;
      return includeStartModifier;
    },

    /**
     * @param {!KeyboardEvent} e
     * @private
     */
    handleKey_: function(e) {
      // While capturing, we prevent all events from bubbling, to prevent
      // shortcuts lacking the right modifier (F3 for example) from activating
      // and ending capture prematurely.
      e.preventDefault();
      e.stopPropagation();

      // We don't allow both Ctrl and Alt in the same keybinding.
      // TODO(devlin): This really should go in extensions.hasValidModifiers,
      // but that requires updating the existing page as well.
      if (e.ctrlKey && e.altKey) {
        this.error_ = ShortcutError.TOO_MANY_MODIFIERS;
        this.$.input.invalid = true;
        return;
      }
      if (!extensions.hasValidModifiers(e)) {
        this.pendingShortcut_ = '';
        this.error_ = ShortcutError.INCLUDE_START_MODIFIER;
        this.$.input.invalid = true;
        return;
      }
      this.pendingShortcut_ = extensions.keystrokeToString(e);
      if (!extensions.isValidKeyCode(e.keyCode)) {
        this.error_ = ShortcutError.NEED_CHARACTER;
        this.$.input.invalid = true;
        return;
      }
      this.$.input.invalid = false;

      this.commitPending_();
      this.endCapture_();
    },

    /** @private */
    commitPending_: function() {
      this.shortcut = this.pendingShortcut_;
      this.delegate.updateExtensionCommandKeybinding(
          this.item, this.commandName, this.shortcut);
    },

    /**
     * @return {string} The text to be displayed in the shortcut field.
     * @private
     */
    computeText_: function() {
      let shortcutString =
          this.capturing_ ? this.pendingShortcut_ : this.shortcut;
      return shortcutString.split('+').join(' + ');
    },

    /**
     * @return {boolean} Whether the clear button is hidden.
     * @private
     */
    computeClearHidden_: function() {
      // We don't want to show the clear button if the input is currently
      // capturing a new shortcut or if there is no shortcut to clear.
      return this.capturing_ || !this.shortcut;
    },

    /** @private */
    onClearTap_: function() {
      if (this.shortcut) {
        this.pendingShortcut_ = '';
        this.commitPending_();
      }
    },
  });

  return {ShortcutInput: ShortcutInput};
});
