// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import android.content.Context;
import android.content.res.Configuration;

import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.browser.ContentViewCore.InternalAccessDelegate;

/**
 * A dummy {@link ContentViewCore} implementation that can be overriden by tests
 * to customize behavior.
 */
public class TestContentViewCore implements ContentViewCore {
    public TestContentViewCore(Context context, String productVersion) {}

    @Override
    public void setContainerViewInternals(InternalAccessDelegate internalDispatcher) {}

    @Override
    public void destroy() {}

    @Override
    public void onAttachedToWindow() {}

    @Override
    public void onDetachedFromWindow() {}

    @Override
    public void onConfigurationChanged(Configuration newConfig) {}

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {}

    @Override
    public void onPause() {}

    @Override
    public void onResume() {}

    @Override
    public void onViewFocusChanged(boolean gainFocus) {}

    @Override
    public void setHideKeyboardOnBlur(boolean hideKeyboardOnBlur) {}
}
