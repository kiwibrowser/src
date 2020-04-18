// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.layouts;

import static org.chromium.chrome.browser.compositor.layouts.ChromeAnimation.AnimatableAnimation.createAnimation;

import android.os.SystemClock;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.compositor.layouts.ChromeAnimation.Animatable;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Unit tests for {@link org.chromium.chrome.browser.compositor.layouts.ChromeAnimation}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class ChromeAnimationTest implements Animatable<ChromeAnimationTest.Property> {
    protected enum Property {
        FAST_ANIMATION,
        SLOW_ANIMATION
    }

    private static final long FAST_DURATION = 100;
    private static final long SLOW_DURATION = 1000;

    private ChromeAnimation<Animatable<?>> mAnimations;

    private boolean mHasFinishedFastAnimation;
    private boolean mHasFinishedSlowAnimation;

    @Before
    public void setUp() throws Exception {
        mHasFinishedFastAnimation = false;
        mHasFinishedSlowAnimation = false;
    }

    @Override
    public void setProperty(Property prop, float val) {}

    @Override
    public void onPropertyAnimationFinished(Property prop) {
        if (prop == Property.FAST_ANIMATION) {
            mHasFinishedFastAnimation = true;
        } else if (prop == Property.SLOW_ANIMATION) {
            mHasFinishedSlowAnimation = true;
        }
    }

    /**
     * Creates an {@link org.chromium.chrome.browser.compositor.layouts.ChromeAnimation.Animatable}
     * and adds it to the animation.
     * Automatically sets the start value at the beginning of the animation.
     *
     * @param <T>                     The Enum type of the Property being used
     * @param object                  The object being animated
     * @param prop                    The property being animated
     * @param start                   The starting value of the animation
     * @param end                     The ending value of the animation
     * @param duration                The duration of the animation in ms
     * @param startTime               The start time in ms
     */
    private <T extends Enum<?>> void addToAnimation(Animatable<T> object, T prop,
            float start, float end, long duration, long startTime) {
        ChromeAnimation.Animation<Animatable<?>> component = createAnimation(
                object, prop, start, end, duration, startTime, false,
                ChromeAnimation.getDecelerateInterpolator());
        addToAnimation(component);
    }

    /**
     * Appends an Animation to the current animation set and starts it immediately.  If the set is
     * already finished or doesn't exist, the animation set is also started.
     */
    private void addToAnimation(ChromeAnimation.Animation<Animatable<?>> component) {
        if (mAnimations == null || mAnimations.finished()) {
            mAnimations = new ChromeAnimation<>();
            mAnimations.start();
        }
        component.start();
        mAnimations.add(component);
    }

    @Test
    @SmallTest
    @Feature({"ContextualSearch"})
    public void testConcurrentAnimationsFinishSeparately() {
        addToAnimation(this, Property.FAST_ANIMATION, 0.f, 1.f, FAST_DURATION, 0);
        addToAnimation(this, Property.SLOW_ANIMATION, 0.f, 1.f, SLOW_DURATION, 0);

        // Update the animation with the current time. This will internally set the initial
        // time of the animation to |now|.
        long now = SystemClock.uptimeMillis();
        mAnimations.update(now);

        // Advances time to check that the fast animation will finish first.
        mAnimations.update(now + FAST_DURATION);
        Assert.assertTrue(mHasFinishedFastAnimation);
        Assert.assertFalse(mHasFinishedSlowAnimation);

        // Advances time to check that all animations are finished.
        mAnimations.update(now + SLOW_DURATION);
        Assert.assertTrue(mHasFinishedFastAnimation);
        Assert.assertTrue(mHasFinishedSlowAnimation);
    }
}
