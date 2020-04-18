/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.setupwizardlib.test;

import android.app.Activity;
import android.content.Intent;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.setupwizardlib.util.WizardManagerHelper;

public class WizardManagerHelperTest extends AndroidTestCase {

    @SmallTest
    public void testGetNextIntent() {
        final Intent intent = new Intent("test.intent.ACTION");
        intent.putExtra("scriptUri", "android-resource://test-script");
        intent.putExtra("actionId", "test_action_id");
        intent.putExtra("theme", "test_theme");
        intent.putExtra("ignoreExtra", "poof"); // extra is ignored because it's not known

        final Intent data = new Intent();
        data.putExtra("extraData", "shazam");

        final Intent nextIntent =
                WizardManagerHelper.getNextIntent(intent, Activity.RESULT_OK, data);
        assertEquals("Next intent action should be NEXT", "com.android.wizard.NEXT",
                nextIntent.getAction());
        assertEquals("Script URI should be the same as original intent",
                "android-resource://test-script", nextIntent.getStringExtra("scriptUri"));
        assertEquals("Action ID should be the same as original intent", "test_action_id",
                nextIntent.getStringExtra("actionId"));
        assertEquals("Theme extra should be the same as original intent", "test_theme",
                nextIntent.getStringExtra("theme"));
        assertFalse("ignoreExtra should not be in nextIntent", nextIntent.hasExtra("ignoreExtra"));
        assertEquals("Result code extra should be RESULT_OK", Activity.RESULT_OK,
                nextIntent.getIntExtra("com.android.setupwizard.ResultCode", 0));
        assertEquals("Extra data should surface as extra in nextIntent", "shazam",
                nextIntent.getStringExtra("extraData"));
    }

    @SmallTest
    public void testIsSetupWizardTrue() {
        final Intent intent = new Intent();
        intent.putExtra("firstRun", true);
        assertTrue("Is setup wizard should be true",
                WizardManagerHelper.isSetupWizardIntent(intent));
    }

    @SmallTest
    public void testIsSetupWizardFalse() {
        final Intent intent = new Intent();
        intent.putExtra("firstRun", false);
        assertFalse("Is setup wizard should be true",
                WizardManagerHelper.isSetupWizardIntent(intent));
    }

    @SmallTest
    public void testHoloIsNotLightTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "holo");
        assertFalse("Theme holo should not be light theme",
                WizardManagerHelper.isLightTheme(intent, true));
    }

    @SmallTest
    public void testHoloLightIsLightTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "holo_light");
        assertTrue("Theme holo_light should be light theme",
                WizardManagerHelper.isLightTheme(intent, false));
    }

    @SmallTest
    public void testMaterialIsNotLightTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "material");
        assertFalse("Theme material should not be light theme",
                WizardManagerHelper.isLightTheme(intent, true));
    }

    @SmallTest
    public void testMaterialLightIsLightTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "material_light");
        assertTrue("Theme material_light should be light theme",
                WizardManagerHelper.isLightTheme(intent, false));
    }

    @SmallTest
    public void testMaterialBlueIsNotLightTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "material_blue");
        assertFalse("Theme material_blue should not be light theme",
                WizardManagerHelper.isLightTheme(intent, true));
    }

    @SmallTest
    public void testMaterialBlueLightIsLightTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "material_blue_light");
        assertTrue("Theme material_blue_light should be light theme",
                WizardManagerHelper.isLightTheme(intent, false));
    }

    @SmallTest
    public void testGlifIsDarkTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "glif");
        assertFalse("Theme glif should be dark theme",
                WizardManagerHelper.isLightTheme(intent, false));
        assertFalse("Theme glif should be dark theme",
                WizardManagerHelper.isLightTheme(intent, true));
    }

    @SmallTest
    public void testGlifLightIsLightTheme() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "glif_light");
        assertTrue("Theme glif_light should be light theme",
                WizardManagerHelper.isLightTheme(intent, false));
        assertTrue("Theme glif_light should be light theme",
                WizardManagerHelper.isLightTheme(intent, true));
    }

    @SmallTest
    public void testIsLightThemeDefault() {
        final Intent intent = new Intent();
        intent.putExtra("theme", "abracadabra");
        assertTrue("isLightTheme should return default value true",
                WizardManagerHelper.isLightTheme(intent, true));
        assertFalse("isLightTheme should return default value false",
                WizardManagerHelper.isLightTheme(intent, false));
    }

    @SmallTest
    public void testIsLightThemeUnspecified() {
        final Intent intent = new Intent();
        assertTrue("isLightTheme should return default value true",
                WizardManagerHelper.isLightTheme(intent, true));
        assertFalse("isLightTheme should return default value false",
                WizardManagerHelper.isLightTheme(intent, false));
    }

    @SmallTest
    public void testIsLightThemeString() {
        assertTrue("isLightTheme should return true for material_light",
                WizardManagerHelper.isLightTheme("material_light", false));
        assertFalse("isLightTheme should return false for material",
                WizardManagerHelper.isLightTheme("material", false));
        assertTrue("isLightTheme should return true for holo_light",
                WizardManagerHelper.isLightTheme("holo_light", false));
        assertFalse("isLightTheme should return false for holo",
                WizardManagerHelper.isLightTheme("holo", false));
        assertTrue("isLightTheme should return default value true",
                WizardManagerHelper.isLightTheme("abracadabra", true));
        assertFalse("isLightTheme should return default value false",
                WizardManagerHelper.isLightTheme("abracadabra", false));
    }
}
