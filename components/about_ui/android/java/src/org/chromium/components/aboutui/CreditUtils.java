// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.aboutui;

import org.chromium.base.annotations.JNINamespace;

/** Credits-related utilities. */
@JNINamespace("about_ui")
public class CreditUtils {
    private CreditUtils() {}

    /** Returns a string containing the content of about_credits.html. */
    public static native byte[] nativeGetJavaWrapperCredits();
}
