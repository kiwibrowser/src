// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting.jni;

import org.chromium.chromoting.R;

/** Interface used for connection state notifications. */
@SuppressWarnings("JavaLangClash")
public interface ConnectionListener {
    /**
     * This enum must match the C++ enumeration remoting::protocol::ConnectionToHost::State.
     */
    public enum State {
        INITIALIZING(0),
        CONNECTING(1),
        AUTHENTICATED(2),
        CONNECTED(3),
        FAILED(4),
        CLOSED(5);

        private final int mValue;

        State(int value) {
            mValue = value;
        }

        public int value() {
            return mValue;
        }

        public static State fromValue(int value) {
            return values()[value];
        }
    }

    /**
     * This enum must match the C++ enumeration remoting::protocol::ErrorCode.
     */
    public enum Error {
        OK(0, 0),
        PEER_IS_OFFLINE(1, R.string.error_host_is_offline),
        SESSION_REJECTED(2, R.string.error_invalid_access_code),
        INCOMPATIBLE_PROTOCOL(3, R.string.error_incompatible_protocol),
        AUTHENTICATION_FAILED(4, R.string.error_invalid_access_code),
        INVALID_ACCOUNT(5, R.string.error_invalid_account),
        CHANNEL_CONNECTION_ERROR(6, R.string.error_p2p_failure),
        SIGNALING_ERROR(7, R.string.error_p2p_failure),
        SIGNALING_TIMEOUT(8, R.string.error_p2p_failure),
        HOST_OVERLOAD(9, R.string.error_host_overload),
        MAX_SESSION_LENGTH(10, R.string.error_max_session_length),
        HOST_CONFIGURATION_ERROR(11, R.string.error_host_configuration_error),
        UNKNOWN_ERROR(12, R.string.error_unexpected);

        private final int mValue;
        private final int mMessage;

        Error(int value, int message) {
            mValue = value;
            mMessage = message;
        }

        public int value() {
            return mValue;
        }

        public int message() {
            return mMessage;
        }

        public static Error fromValue(int value) {
            return values()[value];
        }
    }

    /**
     * Notified on connection state change.
     * @param state The new connection state.
     * @param error The error code, if state is FAILED.
     */
    void onConnectionState(State state, Error error);
}
