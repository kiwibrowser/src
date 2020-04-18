/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ex.camera2.portability;

import android.os.SystemClock;

import com.android.ex.camera2.portability.debug.Log;

public abstract class CameraStateHolder {
    private static final Log.Tag TAG = new Log.Tag("CamStateHolder");

    private int mState;
    private boolean mInvalid;

    /**
     * Construct a new instance of @{link CameraStateHolder} with an initial state.
     *
     * @param state The initial state.
     */
    public CameraStateHolder(int state) {
        setState(state);
        mInvalid = false;
    }

    /**
     * Change to a new state.
     *
     * @param state The new state.
     */
    public synchronized void setState(int state) {
        if (mState != state) {
            Log.v(TAG, "setState - state = " + Integer.toBinaryString(state));
        }
        mState = state;
        this.notifyAll();
    }

    /**
     * Obtain the current state.
     *
     * @return The current state.
     */
    public synchronized int getState() {
        return mState;
    }

    /**
     * Change the state to be invalid. Once invalidated, the state will be invalid forever.
     */
    public synchronized void invalidate() {
        mInvalid = true;
    }

    /**
     * Whether the state is invalid.
     *
     * @return True if the state is invalid.
     */
    public synchronized boolean isInvalid() {
        return mInvalid;
    }

    private static interface ConditionChecker {
        /**
         * @return Whether the condition holds.
         */
        boolean success();
    }

    /**
     * A helper method used by {@link #waitToAvoidStates(int)} and
     * {@link #waitForStates(int)}. This method will wait until the
     * condition is successful.
     *
     * @param stateChecker The state checker to be used.
     * @param timeoutMs The timeout limit in milliseconds.
     * @return {@code false} if the wait is interrupted or timeout limit is
     *         reached.
     */
    private boolean waitForCondition(ConditionChecker stateChecker,
            long timeoutMs) {
        long timeBound = SystemClock.uptimeMillis() + timeoutMs;
        synchronized (this) {
            while (!stateChecker.success()) {
                try {
                    this.wait(timeoutMs);
                } catch (InterruptedException ex) {
                    if (SystemClock.uptimeMillis() > timeBound) {
                        // Timeout.
                        Log.w(TAG, "Timeout waiting.");
                    }
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Block the current thread until the state becomes one of the
     * specified.
     *
     * @param states Expected states.
     * @return {@code false} if the wait is interrupted or timeout limit is
     *         reached.
     */
    public boolean waitForStates(final int states) {
        Log.v(TAG, "waitForStates - states = " + Integer.toBinaryString(states));
        return waitForCondition(new ConditionChecker() {
            @Override
            public boolean success() {
                return (states | getState()) == states;
            }
        }, CameraAgent.CAMERA_OPERATION_TIMEOUT_MS);
    }

    /**
     * Block the current thread until the state becomes NOT one of the
     * specified.
     *
     * @param states States to avoid.
     * @return {@code false} if the wait is interrupted or timeout limit is
     *         reached.
     */
    public boolean waitToAvoidStates(final int states) {
        Log.v(TAG, "waitToAvoidStates - states = " + Integer.toBinaryString(states));
        return waitForCondition(new ConditionChecker() {
            @Override
            public boolean success() {
                return (states & getState()) == 0;
            }
        }, CameraAgent.CAMERA_OPERATION_TIMEOUT_MS);
    }
}
