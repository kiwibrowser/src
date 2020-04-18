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

import com.android.ims.internal.IImsMultiEndpoint;
import com.android.ims.internal.IImsExternalCallStateListener;

import android.os.RemoteException;
import android.telephony.Rlog;

import java.util.List;

/**
 * Provides APIs for the IMS multi-endpoint functionality.  Specifically, provides a means for IMS
 * to subscribe to dialog event packages issued by the network.
 *
 * @hide
 */
public class ImsMultiEndpoint {
    /**
     * Adapter class for {@link IImsExternalCallStateListener}.
     */
    private class ImsExternalCallStateListenerProxy extends IImsExternalCallStateListener.Stub {
        private ImsExternalCallStateListener mListener;

        public ImsExternalCallStateListenerProxy(ImsExternalCallStateListener listener) {
            mListener = listener;
        }


        /**
         * Notifies client when Dialog Event Package update is received
         *
         * @param externalCallState the external call state.
         */
        @Override
        public void onImsExternalCallStateUpdate(List<ImsExternalCallState> externalCallState) {
            if (DBG) Rlog.d(TAG, "onImsExternalCallStateUpdate");

            if (mListener != null) {
                mListener.onImsExternalCallStateUpdate(externalCallState);
            }
        }
    }

    private static final String TAG = "ImsMultiEndpoint";
    private static final boolean DBG = true;

    private final IImsMultiEndpoint mImsMultiendpoint;

    public ImsMultiEndpoint(IImsMultiEndpoint iImsMultiEndpoint) {
        if (DBG) Rlog.d(TAG, "ImsMultiEndpoint created");
        mImsMultiendpoint = iImsMultiEndpoint;
    }

    public void setExternalCallStateListener(ImsExternalCallStateListener externalCallStateListener)
            throws ImsException {
        try {
            if (DBG) Rlog.d(TAG, "setExternalCallStateListener");
            mImsMultiendpoint.setListener(new ImsExternalCallStateListenerProxy(
                    externalCallStateListener));
        } catch (RemoteException e) {
            throw new ImsException("setExternalCallStateListener could not be set.", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }
    }
}
