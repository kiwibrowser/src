// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.util.test;

import android.text.TextUtils;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

import org.chromium.chrome.browser.util.UrlUtilities;

/** Implementation of UrlUtilities which does not rely on native. */
@Implements(UrlUtilities.class)
public class ShadowUrlUtilities {
    @Implementation
    public static boolean urlsMatchIgnoringFragments(String url, String url2) {
        return TextUtils.equals(url, url2);
    }
}
