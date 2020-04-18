// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.rules;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * An annotation for setting the VrCore head tracking service's tracking mode during pre-test setup.
 *
 * The benefit of setting the mode this way instead of via HeadTrackingUtils during a test is that
 * starting services is asynchronous with no good way of waiting until whatever the service does
 * takes effect. When set during a test, the test must idle long enough to safely assume that the
 * service has taken effect. When applied during test setup, the Chrome startup period acts as the
 * wait, as Chrome startup is slow enough that it's safe to assume the service has started by the
 * time Chrome is ready.
 *
 * For example, the following would cause a test to start with its head position locked looking
 * straight forward:
 *     <code>
 *     @HeadTrackingMode(HeadTrackingMode.SupportedMode.FROZEN)
 *     </code>
 * If a test is not annotated with this, it will use whatever mode is currently set. This should
 * usually be the normal, sensor-based tracker, but is not guaranteed.
 */
@Target({ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
public @interface HeadTrackingMode {
    public enum SupportedMode {
        FROZEN, // Locked looking straight forward.
        SWEEP, // Rotates back and forth horizontally in a 180 degree arc.
        ROTATE, // Rotates 360 degrees.
        CIRCLE_STRAFE, // Rotates 360 degrees, and if 6DOF is supported, changes position.
        MOTION_SICKNESS // Moves in a figure-eight-like pattern.
    }

    /**
     * @return The supported mode.
     */
    public SupportedMode value();
}
