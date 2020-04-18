/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

package com.android.ex.camera2.utils;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureRequest.Builder;
import android.hardware.camera2.CaptureRequest.Key;
import android.view.Surface;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

/**
 * A set of settings to be used when filing a {@link CaptureRequest}.
 */
public class Camera2RequestSettingsSet {
    private final Map<Key<?>, Object> mDictionary;
    private long mRevision;

    /**
     * Create a new instance with no settings defined.
     *
     * <p>Creating a request from this object without first specifying any
     * properties on it is equivalent to just creating a request directly
     * from the template of choice. Its revision identifier is initially
     * {@code 0}, and will remain thus until its first modification.</p>
     */
    public Camera2RequestSettingsSet() {
        mDictionary = new HashMap<>();
        mRevision = 0;
    }

    /**
     * Perform a deep copy of the defined settings and revision number.
     *
     * @param other The reference instance.
     *
     * @throws NullPointerException If {@code other} is {@code null}.
     */
    public Camera2RequestSettingsSet(Camera2RequestSettingsSet other) {
        if (other == null) {
            throw new NullPointerException("Tried to copy null Camera2RequestSettingsSet");
        }

        mDictionary = new HashMap<>(other.mDictionary);
        mRevision = other.mRevision;
    }

    /**
     * Specify a setting, potentially overriding the template's default choice.
     *
     * <p>Providing a {@code null} {@code value} will indicate a forced use of
     * the template's selection for that {@code key}; the difference here is
     * that this information will be propagated with unions as documented in
     * {@link #union}. This method increments the revision identifier if the new
     * choice is different than the existing selection.</p>
     *
     * @param key Which setting to alter.
     * @param value The new selection for that setting, or {@code null} to force
     *              the use of the template's default selection for this field.
     * @return Whether the settings were updated, which only occurs if the
     *         {@code value} is different from any already stored.
     *
     * @throws NullPointerException If {@code key} is {@code null}.
     */
    public <T> boolean set(Key<T> key, T value) {
        if (key == null) {
            throw new NullPointerException("Received a null key");
        }

        Object currentValue = get(key);
        // Only save the value if it's different from the one we already have
        if (!mDictionary.containsKey(key) || !Objects.equals(value, currentValue)) {
            mDictionary.put(key, value);
            ++mRevision;
            return true;
        }
        return false;
    }

    /**
     * Unsets a setting, preventing it from being propagated with unions or from
     * overriding the default when creating a capture request.
     *
     * <p>This method increments the revision identifier if a selection had
     * previously been made for that parameter.</p>
     *
     * @param key Which setting to reset.
     * @return Whether the settings were updated, which only occurs if the
     *         specified setting already had a value or was forced to default.
     *
     * @throws NullPointerException If {@code key} is {@code null}.
     */
    public boolean unset(Key<?> key) {
        if (key == null) {
            throw new NullPointerException("Received a null key");
        }

        if (mDictionary.containsKey(key)) {
            mDictionary.remove(key);
            ++mRevision;
            return true;
        }
        return false;
    }

    /**
     * Interrogate the current specialization of a setting.
     *
     * @param key Which setting to check.
     * @return The current selection for that setting, or {@code null} if the
     *         setting is unset or forced to the template-defined default.
     *
     * @throws NullPointerException If {@code key} is {@code null}.
     */
    @SuppressWarnings("unchecked")
    public <T> T get(Key<T> key) {
        if (key == null) {
            throw new NullPointerException("Received a null key");
        }
        return (T) mDictionary.get(key);
    }

    /**
     * Query this instance for whether it prefers a particular choice for the
     * given request parameter.
     *
     * <p>This method can be used to detect whether a particular field is forced
     * to its default value or simply unset. While {@link #get} will return
     * {@code null} in both these cases, this method will return {@code true}
     * and {@code false}, respectively.</p>
     *
     * @param key Which setting to look for.
     * @return Whether that setting has a value that will propagate with unions.
     *
     * @throws NullPointerException If {@code key} is {@code null}.
     */
    public boolean contains(Key<?> key) {
        if (key == null) {
            throw new NullPointerException("Received a null key");
        }
        return mDictionary.containsKey(key);
    }

    /**
     * Check whether the value of the specified setting matches the given one.
     *
     * <p>This method uses the {@code T} type's {@code equals} method, but is
     * {@code null}-tolerant.</p>
     *
     * @param key Which of this class's settings to check.
     * @param value Value to test for equality against.
     * @return Whether they are the same.
     */
    public <T> boolean matches(Key<T> key, T value) {
        return Objects.equals(get(key), value);
    }

    /**
     * Get this set of settings's revision identifier, which can be compared
     * against cached past values to determine whether it has been modified.
     *
     * <p>Distinct revisions across the same object do not necessarily indicate
     * that the object's key/value pairs have changed at all, but the same
     * revision on the same object does imply that they've stayed the same.</p>
     *
     * @return The number of modifications made since the beginning of this
     *         object's heritage.
     */
    public long getRevision() {
        return mRevision;
    }

    /**
     * Add all settings choices defined by {@code moreSettings} to this object.
     *
     * <p>For any settings defined in both, the choice stored in the argument
     * to this method take precedence. Unset settings are not propagated, but
     * those forced to default as described in {@link set} are also forced to
     * default in {@code this} set. Invoking this method increments {@code this}
     * object's revision counter, but leaves the argument's unchanged.</p>
     *
     * @param moreSettings The source of the additional settings ({@code null}
     *                     is allowed here).
     * @return Whether these settings were updated, which can only fail if the
     *         target itself is also given as the argument.
     */
    public boolean union(Camera2RequestSettingsSet moreSettings) {
        if (moreSettings == null || moreSettings == this) {
            return false;
        }

        mDictionary.putAll(moreSettings.mDictionary);
        ++mRevision;
        return true;
    }

    /**
     * Create a {@link CaptureRequest} specialized for the specified
     * {@link CameraDevice} and targeting the given {@link Surface}s.
     *
     * @param camera The camera from which to capture.
     * @param template A {@link CaptureRequest} template defined in
     *                 {@link CameraDevice}.
     * @param targets The location(s) to draw the resulting image onto.
     * @return The request, ready to be passed to the camera framework.
     *
     * @throws CameraAccessException Upon an underlying framework API failure.
     * @throws NullPointerException If any argument is {@code null}.
     */
    public CaptureRequest createRequest(CameraDevice camera, int template, Surface... targets)
            throws CameraAccessException {
        if (camera == null) {
            throw new NullPointerException("Tried to create request using null CameraDevice");
        }

        Builder reqBuilder = camera.createCaptureRequest(template);
        for (Key<?> key : mDictionary.keySet()) {
            setRequestFieldIfNonNull(reqBuilder, key);
        }
        for (Surface target : targets) {
            if (target == null) {
                throw new NullPointerException("Tried to add null Surface as request target");
            }
            reqBuilder.addTarget(target);
        }
        return reqBuilder.build();
    }

    private <T> void setRequestFieldIfNonNull(Builder requestBuilder, Key<T> key) {
        T value = get(key);
        if (value != null) {
            requestBuilder.set(key, value);
        }
    }
}
