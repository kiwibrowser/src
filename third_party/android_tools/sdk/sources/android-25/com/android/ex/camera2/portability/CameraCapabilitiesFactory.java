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

package com.android.ex.camera2.portability;

import android.hardware.Camera;

import com.android.ex.camera2.portability.debug.Log;

public class CameraCapabilitiesFactory {

    private static Log.Tag TAG = new Log.Tag("CamCapabsFact");

    public static CameraCapabilities createFrom(Camera.Parameters p) {
        if (p == null) {
            Log.w(TAG, "Null parameter passed in.");
            return null;
        }
        return new AndroidCameraCapabilities(p);
    }
}
