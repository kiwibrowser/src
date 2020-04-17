// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.selection;

import android.annotation.SuppressLint;
import android.view.View;
import android.widget.Magnifier;

/**
 * Implements MagnifierWrapper interface.
 */
@SuppressLint("NewApi") // Magnifier requires API level 28.
public class MagnifierWrapperImpl implements MagnifierWrapper {
    private Magnifier mMagnifier;
    private SelectionPopupControllerImpl.ReadbackViewCallback mCallback;

    /**
     * Constructor.
     */
    public MagnifierWrapperImpl(SelectionPopupControllerImpl.ReadbackViewCallback callback) {
        mCallback = callback;
    }

    @Override
    public void show(float x, float y) {
        View view = mCallback.getReadbackView();
        if (view == null) return;
        if (mMagnifier == null) mMagnifier = new Magnifier(view);
        mMagnifier.show(x, y);
    }

    @Override
    public void dismiss() {
        if (mMagnifier != null) {
            mMagnifier.dismiss();
            mMagnifier = null;
        }
    }

    @Override
    public boolean isAvailable() {
        return mCallback.getReadbackView() != null;
    }
}
