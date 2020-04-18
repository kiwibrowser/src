// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.preference.PreferenceFragment;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.view.KeyEvent;
import android.widget.EditText;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.preferences.Preferences;
import org.chromium.chrome.browser.preferences.PreferencesTest;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.ui.UiUtils;

import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * Unit test suite for AutofillProfilesFragment.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class AutofillProfilesFragmentTest {
    @Rule
    public final AutofillTestRule rule = new AutofillTestRule();

    @Before
    public void setUp() throws InterruptedException, ExecutionException, TimeoutException {
        AutofillTestHelper helper = new AutofillTestHelper();
        helper.setProfile(new AutofillProfile("", "https://example.com", true, "Seb Doe", "Google",
                "111 First St", "CA", "Los Angeles", "", "90291", "", "US", "650-253-0000",
                "first@gmail.com", "en-US"));
        helper.setProfile(new AutofillProfile("", "https://example.com", true, "John Doe", "Google",
                "111 Second St", "CA", "Los Angeles", "", "90291", "", "US", "650-253-0000",
                "second@gmail.com", "en-US"));
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testAddProfile() throws Exception {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());
        AutofillProfilesFragment autofillProfileFragment =
                (AutofillProfilesFragment) activity.getFragmentForTest();

        // Check the preferences on the initial screen.
        Assert.assertEquals(3 /* One add button + two profiles. */,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference addProfile =
                (AutofillProfileEditorPreference) fragment.findPreference(
                        AutofillProfilesFragment.PREF_NEW_PROFILE);
        Assert.assertTrue(addProfile != null);

        // Add a profile.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferencesTest.clickPreference(autofillProfileFragment, addProfile);
                rule.setEditorDialog(
                        ((AutofillProfileEditorPreference) addProfile).getEditorDialog());
                try {
                    rule.setTextInEditorAndWait(new String[] {"Alice Doe", "Google", "111 Added St",
                            "Los Angeles", "CA", "90291", "650-253-0000", "add@profile.com"});
                    rule.clickInEditorAndWait(R.id.editor_dialog_done_button);
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
        // Check if the preferences are updated correctly.
        rule.waitForThePreferenceUpdate();
        Assert.assertEquals(4 /* One add button + three profiles. */,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        AutofillProfileEditorPreference addedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Alice Doe");
        Assert.assertTrue(addedProfile != null);
        Assert.assertEquals("111 Added St, 90291", addedProfile.getSummary());
        activity.finish();
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testAddIncompletedProfile() throws Exception {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());
        AutofillProfilesFragment autofillProfileFragment =
                (AutofillProfilesFragment) activity.getFragmentForTest();

        // Check the preferences on the initial screen.
        Assert.assertEquals(3 /* One add button + two profiles. */,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference addProfile =
                (AutofillProfileEditorPreference) fragment.findPreference(
                        AutofillProfilesFragment.PREF_NEW_PROFILE);
        Assert.assertTrue(addProfile != null);

        // Try to add an incomplete profile.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferencesTest.clickPreference(autofillProfileFragment, addProfile);
                rule.setEditorDialog(
                        ((AutofillProfileEditorPreference) addProfile).getEditorDialog());
                try {
                    rule.setTextInEditorAndWait(new String[] {"Mike Doe"});
                    rule.clickInEditorAndWaitForValidationError(R.id.editor_dialog_done_button);
                } catch (TimeoutException ex) {
                    // There should be no timeout, which means that there should be a validation
                    // error.
                    Assert.assertTrue(false);
                    ex.printStackTrace();
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
        activity.finish();
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testDeleteProfile() throws Exception {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());
        AutofillProfilesFragment autofillProfileFragment =
                (AutofillProfilesFragment) activity.getFragmentForTest();

        // Check the preferences on the initial screen.
        Assert.assertEquals(3 /* One add button + two profiles. */,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference sebProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Seb Doe");
        Assert.assertTrue(sebProfile != null);
        Assert.assertEquals("Seb Doe", sebProfile.getTitle());

        // Delete a profile.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferencesTest.clickPreference(autofillProfileFragment, sebProfile);
                rule.setEditorDialog(
                        ((AutofillProfileEditorPreference) sebProfile).getEditorDialog());
                try {
                    rule.clickInEditorAndWait(R.id.delete_menu_id);
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
        // Check if the preferences are updated correctly.
        rule.waitForThePreferenceUpdate();
        Assert.assertEquals(2 /* One add button + one profile. */,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        AutofillProfileEditorPreference remainedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("John Doe");
        Assert.assertTrue(remainedProfile != null);
        AutofillProfileEditorPreference deletedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Seb Doe");
        Assert.assertTrue(deletedProfile == null);
        activity.finish();
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testEditProfile() throws Exception {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());
        AutofillProfilesFragment autofillProfileFragment =
                (AutofillProfilesFragment) activity.getFragmentForTest();

        // Check the preferences on the initial screen.
        Assert.assertEquals(3 /* One add button + two profiles. */,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference johnProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("John Doe");
        Assert.assertTrue(johnProfile != null);
        Assert.assertEquals("John Doe", johnProfile.getTitle());

        // Edit a profile.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferencesTest.clickPreference(autofillProfileFragment, johnProfile);
                rule.setEditorDialog(
                        ((AutofillProfileEditorPreference) johnProfile).getEditorDialog());
                try {
                    rule.setTextInEditorAndWait(
                            new String[] {"Emily Doe", "Google", "111 Edited St", "Los Angeles",
                                    "CA", "90291", "650-253-0000", "edit@profile.com"});
                    rule.clickInEditorAndWait(R.id.editor_dialog_done_button);
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
        // Check if the preferences are updated correctly.
        rule.waitForThePreferenceUpdate();
        Assert.assertEquals(3 /* One add button + two profiles. */,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        AutofillProfileEditorPreference editedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Emily Doe");
        Assert.assertTrue(editedProfile != null);
        Assert.assertEquals("111 Edited St, 90291", editedProfile.getSummary());
        AutofillProfileEditorPreference oldProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("John Doe");
        Assert.assertTrue(oldProfile == null);
        activity.finish();
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testKeyboardShownOnDpadCenter() {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());

        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference addProfile =
                (AutofillProfileEditorPreference) fragment.findPreference(
                        AutofillProfilesFragment.PREF_NEW_PROFILE);
        Assert.assertNotNull(addProfile);

        // Open AutofillProfileEditorPreference.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            PreferencesTest.clickPreference(fragment, addProfile);
            rule.setEditorDialog(addProfile.getEditorDialog());
        });
        // The keyboard is shown as soon as AutofillProfileEditorPreference comes into view.
        waitForKeyboardStatus(true, activity);

        // Hide the keyboard.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            List<EditText> fields = addProfile.getEditorDialog().getEditableTextFieldsForTest();
            UiUtils.hideKeyboard(fields.get(0));
        });
        // Check that the keyboard is hidden.
        waitForKeyboardStatus(false, activity);

        // Send a d-pad key event to one of the text fields
        ThreadUtils.runOnUiThreadBlocking(() -> {
            try {
                rule.sendKeycodeToTextFieldInEditorAndWait(KeyEvent.KEYCODE_DPAD_CENTER, 0);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        });
        // Check that the keyboard was shown.
        waitForKeyboardStatus(true, activity);
        activity.finish();
    }

    private void waitForKeyboardStatus(final boolean keyboardVisible, final Preferences activity) {
        CriteriaHelper.pollUiThread(
                new Criteria("Keyboard was not " + (keyboardVisible ? "shown." : "hidden.")) {
                    @Override
                    public boolean isSatisfied() {
                        return keyboardVisible
                                == UiUtils.isKeyboardShowing(
                                           activity, activity.findViewById(android.R.id.content));
                    }
                });
    }
}
