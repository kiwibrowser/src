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
 * limitations under the License
 */

package com.android.ims;

import java.util.List;

/**
 * Listener for receiving notifications about {@link ImsExternalCallState} information received
 * from the network via a dialog event package.
 *
 * @hide
 */
public class ImsExternalCallStateListener {
    /**
     * Notifies client when Dialog Event Package update is received
     *
     * @param externalCallState the external call state.
     */
    public void onImsExternalCallStateUpdate(List<ImsExternalCallState> externalCallState) {
        // no-op
    }
}
