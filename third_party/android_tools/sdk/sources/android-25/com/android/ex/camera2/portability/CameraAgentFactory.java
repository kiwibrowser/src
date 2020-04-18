/*
 * Copyright (C) 2013 The Android Open Source Project
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

import android.content.Context;
import android.os.Build;

import com.android.ex.camera2.portability.debug.Log;
import com.android.ex.camera2.portability.util.SystemProperties;

/**
 * A factory class for {@link CameraAgent}.
 *
 * <p>The choice of framework API to use can be made automatically based on the
 * system API level, explicitly forced by the client app, or overridden entirely
 * by setting the system property com.camera2.portability.fwk_api to 1 or 2.</p>
 */
public class CameraAgentFactory {
    private static final Log.Tag TAG = new Log.Tag("CamAgntFact");

    /** Android release replacing the Camera class with the camera2 package. */
    private static final int FIRST_SDK_WITH_API_2 = 21;

    // The debugging override, which overrides *all* API level selections if set
    // to API_LEVEL_OVERRIDE_API{1,2}; otherwise, this has no effect. Note that
    // we check this once when the library is first loaded so that #recycle()
    // doesn't try to clean up the wrong type of CameraAgent.
    private static final String API_LEVEL_OVERRIDE_KEY = "camera2.portability.force_api";
    private static final String API_LEVEL_OVERRIDE_DEFAULT = "0";
    private static final String API_LEVEL_OVERRIDE_API1 = "1";
    private static final String API_LEVEL_OVERRIDE_API2 = "2";
    private static final String API_LEVEL_OVERRIDE_VALUE =
            SystemProperties.get(API_LEVEL_OVERRIDE_KEY, API_LEVEL_OVERRIDE_DEFAULT);

    private static CameraAgent sAndroidCameraAgent;
    private static CameraAgent sAndroidCamera2Agent;
    private static int sAndroidCameraAgentClientCount;
    private static int sAndroidCamera2AgentClientCount;

    /**
     * Used to indicate which camera framework should be used.
     */
    public static enum CameraApi {
        /** Automatically select based on the device's SDK level. */
        AUTO,

        /** Use the {@link android.hardware.Camera} class. */
        API_1,

        /** Use the {@link android.hardware.camera2} package. */
        API_2
    };

    private static CameraApi highestSupportedApi() {
        // TODO: Check SDK_INT instead of RELEASE before L launch
        if (Build.VERSION.SDK_INT >= FIRST_SDK_WITH_API_2 || Build.VERSION.CODENAME.equals("L")) {
            return CameraApi.API_2;
        } else {
            return CameraApi.API_1;
        }
    }

    private static CameraApi validateApiChoice(CameraApi choice) {
        if (API_LEVEL_OVERRIDE_VALUE.equals(API_LEVEL_OVERRIDE_API1)) {
            Log.d(TAG, "API level overridden by system property: forced to 1");
            return CameraApi.API_1;
        } else if (API_LEVEL_OVERRIDE_VALUE.equals(API_LEVEL_OVERRIDE_API2)) {
            Log.d(TAG, "API level overridden by system property: forced to 2");
            return CameraApi.API_2;
        }

        if (choice == null) {
            Log.w(TAG, "null API level request, so assuming AUTO");
            choice = CameraApi.AUTO;
        }
        if (choice == CameraApi.AUTO) {
            choice = highestSupportedApi();
        }

        return choice;
    }

    /**
     * Returns the android camera implementation of
     * {@link com.android.camera.cameradevice.CameraAgent}.
     *
     * <p>To clean up the resources allocated by this call, be sure to invoke
     * {@link #recycle(boolean)} with the same {@code api} value provided
     * here.</p>
     *
     * @param context The application context.
     * @param api Which camera framework to use.
     * @return The {@link CameraAgent} to control the camera device.
     *
     * @throws UnsupportedOperationException If {@code CameraApi.API_2} was
     *                                       requested on an unsupported device.
     */
    public static synchronized CameraAgent getAndroidCameraAgent(Context context, CameraApi api) {
        api = validateApiChoice(api);

        if (api == CameraApi.API_1) {
            if (sAndroidCameraAgent == null) {
                sAndroidCameraAgent = new AndroidCameraAgentImpl();
                sAndroidCameraAgentClientCount = 1;
            } else {
                ++sAndroidCameraAgentClientCount;
            }
            return sAndroidCameraAgent;
        } else { // API_2
            if (highestSupportedApi() == CameraApi.API_1) {
                throw new UnsupportedOperationException("Camera API_2 unavailable on this device");
            }

            if (sAndroidCamera2Agent == null) {
                sAndroidCamera2Agent = new AndroidCamera2AgentImpl(context);
                sAndroidCamera2AgentClientCount = 1;
            } else {
                ++sAndroidCamera2AgentClientCount;
            }
            return sAndroidCamera2Agent;
        }
    }

    /**
     * Recycles the resources. Always call this method when the activity is
     * stopped.
     *
     * @param api Which camera framework handle to recycle.
     *
     * @throws UnsupportedOperationException If {@code CameraApi.API_2} was
     *                                       requested on an unsupported device.
     */
    public static synchronized void recycle(CameraApi api) {
        api = validateApiChoice(api);

        if (api == CameraApi.API_1) {
            if (--sAndroidCameraAgentClientCount == 0 && sAndroidCameraAgent != null) {
                sAndroidCameraAgent.recycle();
                sAndroidCameraAgent = null;
            }
        } else { // API_2
            if (highestSupportedApi() == CameraApi.API_1) {
                throw new UnsupportedOperationException("Camera API_2 unavailable on this device");
            }

            if (--sAndroidCamera2AgentClientCount == 0 && sAndroidCamera2Agent != null) {
                sAndroidCamera2Agent.recycle();
                sAndroidCamera2Agent = null;
            }
        }
    }
}
