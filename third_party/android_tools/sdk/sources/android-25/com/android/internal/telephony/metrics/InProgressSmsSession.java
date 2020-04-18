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
 * limitations under the License.
 */

package com.android.internal.telephony.metrics;

import android.os.SystemClock;

import com.android.internal.telephony.TelephonyProto.SmsSession;

import java.util.ArrayDeque;
import java.util.Deque;
import java.util.concurrent.atomic.AtomicInteger;

/** The ongoing SMS session */
public class InProgressSmsSession {

    /** Maximum events stored in the session */
    private static final int MAX_EVENTS = 20;

    /** Phone id */
    public final int phoneId;

    /** SMS session events */
    public final Deque<SmsSession.Event> events;

    /** Sms session starting system time in minute */
    public final int startSystemTimeMin;

    /** Sms session starting elapsed time in milliseconds */
    public final long startElapsedTimeMs;

    /** The last event's time */
    private long mLastElapsedTimeMs;

    /** Indicating events dropped */
    private boolean mEventsDropped = false;

    /** The expected SMS response #. One session could contain multiple SMS requests/responses. */
    private AtomicInteger mNumExpectedResponses = new AtomicInteger(0);

    /** Increase the expected response # */
    public void increaseExpectedResponse() {
        mNumExpectedResponses.incrementAndGet();
    }

    /** Decrease the expected response # */
    public void decreaseExpectedResponse() {
        mNumExpectedResponses.decrementAndGet();
    }

    /** Get the expected response # */
    public int getNumExpectedResponses() {
        return mNumExpectedResponses.get();
    }

    /** Check if events dropped */
    public boolean isEventsDropped() { return mEventsDropped; }

    /**
     * Constructor
     *
     * @param phoneId Phone id
     */
    public InProgressSmsSession(int phoneId) {
        this.phoneId = phoneId;
        events = new ArrayDeque<>();
        // Save session start with lowered precision due to the privacy requirements
        startSystemTimeMin = TelephonyMetrics.roundSessionStart(System.currentTimeMillis());
        startElapsedTimeMs = SystemClock.elapsedRealtime();
        mLastElapsedTimeMs = startElapsedTimeMs;
    }

    /**
     * Add event
     *
     * @param builder Event builder
     */
    public void addEvent(SmsSessionEventBuilder builder) {
        addEvent(SystemClock.elapsedRealtime(), builder);
    }

    /**
     * Add event
     *
     * @param timestamp Timestamp to be recoded with the event
     * @param builder Event builder
     */
    public synchronized void addEvent(long timestamp, SmsSessionEventBuilder builder) {
        if (events.size() >= MAX_EVENTS) {
            events.removeFirst();
            mEventsDropped = true;
        }

        builder.setDelay(TelephonyMetrics.toPrivacyFuzzedTimeInterval(
                mLastElapsedTimeMs, timestamp));

        events.add(builder.build());
        mLastElapsedTimeMs = timestamp;
    }
}
