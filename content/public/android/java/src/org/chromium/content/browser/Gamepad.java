// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;

import org.chromium.content_public.browser.WebContents;
import org.chromium.device.gamepad.GamepadList;

/**
 * Encapsulates component class {@link GamepadList} for use in content, with regards
 * to its state according to content being attached to/detached from window.
 */
class Gamepad implements WindowEventObserver {
    private Context mContext;

    public static Gamepad create(Context context, WebContents webContents) {
        return new Gamepad(context, webContents);
    }

    private Gamepad(Context context, WebContents webContents) {
        mContext = context;
        WindowEventObserverManager.from(webContents).addObserver(this);
    }

    // WindowEventObserver

    @Override
    public void onAttachedToWindow() {
        GamepadList.onAttachedToWindow(mContext);
    }

    @Override
    public void onDetachedFromWindow() {
        GamepadList.onDetachedFromWindow();
    }
}
