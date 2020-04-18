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

package com.android.internal.telephony.imsphone;

import com.android.internal.R;
import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.UUSInfo;

import android.content.Context;
import android.net.Uri;
import android.telecom.PhoneAccount;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.util.Log;

import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Represents an IMS call external to the device.  This class is used to represent a call which
 * takes places on a secondary device associated with this one.  Originates from a Dialog Event
 * Package.
 *
 * Dialog event package information is received from the IMS framework via
 * {@link com.android.ims.ImsExternalCallState} instances.
 *
 * @hide
 */
public class ImsExternalConnection extends Connection {

    private static final String CONFERENCE_PREFIX = "conf";
    private final Context mContext;

    public interface Listener {
        void onPullExternalCall(ImsExternalConnection connection);
    }

    /**
     * ConcurrentHashMap constructor params: 8 is initial table size, 0.9f is
     * load factor before resizing, 1 means we only expect a single thread to
     * access the map so make only a single shard
     */
    private final Set<Listener> mListeners = Collections.newSetFromMap(
            new ConcurrentHashMap<Listener, Boolean>(8, 0.9f, 1));

    /**
     * The unqiue dialog event package specified ID associated with this external connection.
     */
    private int mCallId;

    /**
     * A backing call associated with this external connection.
     */
    private ImsExternalCall mCall;

    /**
     * The original address as contained in the dialog event package.
     */
    private Uri mOriginalAddress;

    /**
     * Determines if the call is pullable.
     */
    private boolean mIsPullable;

    protected ImsExternalConnection(Phone phone, int callId, Uri address, boolean isPullable) {
        super(phone.getPhoneType());
        mContext = phone.getContext();
        mCall = new ImsExternalCall(phone, this);
        mCallId = callId;
        setExternalConnectionAddress(address);
        mNumberPresentation = PhoneConstants.PRESENTATION_ALLOWED;
        mIsPullable = isPullable;

        rebuildCapabilities();
        setActive();
    }

    /**
     * @return the unique ID of this connection from the dialog event package data.
     */
    public int getCallId() {
        return mCallId;
    }

    @Override
    public Call getCall() {
        return mCall;
    }

    @Override
    public long getDisconnectTime() {
        return 0;
    }

    @Override
    public long getHoldDurationMillis() {
        return 0;
    }

    @Override
    public String getVendorDisconnectCause() {
        return null;
    }

    @Override
    public void hangup() throws CallStateException {
        // No-op - Hangup is not supported for external calls.
    }

    @Override
    public void separate() throws CallStateException {
        // No-op - Separate is not supported for external calls.
    }

    @Override
    public void proceedAfterWaitChar() {
        // No-op - not supported for external calls.
    }

    @Override
    public void proceedAfterWildChar(String str) {
        // No-op - not supported for external calls.
    }

    @Override
    public void cancelPostDial() {
        // No-op - not supported for external calls.
    }

    @Override
    public int getNumberPresentation() {
        return mNumberPresentation;
    }

    @Override
    public UUSInfo getUUSInfo() {
        return null;
    }

    @Override
    public int getPreciseDisconnectCause() {
        return 0;
    }

    @Override
    public boolean isMultiparty() {
        return false;
    }

    /**
     * Called by a {@link android.telecom.Connection} to indicate that this call should be pulled
     * to the local device.
     *
     * Informs all listeners, in this case {@link ImsExternalCallTracker}, of the request to pull
     * the call.
     */
    @Override
    public void pullExternalCall() {
        for (Listener listener : mListeners) {
            listener.onPullExternalCall(this);
        }
    }

    /**
     * Sets this external call as active.
     */
    public void setActive() {
        if (mCall == null) {
            return;
        }
        mCall.setActive();
    }

    /**
     * Sets this external call as terminated.
     */
    public void setTerminated() {
        if (mCall == null) {
            return;
        }

        mCall.setTerminated();
    }

    /**
     * Changes whether the call can be pulled or not.
     *
     * @param isPullable {@code true} if the call can be pulled, {@code false} otherwise.
     */
    public void setIsPullable(boolean isPullable) {
        mIsPullable = isPullable;
        rebuildCapabilities();
    }

    /**
     * Sets the address of this external connection.  Ensures that dialog event package SIP
     * {@link Uri}s are converted to a regular telephone number.
     *
     * @param address The address from the dialog event package.
     */
    public void setExternalConnectionAddress(Uri address) {
        mOriginalAddress = address;

        if (PhoneAccount.SCHEME_SIP.equals(address.getScheme())) {
            if (address.getSchemeSpecificPart().startsWith(CONFERENCE_PREFIX)) {
                mCnapName = mContext.getString(com.android.internal.R.string.conference_call);
                mCnapNamePresentation = PhoneConstants.PRESENTATION_ALLOWED;
                mAddress = "";
                mNumberPresentation = PhoneConstants.PRESENTATION_RESTRICTED;
                return;
            }
        }
        Uri telUri = PhoneNumberUtils.convertSipUriToTelUri(address);
        mAddress = telUri.getSchemeSpecificPart();
    }

    public void addListener(Listener listener) {
        mListeners.add(listener);
    }

    public void removeListener(Listener listener) {
        mListeners.remove(listener);
    }

    /**
     * Build a human representation of a connection instance, suitable for debugging.
     * Don't log personal stuff unless in debug mode.
     * @return a string representing the internal state of this connection.
     */
    public String toString() {
        StringBuilder str = new StringBuilder(128);
        str.append("[ImsExternalConnection dialogCallId:");
        str.append(mCallId);
        str.append(" state:");
        if (mCall.getState() == Call.State.ACTIVE) {
            str.append("Active");
        } else if (mCall.getState() == Call.State.DISCONNECTED) {
            str.append("Disconnected");
        }
        str.append("]");
        return str.toString();
    }

    /**
     * Rebuilds the connection capabilities.
     */
    private void rebuildCapabilities() {
        int capabilities = Capability.IS_EXTERNAL_CONNECTION;
        if (mIsPullable) {
            capabilities |= Capability.IS_PULLABLE;
        }

        setConnectionCapabilities(capabilities);
    }
}
