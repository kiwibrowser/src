/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.bordeaux.services;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PointF;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;

/**
 * {@hide}
 * This is used to provide a convenience to access the actual remote running
 * service.
 * TODO: Eventally the remote service will be running in the system server, and
 * this will need to be served as a stub for the remote running service. And
 * extends from IBordeauxManager.stub
 */
public class BordeauxManagerService {

    static private final String TAG = "BordeauxMangerService";
    static private IBordeauxService mService = null;
    static private ILearning_StochasticLinearRanker mRanker = null;
    static private IAggregatorManager mAggregatorManager = null;
    static private IPredictor mPredictor = null;
    static private ILearning_MulticlassPA mClassifier = null;
    static private boolean mStarted = false;

    public BordeauxManagerService() {
    }

    static private synchronized void bindServices(Context context) {
        if (mStarted) return;
        context.bindService(new Intent(IBordeauxService.class.getName()),
               mConnection, Context.BIND_AUTO_CREATE);
        mStarted = true;
    }

    // Call the release, before the Context gets destroyed.
    static public synchronized void release(Context context) {
        if (mStarted && mConnection != null) {
            context.unbindService(mConnection);
            mService = null;
            mStarted = false;
        }
    }

    static public synchronized IBordeauxService getService(Context context) {
        if (mService == null) bindServices(context);
        return mService;
    }

    static public synchronized IAggregatorManager getAggregatorManager(Context context) {
        if (mService == null) {
            bindServices(context);
            return null;
        }
        try {
            mAggregatorManager = IAggregatorManager.Stub.asInterface(
                    mService.getAggregatorManager());
        } catch (RemoteException e) {
            mAggregatorManager = null;
        }
        return mAggregatorManager;
    }

    static public synchronized IPredictor getPredictor(Context context, String name) {
        if (mService == null) {
            bindServices(context);
            return null;
        }
        try {
            mPredictor = IPredictor.Stub.asInterface(mService.getPredictor(name));
        } catch (RemoteException e) {
            mPredictor = null;
        }
        return mPredictor;
    }

    static public synchronized ILearning_StochasticLinearRanker
            getRanker(Context context, String name) {
        if (mService == null) {
            bindServices(context);
            return null;
        }
        try {
            mRanker =
                    ILearning_StochasticLinearRanker.Stub.asInterface(
                            mService.getRanker(name));
        } catch (RemoteException e) {
            mRanker = null;
        }
        return mRanker;
    }

    static public synchronized ILearning_MulticlassPA
            getClassifier(Context context, String name) {
        if (mService == null) {
            bindServices(context);
            return null;
        }
        try {
            mClassifier =
                    ILearning_MulticlassPA.Stub.asInterface(mService.getClassifier(name));
        } catch (RemoteException e) {
            mClassifier = null;
        }
        return mClassifier;
    }

    /**
     * Class for interacting with the main interface of the service.
     */
    static private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className,
                IBinder service) {
            // This is called when the connection with the service has been
            // established.
            mService = IBordeauxService.Stub.asInterface(service);
        }

        public void onServiceDisconnected(ComponentName className) {
            // This is called when the connection with the service has been
            // unexpectedly disconnected -- that is, its process crashed.
            mService = null;
            mStarted = false; // needs to bind again
        }
    };
}
