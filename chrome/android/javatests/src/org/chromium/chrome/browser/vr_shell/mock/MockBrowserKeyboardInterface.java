// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.mock;

import org.chromium.chrome.browser.vr_shell.keyboard.VrInputMethodManagerWrapper;

/**
 * Mock version of BrowserKeyboardInterface.
 */
public class MockBrowserKeyboardInterface
        implements VrInputMethodManagerWrapper.BrowserKeyboardInterface {
    /**
     * A convenience class holding text input indices.
     */
    public static class Indices {
        private final int mSelectionStart;
        private final int mSelectionEnd;
        private final int mCompositionStart;
        private final int mCompositionEnd;

        public Indices(
                int selectionStart, int selectionEnd, int compositionStart, int compositionEnd) {
            mSelectionStart = selectionStart;
            mSelectionEnd = selectionEnd;
            mCompositionStart = compositionStart;
            mCompositionEnd = compositionEnd;
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof Indices)) return false;
            Indices i = (Indices) o;
            return this.mSelectionStart == i.mSelectionStart
                    && this.mSelectionEnd == i.mSelectionEnd
                    && this.mCompositionStart == i.mCompositionStart
                    && this.mCompositionEnd == i.mCompositionEnd;
        }
    }

    private Boolean mLastKeyboardVisibility;
    private Indices mLastIndices;

    @Override
    public void showSoftInput(boolean show) {
        mLastKeyboardVisibility = show;
    }

    @Override
    public void updateIndices(
            int selectionStart, int selectionEnd, int compositionStart, int compositionEnd) {
        mLastIndices = new Indices(selectionEnd, selectionEnd, compositionStart, compositionEnd);
    }

    public Boolean getLastKeyboardVisibility() {
        return mLastKeyboardVisibility;
    }

    public Indices getLastIndices() {
        return mLastIndices;
    }
}
