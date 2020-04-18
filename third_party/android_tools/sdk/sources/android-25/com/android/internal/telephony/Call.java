/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.internal.telephony;

import android.telecom.ConferenceParticipant;

import java.util.ArrayList;
import java.util.List;

import android.telephony.Rlog;

/**
 * {@hide}
 */
public abstract class Call {
    protected final String LOG_TAG = "Call";

    /* Enums */

    public enum State {
        IDLE, ACTIVE, HOLDING, DIALING, ALERTING, INCOMING, WAITING, DISCONNECTED, DISCONNECTING;

        public boolean isAlive() {
            return !(this == IDLE || this == DISCONNECTED || this == DISCONNECTING);
        }

        public boolean isRinging() {
            return this == INCOMING || this == WAITING;
        }

        public boolean isDialing() {
            return this == DIALING || this == ALERTING;
        }
    }

    public static State
    stateFromDCState (DriverCall.State dcState) {
        switch (dcState) {
            case ACTIVE:        return State.ACTIVE;
            case HOLDING:       return State.HOLDING;
            case DIALING:       return State.DIALING;
            case ALERTING:      return State.ALERTING;
            case INCOMING:      return State.INCOMING;
            case WAITING:       return State.WAITING;
            default:            throw new RuntimeException ("illegal call state:" + dcState);
        }
    }

    public enum SrvccState {
        NONE, STARTED, COMPLETED, FAILED, CANCELED;
    }

    /* Instance Variables */

    public State mState = State.IDLE;

    public ArrayList<Connection> mConnections = new ArrayList<Connection>();

    /* Instance Methods */

    /** Do not modify the List result!!! This list is not yours to keep
     *  It will change across event loop iterations            top
     */

    public abstract List<Connection> getConnections();
    public abstract Phone getPhone();
    public abstract boolean isMultiparty();
    public abstract void hangup() throws CallStateException;


    /**
     * hasConnection
     *
     * @param c a Connection object
     * @return true if the call contains the connection object passed in
     */
    public boolean hasConnection(Connection c) {
        return c.getCall() == this;
    }

    /**
     * hasConnections
     * @return true if the call contains one or more connections
     */
    public boolean hasConnections() {
        List<Connection> connections = getConnections();

        if (connections == null) {
            return false;
        }

        return connections.size() > 0;
    }

    /**
     * getState
     * @return state of class call
     */
    public State getState() {
        return mState;
    }

    /**
     * getConferenceParticipants
     * @return List of conference participants.
     */
    public List<ConferenceParticipant> getConferenceParticipants() {
        return null;
    }

    /**
     * isIdle
     *
     * FIXME rename
     * @return true if the call contains only disconnected connections (if any)
     */
    public boolean isIdle() {
        return !getState().isAlive();
    }

    /**
     * Returns the Connection associated with this Call that was created
     * first, or null if there are no Connections in this Call
     */
    public Connection
    getEarliestConnection() {
        List<Connection> l;
        long time = Long.MAX_VALUE;
        Connection c;
        Connection earliest = null;

        l = getConnections();

        if (l.size() == 0) {
            return null;
        }

        for (int i = 0, s = l.size() ; i < s ; i++) {
            c = l.get(i);
            long t;

            t = c.getCreateTime();

            if (t < time) {
                earliest = c;
                time = t;
            }
        }

        return earliest;
    }

    public long
    getEarliestCreateTime() {
        List<Connection> l;
        long time = Long.MAX_VALUE;

        l = getConnections();

        if (l.size() == 0) {
            return 0;
        }

        for (int i = 0, s = l.size() ; i < s ; i++) {
            Connection c = l.get(i);
            long t;

            t = c.getCreateTime();

            time = t < time ? t : time;
        }

        return time;
    }

    public long
    getEarliestConnectTime() {
        long time = Long.MAX_VALUE;
        List<Connection> l = getConnections();

        if (l.size() == 0) {
            return 0;
        }

        for (int i = 0, s = l.size() ; i < s ; i++) {
            Connection c = l.get(i);
            long t;

            t = c.getConnectTime();

            time = t < time ? t : time;
        }

        return time;
    }


    public boolean
    isDialingOrAlerting() {
        return getState().isDialing();
    }

    public boolean
    isRinging() {
        return getState().isRinging();
    }

    /**
     * Returns the Connection associated with this Call that was created
     * last, or null if there are no Connections in this Call
     */
    public Connection
    getLatestConnection() {
        List<Connection> l = getConnections();
        if (l.size() == 0) {
            return null;
        }

        long time = 0;
        Connection latest = null;
        for (int i = 0, s = l.size() ; i < s ; i++) {
            Connection c = l.get(i);
            long t = c.getCreateTime();

            if (t > time) {
                latest = c;
                time = t;
            }
        }

        return latest;
    }

    /**
     * Hangup call if it is alive
     */
    public void hangupIfAlive() {
        if (getState().isAlive()) {
            try {
                hangup();
            } catch (CallStateException ex) {
                Rlog.w(LOG_TAG, " hangupIfActive: caught " + ex);
            }
        }
    }

    /**
     * Called when it's time to clean up disconnected Connection objects
     */
    public void clearDisconnected() {
        for (int i = mConnections.size() - 1 ; i >= 0 ; i--) {
            Connection c = mConnections.get(i);
            if (c.getState() == State.DISCONNECTED) {
                mConnections.remove(i);
            }
        }

        if (mConnections.size() == 0) {
            setState(State.IDLE);
        }
    }

    protected void setState(State newState) {
        mState = newState;
    }
}
