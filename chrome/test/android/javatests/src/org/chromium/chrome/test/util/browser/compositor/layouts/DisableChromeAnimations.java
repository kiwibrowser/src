// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.compositor.layouts;

import org.junit.rules.ExternalResource;

import org.chromium.chrome.browser.compositor.layouts.ChromeAnimation;

/**
 * JUnit 4 rule that disables animations in ChromeAnimation for tests.
 */
public class DisableChromeAnimations extends ExternalResource {
    private float mOldAnimationMultiplier;

    @Override
    protected void before() {
        mOldAnimationMultiplier = ChromeAnimation.Animation.getAnimationMultiplier();
        ChromeAnimation.Animation.setAnimationMultiplierForTesting(0f);
    }

    @Override
    protected void after() {
        ChromeAnimation.Animation.setAnimationMultiplierForTesting(mOldAnimationMultiplier);
    }
}
