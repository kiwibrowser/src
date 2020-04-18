// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.action.ViewActions.click;
import static android.support.test.espresso.action.ViewActions.scrollTo;
import static android.support.test.espresso.matcher.ViewMatchers.withText;

import static org.chromium.ui.base.LocalizationUtils.setRtlForTesting;

import android.os.Build;
import android.support.test.filters.MediumTest;
import android.view.View;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.UiUtils;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Integration tests for autofill keyboard accessory.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@EnableFeatures({ChromeFeatureList.AUTOFILL_KEYBOARD_ACCESSORY})
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AutofillKeyboardAccessoryIntegrationTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);
    @Rule
    public TestRule mFeaturesProcessor = new Features.InstrumentationProcessor();

    private final AtomicReference<WebContents> mWebContentsRef = new AtomicReference<>();
    private final AtomicReference<ViewGroup> mContainerRef = new AtomicReference<>();

    private void loadTestPage(boolean isRtl)
            throws InterruptedException, ExecutionException, TimeoutException {
        mActivityTestRule.startMainActivityWithURL(UrlUtils.encodeHtmlDataUri("<html"
                + (isRtl ? " dir=\"rtl\"" : "") + "><head>"
                + "<meta name=\"viewport\""
                + "content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0\" /></head>"
                + "<body><form method=\"POST\">"
                + "<input type=\"text\" id=\"fn\" autocomplete=\"given-name\" autofocus/><br>"
                + "<input type=\"text\" id=\"ln\" autocomplete=\"family-name\" /><br>"
                + "<textarea id=\"sa\" autocomplete=\"street-address\"></textarea><br>"
                + "<input type=\"text\" id=\"a1\" autocomplete=\"address-line1\" /><br>"
                + "<input type=\"text\" id=\"a2\" autocomplete=\"address-line2\" /><br>"
                + "<input type=\"text\" id=\"ct\" autocomplete=\"address-level2\" /><br>"
                + "<input type=\"text\" id=\"zc\" autocomplete=\"postal-code\" /><br>"
                + "<input type=\"text\" id=\"em\" autocomplete=\"email\" /><br>"
                + "<input type=\"text\" id=\"ph\" autocomplete=\"tel\" /><br>"
                + "<input type=\"text\" id=\"fx\" autocomplete=\"fax\" /><br>"
                + "<select id=\"co\" autocomplete=\"country\"><br>"
                + "<option value=\"BR\">Brazil</option>"
                + "<option value=\"US\">United States</option>"
                + "</select>"
                + "<input type=\"submit\" />"
                + "</form></body></html>"));
        new AutofillTestHelper().setProfile(new AutofillProfile("", "https://www.example.com",
                "Johnathan Smithonian-Jackson", "Acme Inc", "1 Main\nApt A", "CA", "San Francisco",
                "", "94102", "", "US", "(415) 888-9999", "john@acme.inc", "en"));
        new AutofillTestHelper().setProfile(new AutofillProfile("", "https://www.example.com",
                "Jane Erika Donovanova", "Acme Inc", "1 Main\nApt A", "CA", "San Francisco", "",
                "94102", "", "US", "(415) 999-0000", "jane@acme.inc", "en"));
        new AutofillTestHelper().setProfile(new AutofillProfile("", "https://www.example.com",
                "Marcus McSpartangregor", "Acme Inc", "1 Main\nApt A", "CA", "San Francisco", "",
                "94102", "", "US", "(415) 999-0000", "marc@acme.inc", "en"));
        setRtlForTesting(isRtl);
        ThreadUtils.runOnUiThreadBlocking(() -> {
            Tab tab = mActivityTestRule.getActivity().getActivityTab();
            mWebContentsRef.set(tab.getWebContents());
            mContainerRef.set(tab.getContentView());
        });
        DOMUtils.waitForNonZeroNodeBounds(mWebContentsRef.get(), "fn");
    }

    /**
     * Autofocused fields should not show a keyboard accessory.
     */
    @Test
    @MediumTest
    @Feature({"keyboard-accessory"})
    public void testAutofocusedFieldDoesNotShowKeyboardAccessory()
            throws ExecutionException, InterruptedException, TimeoutException {
        loadTestPage(false);
        Assert.assertTrue("Keyboard accessory should be hidden.", isAccessoryGone());
    }

    /**
     * Tapping on an input field should show a keyboard and its keyboard accessory.
     */
    @Test
    @MediumTest
    @Feature({"keyboard-accessory"})
    public void testTapInputFieldShowsKeyboardAccessory()
            throws ExecutionException, InterruptedException, TimeoutException {
        loadTestPage(false);
        DOMUtils.clickNode(mWebContentsRef.get(), "fn");

        CriteriaHelper.pollUiThread(Criteria.equals(true,
                ()
                        -> UiUtils.isKeyboardShowing(
                                mActivityTestRule.getActivity(), mContainerRef.get())));
        Assert.assertTrue("Keyboard accessory should be showing.", isAccessoryVisible());
    }

    /**
     * Switching fields should re-scroll the keyboard accessory to the left.
     */
    @Test
    @MediumTest
    @Feature({"keyboard-accessory"})
    @DisabledTest(message = "crbug.com/836027")
    public void testSwitchFieldsRescrollsKeyboardAccessory()
            throws ExecutionException, InterruptedException, TimeoutException {
        loadTestPage(false);
        DOMUtils.clickNode(mWebContentsRef.get(), "fn");

        CriteriaHelper.pollUiThread(Criteria.equals(true,
                ()
                        -> UiUtils.isKeyboardShowing(
                                mActivityTestRule.getActivity(), mContainerRef.get())));

        ThreadUtils.runOnUiThreadBlocking(() -> getSuggestionsComponent().scrollTo(2000, 0));
        assertSuggestionScrollPosition(
                false, "First suggestion should be off the screen after manual scroll.");

        DOMUtils.clickNode(mWebContentsRef.get(), "ln");
        assertSuggestionScrollPosition(
                true, "First suggestion should be on the screen after switching fields.");
    }

    /**
     * Switching fields in RTL should re-scroll the keyboard accessory to the right.
     *
     * RTL is only supported on Jelly Bean MR 1+.
     * http://android-developers.blogspot.com/2013/03/native-rtl-support-in-android-42.html
     */
    @Test
    @MediumTest
    @Feature({"keyboard-accessory"})
    @MinAndroidSdkLevel(Build.VERSION_CODES.JELLY_BEAN_MR1)
    @DisabledTest(message = "crbug.com/836027")
    public void testSwitchFieldsRescrollsKeyboardAccessoryRtl()
            throws ExecutionException, InterruptedException, TimeoutException {
        loadTestPage(true);
        DOMUtils.clickNode(mWebContentsRef.get(), "fn");

        CriteriaHelper.pollUiThread(Criteria.equals(true,
                ()
                        -> UiUtils.isKeyboardShowing(
                                mActivityTestRule.getActivity(), mContainerRef.get())));
        assertSuggestionScrollPosition(false, "Last suggestion should be off the screen intially.");

        ThreadUtils.runOnUiThreadBlocking(() -> getSuggestionsComponent().scrollTo(-500, 0));
        assertSuggestionScrollPosition(
                true, "Last suggestion should be on the screen after manual scroll.");

        DOMUtils.clickNode(mWebContentsRef.get(), "ln");
        assertSuggestionScrollPosition(
                false, "Last suggestion should be off the screen after switching fields.");
    }

    /**
     * Selecting a keyboard accessory suggestion should hide the keyboard and its keyboard
     * accessory.
     */
    @Test
    @MediumTest
    @Feature({"keyboard-accessory"})
    public void testSelectSuggestionHidesKeyboardAccessory()
            throws ExecutionException, InterruptedException, TimeoutException {
        loadTestPage(false);
        DOMUtils.clickNode(mWebContentsRef.get(), "fn");

        CriteriaHelper.pollUiThread(Criteria.equals(true,
                ()
                        -> UiUtils.isKeyboardShowing(
                                mActivityTestRule.getActivity(), mContainerRef.get())));
        Assert.assertTrue("Keyboard accessory should be visible.", isAccessoryVisible());

        onView(withText("Marcus")).perform(scrollTo(), click());

        CriteriaHelper.pollUiThread(Criteria.equals(false,
                ()
                        -> UiUtils.isKeyboardShowing(
                                mActivityTestRule.getActivity(), mContainerRef.get())));
        Assert.assertTrue("Keyboard accessory should be hidden.", isAccessoryGone());
    }

    private void assertSuggestionScrollPosition(boolean shouldBeOnScreen, String failureReason) {
        CriteriaHelper.pollUiThread(new Criteria(failureReason) {
            @Override
            public boolean isSatisfied() {
                View suggestion = getSuggestionAt(0);
                if (suggestion == null) return false;
                int[] location = new int[2];
                suggestion.getLocationOnScreen(location);
                return shouldBeOnScreen ? location[0] > 0 : location[0] < 0;
            }
        });
    }

    private HorizontalScrollView getSuggestionsComponent() {
        final ViewGroup keyboardAccessory = ThreadUtils.runOnUiThreadBlockingNoException(
                () -> mActivityTestRule.getActivity().findViewById(R.id.keyboard_accessory));
        if (keyboardAccessory == null) return null; // It might still be loading, so don't assert!

        final View scrollview = keyboardAccessory.findViewById(R.id.suggestions_view);
        if (scrollview == null) return null; // It might still be loading, so don't assert!

        return (HorizontalScrollView) scrollview;
    }

    private View getSuggestionAt(int index) {
        ViewGroup scrollview = getSuggestionsComponent();
        if (scrollview == null) return null; // It might still be loading, so don't assert!

        return scrollview.getChildAt(index);
    }

    private boolean isAccessoryVisible() throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(() -> {
            LinearLayout keyboard =
                    mActivityTestRule.getActivity().findViewById(R.id.keyboard_accessory);
            return keyboard != null && keyboard.getVisibility() == View.VISIBLE;
        });
    }

    private boolean isAccessoryGone() throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(() -> {
            LinearLayout keyboard =
                    mActivityTestRule.getActivity().findViewById(R.id.keyboard_accessory);
            return keyboard == null || keyboard.getVisibility() == View.GONE;
        });
    }
}
