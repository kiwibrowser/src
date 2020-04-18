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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelUuid;
import android.util.Log;
import android.util.SparseArray;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.lang.ref.WeakReference;

import javax.obex.ServerSession;

class BluetoothMnsService {

    private static final String TAG = "BluetoothMnsService";

    private static final ParcelUuid MAP_MNS =
            ParcelUuid.fromString("00001133-0000-1000-8000-00805F9B34FB");

    static final int MSG_EVENT = 1;

    /* for BluetoothMasClient */
    static final int EVENT_REPORT = 1001;

    /* these are shared across instances */
    static private SparseArray<Handler> mCallbacks = null;
    static private SocketAcceptThread mAcceptThread = null;
    static private Handler mSessionHandler = null;
    static private BluetoothServerSocket mServerSocket = null;

    private static class SessionHandler extends Handler {

        private final WeakReference<BluetoothMnsService> mService;

        SessionHandler(BluetoothMnsService service) {
            mService = new WeakReference<BluetoothMnsService>(service);
        }

        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, "Handler: msg: " + msg.what);

            switch (msg.what) {
                case MSG_EVENT:
                    int instanceId = msg.arg1;

                    synchronized (mCallbacks) {
                        Handler cb = mCallbacks.get(instanceId);

                        if (cb != null) {
                            BluetoothMapEventReport ev = (BluetoothMapEventReport) msg.obj;
                            cb.obtainMessage(EVENT_REPORT, ev).sendToTarget();
                        } else {
                            Log.w(TAG, "Got event for instance which is not registered: "
                                    + instanceId);
                        }
                    }
                    break;
            }
        }
    }

    private static class SocketAcceptThread extends Thread {

        private boolean mInterrupted = false;

        @Override
        public void run() {

            if (mServerSocket != null) {
                Log.w(TAG, "Socket already created, exiting");
                return;
            }

            try {
                BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
                mServerSocket = adapter.listenUsingEncryptedRfcommWithServiceRecord(
                        "MAP Message Notification Service", MAP_MNS.getUuid());
            } catch (IOException e) {
                mInterrupted = true;
                Log.e(TAG, "I/O exception when trying to create server socket", e);
            }

            while (!mInterrupted) {
                try {
                    Log.v(TAG, "waiting to accept connection...");

                    BluetoothSocket sock = mServerSocket.accept();

                    Log.v(TAG, "new incoming connection from "
                            + sock.getRemoteDevice().getName());

                    // session will live until closed by remote
                    BluetoothMnsObexServer srv = new BluetoothMnsObexServer(mSessionHandler);
                    BluetoothMapRfcommTransport transport = new BluetoothMapRfcommTransport(
                            sock);
                    new ServerSession(transport, srv, null);
                } catch (IOException ex) {
                    Log.v(TAG, "I/O exception when waiting to accept (aborted?)");
                    mInterrupted = true;
                }
            }

            if (mServerSocket != null) {
                try {
                    mServerSocket.close();
                } catch (IOException e) {
                    // do nothing
                }

                mServerSocket = null;
            }
        }
    }

    BluetoothMnsService() {
        Log.v(TAG, "BluetoothMnsService()");

        if (mCallbacks == null) {
            Log.v(TAG, "BluetoothMnsService(): allocating callbacks");
            mCallbacks = new SparseArray<Handler>();
        }

        if (mSessionHandler == null) {
            Log.v(TAG, "BluetoothMnsService(): allocating session handler");
            mSessionHandler = new SessionHandler(this);
        }
    }

    public void registerCallback(int instanceId, Handler callback) {
        Log.v(TAG, "registerCallback()");

        synchronized (mCallbacks) {
            mCallbacks.put(instanceId, callback);

            if (mAcceptThread == null) {
                Log.v(TAG, "registerCallback(): starting MNS server");
                mAcceptThread = new SocketAcceptThread();
                mAcceptThread.setName("BluetoothMnsAcceptThread");
                mAcceptThread.start();
            }
        }
    }

    public void unregisterCallback(int instanceId) {
        Log.v(TAG, "unregisterCallback()");

        synchronized (mCallbacks) {
            mCallbacks.remove(instanceId);

            if (mCallbacks.size() == 0) {
                Log.v(TAG, "unregisterCallback(): shutting down MNS server");

                if (mServerSocket != null) {
                    try {
                        mServerSocket.close();
                    } catch (IOException e) {
                    }

                    mServerSocket = null;
                }

                mAcceptThread.interrupt();

                try {
                    mAcceptThread.join(5000);
                } catch (InterruptedException e) {
                }

                mAcceptThread = null;
            }
        }
    }
}
