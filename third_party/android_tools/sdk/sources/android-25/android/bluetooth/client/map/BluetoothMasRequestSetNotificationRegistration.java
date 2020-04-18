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

package android.bluetooth.client.map;

import android.bluetooth.client.map.utils.ObexAppParameters;

import java.io.IOException;

import javax.obex.ClientSession;
import javax.obex.HeaderSet;

final class BluetoothMasRequestSetNotificationRegistration extends BluetoothMasRequest {

    private static final String TYPE = "x-bt/MAP-NotificationRegistration";

    private final boolean mStatus;

    public BluetoothMasRequestSetNotificationRegistration(boolean status) {
        mStatus = status;

        mHeaderSet.setHeader(HeaderSet.TYPE, TYPE);

        ObexAppParameters oap = new ObexAppParameters();

        oap.add(OAP_TAGID_NOTIFICATION_STATUS, status ? NOTIFICATION_ON : NOTIFICATION_OFF);

        oap.addToHeaderSet(mHeaderSet);
    }

    @Override
    public void execute(ClientSession session) throws IOException {
        executePut(session, FILLER_BYTE);
    }

    public boolean getStatus() {
        return mStatus;
    }
}
