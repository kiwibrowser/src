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
import java.io.InputStream;
import java.util.ArrayList;

import javax.obex.ClientSession;
import javax.obex.HeaderSet;

final class BluetoothMasRequestGetFolderListing extends BluetoothMasRequest {

    private static final String TYPE = "x-obex/folder-listing";

    private BluetoothMapFolderListing mResponse = null;

    public BluetoothMasRequestGetFolderListing(int maxListCount, int listStartOffset) {

        if (maxListCount < 0 || maxListCount > 65535) {
            throw new IllegalArgumentException("maxListCount should be [0..65535]");
        }

        if (listStartOffset < 0 || listStartOffset > 65535) {
            throw new IllegalArgumentException("listStartOffset should be [0..65535]");
        }

        mHeaderSet.setHeader(HeaderSet.TYPE, TYPE);

        ObexAppParameters oap = new ObexAppParameters();
        // Allow GetFolderListing for maxListCount value 0 also.
        if (maxListCount >= 0) {
            oap.add(OAP_TAGID_MAX_LIST_COUNT, (short) maxListCount);
        }

        if (listStartOffset > 0) {
            oap.add(OAP_TAGID_START_OFFSET, (short) listStartOffset);
        }

        oap.addToHeaderSet(mHeaderSet);
    }

    @Override
    protected void readResponse(InputStream stream) {
        mResponse = new BluetoothMapFolderListing(stream);
    }

    public ArrayList<String> getList() {
        if (mResponse == null) {
            return null;
        }

        return mResponse.getList();
    }

    @Override
    public void execute(ClientSession session) throws IOException {
        executeGet(session);
    }
}
