// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('GestureCommandData');

/**
 * Map from gesture names (ax::mojom::Gesture defined in
 *     ui/accessibility/ax_enums.mojom.)  to commands.
 * @type {!Object<string,
 *     {
 *     command: string,
 *     menuKeyOverride: (boolean|undefined),
 *     keyOverride: ({keyCode: number, modifiers: ({ctrl:
 * boolean}|undefined)}|undefined)
 *    }>}
 * @const
 */
GestureCommandData.GESTURE_COMMAND_MAP = {
  'click': {command: 'forceClickOnCurrentItem'},
  'swipeUp1': {
    command: 'previousLine',
    menuKeyOverride: true,
    keyOverride: {keyCode: 38 /* up */, skipStart: true, multiline: true}
  },
  'swipeDown1': {
    command: 'nextLine',
    menuKeyOverride: true,
    keyOverride: {keyCode: 40 /* Down */, skipEnd: true, multiline: true}
  },
  'swipeLeft1': {
    command: 'previousObject',
    menuKeyOverride: true,
    keyOverride: {keyCode: 37 /* left */}
  },
  'swipeRight1': {
    command: 'nextObject',
    menuKeyOverride: true,
    keyOverride: {keyCode: 39 /* right */}
  },
  'swipeUp2': {command: 'jumpToTop'},
  'swipeDown2': {command: 'readFromHere'},
  'swipeLeft2': {
    command: 'previousWord',
    keyOverride: {keyCode: 37 /* left */, modifiers: {ctrl: true}}
  },
  'swipeRight2': {
    command: 'nextWord',
    keyOverride: {keyCode: 40 /* right */, modifiers: {ctrl: true}}
  },
  'swipeUp3': {command: 'scrollBackward'},
  'swipeDown3': {command: 'scrollForward'},
  'tap2': {command: 'stopSpeech'},
  'tap4': {command: 'showPanelMenuMostRecent'},
};
