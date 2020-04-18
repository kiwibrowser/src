// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting.jni;

import android.view.MotionEvent;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * This class holds touch event data from a MotionEvent object which will be marshaled across
 * the java / C++ boundary and get sent to a remote host for input injection.
 */
@JNINamespace("remoting")
public class TouchEventData {
    /**
     * These values are mirrored in the 'EventType' enum in event.proto.
     */
    public enum EventType {
        TOUCH_EVENT_UNKNOWN(-1),
        TOUCH_EVENT_START(1),
        TOUCH_EVENT_MOVE(2),
        TOUCH_EVENT_END(3),
        TOUCH_EVENT_CANCEL(4);

        private final int mValue;

        EventType(int value) {
            mValue = value;
        }

        public int value() {
            return mValue;
        }

        /**
         * Converts an Android MotionEvent masked action value into the corresponding
         * native touch event value.
         */
        public static EventType fromMaskedAction(int action) {
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    return TOUCH_EVENT_START;

                case MotionEvent.ACTION_MOVE:
                    return TOUCH_EVENT_MOVE;

                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                    return TOUCH_EVENT_END;

                case MotionEvent.ACTION_CANCEL:
                    return TOUCH_EVENT_CANCEL;

                default:
                    return TOUCH_EVENT_UNKNOWN;
            }
        }
    }

    /**
     * Creates a new TouchEventData object using the provided values.
     */
    public TouchEventData(int touchPointId, float touchPointX, float touchPointY,
            float touchPointRadiusX, float touchPointRadiusY, float touchPointAngleInRadians,
            float touchPointPressure) {
        mTouchPointId = touchPointId;
        mTouchPointX = touchPointX;
        mTouchPointY = touchPointY;
        mTouchPointRadiusX = touchPointRadiusX;
        mTouchPointRadiusY = touchPointRadiusY;
        mTouchPointPressure = touchPointPressure;

        // MotionEvent angle is measured in radians and our API expects a positive value in degrees.
        if (touchPointAngleInRadians < 0.0f) {
            touchPointAngleInRadians = (float) (touchPointAngleInRadians + (2 * Math.PI));
        }
        mTouchPointAngleInDegrees = (float) Math.toDegrees(touchPointAngleInRadians);
    }

    /**
     * The ID for the touch point (as tracked by the OS).
     */
    private final int mTouchPointId;

    /**
     * Returns the ID for this touch point.
     */
    @CalledByNative
    public int getTouchPointId() {
        return mTouchPointId;
    }

    /**
     * The x-coordinate of the touch point on the screen.
     */
    private final float mTouchPointX;

    /**
     * Returns the x-coordinate of this touch point.
     */
    @CalledByNative
    public float getTouchPointX() {
        return mTouchPointX;
    }

    /**
     * The y-coordinate of the touch point on the screen.
     */
    private final float mTouchPointY;

    /**
     * Returns the y-coordinate of this touch point.
     */
    @CalledByNative
    public float getTouchPointY() {
        return mTouchPointY;
    }

    /**
     * The size of the touch point on the screen measured along the x-axis.
     */
    private final float mTouchPointRadiusX;

    /**
     * Returns the size of this touch point as measured along the x-axis.
     */
    @CalledByNative
    public float getTouchPointRadiusX() {
        return mTouchPointRadiusX;
    }

    /**
     * The size of the touch point on the screen measured along the y-axis.
     */
    private final float mTouchPointRadiusY;

    /**
     * Returns the size of this touch point as measured along the y-axis.
     */
    @CalledByNative
    public float getTouchPointRadiusY() {
        return mTouchPointRadiusY;
    }

    /**
     * The angle of the tool (finger/stylus/etc) which is generating this touch point.
     * More information can be found at: {@link MotionEvent.getOrientation()}.
     */
    private final float mTouchPointAngleInDegrees;

    /**
     * Returns the angle of tool generating this touch point.
     */
    @CalledByNative
    public float getTouchPointAngle() {
        return mTouchPointAngleInDegrees;
    }

    /**
     * The pressure of the touch point as measured by the OS from 0.0 to 1.0.
     */
    private final float mTouchPointPressure;

    /**
     * Returns the pressure of this touch point.
     */
    @CalledByNative
    public float getTouchPointPressure() {
        return mTouchPointPressure;
    }
}
