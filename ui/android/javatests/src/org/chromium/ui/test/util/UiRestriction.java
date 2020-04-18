// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.test.util;

/**
 * UiRestrictions list form factor restrictions, that are usable with
 * the @Restriction enum in layers depending on //ui.
 * E.g. @Restriction({UiRestriction.RESTRICTION_TYPE_PHONE})
 */

public final class UiRestriction {
    /** Specifies the test is only valid on phone form factors. */
    public static final String RESTRICTION_TYPE_PHONE = "Phone";
    /** Specifies the test is only valid on tablet form factors. */
    public static final String RESTRICTION_TYPE_TABLET = "Tablet";
}
