/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.drawelements.deqp.runner;

/**
 * Test configuration of dEPQ test instance execution.
 */
public class BatchRunConfiguration {
    public static final String ROTATION_UNSPECIFIED = "unspecified";
    public static final String ROTATION_PORTRAIT = "0";
    public static final String ROTATION_LANDSCAPE = "90";
    public static final String ROTATION_REVERSE_PORTRAIT = "180";
    public static final String ROTATION_REVERSE_LANDSCAPE = "270";

    private final String mGlConfig;
    private final String mRotation;
    private final String mSurfaceType;
    private final boolean mRequired;

    public BatchRunConfiguration(String glConfig, String rotation, String surfaceType,
            boolean required) {
        mGlConfig = glConfig;
        mRotation = rotation;
        mSurfaceType = surfaceType;
        mRequired = required;
    }

    /**
     * Get string that uniquely identifies this config
     */
    public String getId() {
        return String.format("{glformat=%s,rotation=%s,surfacetype=%s,required=%b}",
                mGlConfig, mRotation, mSurfaceType, mRequired);
    }

    /**
     * Get the GL config used in this configuration.
     */
    public String getGlConfig() {
        return mGlConfig;
    }

    /**
     * Get the screen rotation used in this configuration.
     */
    public String getRotation() {
        return mRotation;
    }

    /**
     * Get the surface type used in this configuration.
     */
    public String getSurfaceType() {
        return mSurfaceType;
    }

    /**
     * Is this configuration mandatory to support, if target API is supported?
     */
    public boolean isRequired() {
        return mRequired;
    }

    @Override
    public boolean equals(Object other) {
        if (other == null) {
            return false;
        } else if (!(other instanceof BatchRunConfiguration)) {
            return false;
        } else {
            return getId().equals(((BatchRunConfiguration)other).getId());
        }
    }

    @Override
    public int hashCode() {
        return getId().hashCode();
    }
}
