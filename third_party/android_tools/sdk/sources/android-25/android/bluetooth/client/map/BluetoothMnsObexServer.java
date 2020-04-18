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

import android.os.Handler;
import android.util.Log;

import android.bluetooth.client.map.utils.ObexAppParameters;

import java.io.IOException;
import java.util.Arrays;

import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ResponseCodes;
import javax.obex.ServerRequestHandler;

class BluetoothMnsObexServer extends ServerRequestHandler {

    private final static String TAG = "BluetoothMnsObexServer";

    private static final byte[] MNS_TARGET = new byte[] {
            (byte) 0xbb, 0x58, 0x2b, 0x41, 0x42, 0x0c, 0x11, (byte) 0xdb, (byte) 0xb0, (byte) 0xde,
            0x08, 0x00, 0x20, 0x0c, (byte) 0x9a, 0x66
    };

    private final static String TYPE = "x-bt/MAP-event-report";

    private final Handler mCallback;

    public BluetoothMnsObexServer(Handler callback) {
        super();

        mCallback = callback;
    }

    @Override
    public int onConnect(final HeaderSet request, HeaderSet reply) {
        Log.v(TAG, "onConnect");

        try {
            byte[] uuid = (byte[]) request.getHeader(HeaderSet.TARGET);

            if (!Arrays.equals(uuid, MNS_TARGET)) {
                return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
            }

        } catch (IOException e) {
            // this should never happen since getHeader won't throw exception it
            // declares to throw
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        reply.setHeader(HeaderSet.WHO, MNS_TARGET);
        return ResponseCodes.OBEX_HTTP_OK;
    }

    @Override
    public void onDisconnect(final HeaderSet request, HeaderSet reply) {
        Log.v(TAG, "onDisconnect");
    }

    @Override
    public int onGet(final Operation op) {
        Log.v(TAG, "onGet");

        return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
    }

    @Override
    public int onPut(final Operation op) {
        Log.v(TAG, "onPut");

        try {
            HeaderSet headerset;
            headerset = op.getReceivedHeader();

            String type = (String) headerset.getHeader(HeaderSet.TYPE);
            ObexAppParameters oap = ObexAppParameters.fromHeaderSet(headerset);

            if (!TYPE.equals(type) || !oap.exists(BluetoothMasRequest.OAP_TAGID_MAS_INSTANCE_ID)) {
                return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
            }

            Byte inst = oap.getByte(BluetoothMasRequest.OAP_TAGID_MAS_INSTANCE_ID);

            BluetoothMapEventReport ev = BluetoothMapEventReport.fromStream(op
                    .openDataInputStream());

            op.close();

            mCallback.obtainMessage(BluetoothMnsService.MSG_EVENT, inst, 0, ev).sendToTarget();
        } catch (IOException e) {
            Log.e(TAG, "I/O exception when handling PUT request", e);
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        return ResponseCodes.OBEX_HTTP_OK;
    }

    @Override
    public int onAbort(final HeaderSet request, HeaderSet reply) {
        Log.v(TAG, "onAbort");

        return ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
    }

    @Override
    public int onSetPath(final HeaderSet request, HeaderSet reply,
            final boolean backup, final boolean create) {
        Log.v(TAG, "onSetPath");

        return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
    }

    @Override
    public void onClose() {
        Log.v(TAG, "onClose");

        // TODO: call session handler so it can disconnect
    }
}
