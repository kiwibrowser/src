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

final class BluetoothMasRequestGetFolderListingSize extends BluetoothMasRequest {

    private static final String TYPE = "x-obex/folder-listing";

    private int mSize;

    public BluetoothMasRequestGetFolderListingSize() {
        mHeaderSet.setHeader(HeaderSet.TYPE, TYPE);

        ObexAppParameters oap = new ObexAppParameters();
        oap.add(OAP_TAGID_MAX_LIST_COUNT, 0);

        oap.addToHeaderSet(mHeaderSet);
    }

    @Override
    protected void readResponseHeaders(HeaderSet headerset) {
        ObexAppParameters oap = ObexAppParameters.fromHeaderSet(headerset);

        mSize = oap.getShort(OAP_TAGID_FOLDER_LISTING_SIZE);
    }

    public int getSize() {
        return mSize;
    }

    @Override
    public void execute(ClientSession session) throws IOException {
        executeGet(session);
    }
}
