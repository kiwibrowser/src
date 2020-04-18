// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.metrics;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;

/**
 * Tests for VariationsSession
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class VariationsSessionTest {
    private TestVariationsSession mSession;

    private static class TestVariationsSession extends VariationsSession {
        private Callback<String> mCallback;

        @Override
        protected void getRestrictMode(Context context, Callback<String> callback) {
            mCallback = callback;
        }

        public void runCallback(String value) {
            mCallback.onResult(value);
        }

        @Override
        protected void nativeStartVariationsSession(String restrictMode) {
            // No-op for tests.
        }
    }

    @Before
    public void setUp() {
        mSession = spy(new TestVariationsSession());
    }

    @Test
    public void testStart() {
        mSession.start(ContextUtils.getApplicationContext());
        verify(mSession, never()).nativeStartVariationsSession(any(String.class));

        String restrictValue = "test";
        mSession.runCallback(restrictValue);
        verify(mSession, times(1)).nativeStartVariationsSession(restrictValue);
    }

    @Test
    public void testGetRestrictModeValue() {
        mSession.getRestrictModeValue(ContextUtils.getApplicationContext(), new Callback<String>() {
            @Override
            public void onResult(String restrictMode) {}
        });
        String restrictValue = "test";
        mSession.runCallback(restrictValue);
        verify(mSession, never()).nativeStartVariationsSession(any(String.class));

        mSession.start(ContextUtils.getApplicationContext());
        verify(mSession, times(1)).nativeStartVariationsSession(restrictValue);
    }
}
