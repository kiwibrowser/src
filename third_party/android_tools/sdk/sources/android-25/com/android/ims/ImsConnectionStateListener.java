/*
 * Copyright (c) 2013 The Android Open Source Project
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

package com.android.ims;

import android.net.Uri;

/**
 * Listener for receiving notifications about changes to the IMS connection.
 * It provides a state of IMS registration between UE and IMS network, the service
 * availability of the local device during IMS registered.
 *
 * @hide
 */
public class ImsConnectionStateListener {
    /**
     * Called when the device is connected to the IMS network.
     */
    public void onImsConnected() {
        // no-op
    }

    /**
     * Called when the device is trying to connect to the IMS network.
     */
    public void onImsProgressing() {
        // no-op
    }

    /**
     * Called when the device is disconnected from the IMS network.
     */
    public void onImsDisconnected(ImsReasonInfo imsReasonInfo) {
        // no-op
    }

    /**
     * Called when its suspended IMS connection is resumed, meaning the connection
     * now allows throughput.
     */
    public void onImsResumed() {
        // no-op
    }

    /**
     * Called when its current IMS connection is suspended, meaning there is no data throughput.
     */
    public void onImsSuspended() {
        // no-op
    }

    /**
     * Called when its current IMS connection feature capability changes.
     */
    public void onFeatureCapabilityChanged(int serviceClass,
                int[] enabledFeatures, int[] disabledFeatures) {
        // no-op
    }

    /**
     * Called when waiting voice message count changes.
     */
    public void onVoiceMessageCountChanged(int count) {
        // no-op
    }

    /**
     * Called after IMS registration.
     */
    public void registrationAssociatedUriChanged(Uri[] uris) {
        // no-op
    }
}
