// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import static org.junit.Assert.assertNotNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;

import android.support.v7.widget.RecyclerView;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/**
 * Controller tests for the password accessory sheet.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class PasswordAccessorySheetControllerTest {
    @Mock
    private RecyclerView mMockView;

    private PasswordAccessorySheetCoordinator mCoordinator;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mCoordinator = new PasswordAccessorySheetCoordinator(RuntimeEnvironment.application);
        assertNotNull(mCoordinator);
    }

    @Test
    public void testIsValidTab() {
        assertNotNull(mCoordinator.getIcon());
        assertNotNull(mCoordinator.getListener());
    }

    @Test
    public void testSetsViewAdapterOnTabCreation() {
        assertNotNull(mCoordinator.getListener());
        mCoordinator.getListener().onTabCreated(mMockView);
        verify(mMockView).setAdapter(any());
    }
}