// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.os.Bundle;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/**
 * Tests for the "Save Passwords" settings screen.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class PasswordReauthenticationFragmentTest {
    // All reauthentication scopes to be checked in the tests.
    private static final int[] ALL_SCOPES = {ReauthenticationManager.REAUTH_SCOPE_ONE_AT_A_TIME,
            ReauthenticationManager.REAUTH_SCOPE_BULK};

    /**
     * Creates a dummy fragment, pushes the reauth fragment on top of it, then resolves the activity
     * for the reauth fragment and checks that back stack is in a correct state.
     * @param resultCode The code which is passed to the reauth fragment as the result of the
     *                   activity.
     * @param scope The scope of the reauthentication.
     */
    private void checkPopFromBackStackOnResult(
            int resultCode, @ReauthenticationManager.ReauthScope int scope) {
        PasswordReauthenticationFragment passwordReauthentication =
                new PasswordReauthenticationFragment();
        Bundle args = new Bundle();
        args.putInt(PasswordReauthenticationFragment.DESCRIPTION_ID, 0);
        args.putSerializable(PasswordReauthenticationFragment.SCOPE_ID, scope);
        passwordReauthentication.setArguments(args);

        // Replacement fragment for PasswordEntryEditor, which is the fragment that
        // replaces PasswordReauthentication after popBackStack is called.
        Fragment mockPasswordEntryEditor = new Fragment();

        Activity testActivity = Robolectric.setupActivity(Activity.class);
        Intent returnIntent = new Intent();
        returnIntent.putExtra("result", "This is the result");
        PasswordReauthenticationFragment.preventLockingForTesting();

        FragmentManager fragmentManager = testActivity.getFragmentManager();
        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        fragmentTransaction.add(mockPasswordEntryEditor, "password_entry_editor");
        fragmentTransaction.addToBackStack("add_password_entry_editor");
        fragmentTransaction.commit();

        FragmentTransaction fragmentTransaction2 = fragmentManager.beginTransaction();
        fragmentTransaction2.add(passwordReauthentication, "password_reauthentication");
        fragmentTransaction2.addToBackStack("add_password_reauthentication");
        fragmentTransaction2.commit();

        passwordReauthentication.onActivityResult(
                PasswordReauthenticationFragment.CONFIRM_DEVICE_CREDENTIAL_REQUEST_CODE, resultCode,
                returnIntent);
        fragmentManager.executePendingTransactions();

        // Assert that the number of fragments in the Back Stack is equal to 1 after
        // reauthentication, as PasswordReauthenticationFragment is popped.
        assertEquals(1, fragmentManager.getBackStackEntryCount());

        // Assert that the remaining fragment in the Back Stack is PasswordEntryEditor.
        assertEquals("add_password_entry_editor", fragmentManager.getBackStackEntryAt(0).getName());
    }

    /**
     * Ensure that upon successful reauthentication PasswordReauthenticationFragment is popped from
     * the FragmentManager backstack and the reauthentication is marked as valid.
     */
    @Test
    public void testOnOkActivityResult() {
        for (int scope : ALL_SCOPES) {
            // Ensure that the reauthentication state is changed by setting it to fail the final
            // expectation.
            ReauthenticationManager.resetLastReauth();

            checkPopFromBackStackOnResult(Activity.RESULT_OK, scope);
            assertTrue(ReauthenticationManager.authenticationStillValid(scope));
        }
    }

    /**
     * Ensure that upon canceled reauthentication PasswordReauthenticationFragment is popped from
     * the FragmentManager backstack and the reauthentication is marked as invalid.
     */
    @Test
    public void testOnCanceledActivityResult() {
        for (int scope : ALL_SCOPES) {
            // Ensure that the reauthentication state is changed by setting it to fail the final
            // expectation.
            ReauthenticationManager.recordLastReauth(System.currentTimeMillis(), scope);

            checkPopFromBackStackOnResult(Activity.RESULT_CANCELED, scope);
            assertFalse(ReauthenticationManager.authenticationStillValid(scope));
        }
    }
}
