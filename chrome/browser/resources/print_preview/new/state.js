// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');

/** @enum {number} */
print_preview_new.State = {
  NOT_READY: 0,
  READY: 1,
  HIDDEN: 2,
  PRINTING: 3,
  SYSTEM_DIALOG: 4,
  INVALID_TICKET: 5,
  INVALID_PRINTER: 6,
  FATAL_ERROR: 7,
  CLOSING: 8,
};

Polymer({
  is: 'print-preview-state',

  properties: {
    /** @type {print_preview_new.State} */
    state: {
      type: Number,
      notify: true,
      value: print_preview_new.State.NOT_READY,
    },
  },

  /** @param {print_preview_new.State} newState The state to transition to. */
  transitTo: function(newState) {
    switch (newState) {
      case (print_preview_new.State.NOT_READY):
        assert(
            this.state == print_preview_new.State.NOT_READY ||
            this.state == print_preview_new.State.READY ||
            this.state == print_preview_new.State.INVALID_PRINTER);
        break;
      case (print_preview_new.State.READY):
        assert(
            this.state == print_preview_new.State.INVALID_TICKET ||
            this.state == print_preview_new.State.NOT_READY ||
            this.state == print_preview_new.State.PRINTING);
        break;
      case (print_preview_new.State.HIDDEN):
        assert(this.state == print_preview_new.State.READY);
        break;
      case (print_preview_new.State.PRINTING):
        assert(
            this.state == print_preview_new.State.READY ||
            this.state == print_preview_new.State.HIDDEN);
        break;
      case (print_preview_new.State.SYSTEM_DIALOG):
        assert(
            this.state != print_preview_new.State.HIDDEN &&
            this.state != print_preview_new.State.PRINTING &&
            this.state != print_preview_new.State.CLOSING);
        break;
      case (print_preview_new.State.INVALID_TICKET):
        assert(this.state == print_preview_new.State.READY);
        break;
      case (print_preview_new.State.INVALID_PRINTER):
        assert(
            this.state == print_preview_new.State.INVALID_PRINTER ||
            this.state == print_preview_new.State.NOT_READY ||
            this.state == print_preview_new.State.READY);
        break;
      case (print_preview_new.State.CLOSING):
        assert(this.state != print_preview_new.State.HIDDEN);
        break;
    }
    this.state = newState;
  },
});
