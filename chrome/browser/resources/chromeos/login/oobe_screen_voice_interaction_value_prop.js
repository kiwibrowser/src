// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe Voice Interaction Value Prop screen implementation.
 */

login.createScreen(
    'VoiceInteractionValuePropScreen', 'voice-interaction-value-prop',
    function() {
      return {

        /** @Override */
        onBeforeShow: function(data) {
          Oobe.getInstance().headerHidden = true;
          $('voice-interaction-value-prop-md').locale =
              loadTimeData.getString('locale');
          $('voice-interaction-value-prop-md').onShow();
        }
      };
    });
