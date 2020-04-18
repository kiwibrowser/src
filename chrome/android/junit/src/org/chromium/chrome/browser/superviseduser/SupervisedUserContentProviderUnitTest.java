// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.superviseduser;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.accounts.Account;
import android.content.Context;
import android.content.pm.ProviderInfo;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.DisableHistogramsRule;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.superviseduser.SupervisedUserContentProvider.SupervisedUserQueryReply;
import org.chromium.components.signin.AccountManagerDelegate;
import org.chromium.components.signin.AccountManagerDelegateException;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.components.webrestrictions.browser.WebRestrictionsContentProvider.WebRestrictionsResult;

/**
 * Tests of SupervisedUserContentProvider. This is tested as a simple class, not as a content
 * provider. The content provider aspects are tested with WebRestrictionsContentProviderTest.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SupervisedUserContentProviderUnitTest {
    @Rule
    public DisableHistogramsRule mDisableHistogramsRule = new DisableHistogramsRule();

    private SupervisedUserContentProvider mSupervisedUserContentProvider;

    private static final String DEFAULT_CALLING_PACKAGE = "com.example.some.app";

    // Override methods that wrap things that can't be mocked (including native calls).
    private static class MySupervisedUserContentProvider extends SupervisedUserContentProvider {
        @Override
        void startForcedSigninProcessor(Context context, Runnable onComplete) {
            ChromeSigninController.get().setSignedInAccountName("Dummy");
            onComplete.run();
        }

        @Override
        void listenForChildAccountStatusChange(Callback<Boolean> callback) {
            callback.onResult(true);
        }

        @Override
        void nativeShouldProceed(long l, SupervisedUserQueryReply reply, String url) {
            reply.onQueryComplete();
        }

        @Override
        void nativeRequestInsert(long l, SupervisedUserInsertReply reply, String url) {
            reply.onInsertRequestSendComplete(true);
        }

        @Override
        long nativeCreateSupervisedUserContentProvider() {
            return 5678L;
        }
    }

    @Before
    public void setUp() {

        // Ensure clean state (in particular not signed in).
        ContextUtils.getAppSharedPreferences().edit().clear().apply();

        // Spy on the content provider so that we can watch its calls.
        ProviderInfo info = new ProviderInfo();
        info.authority = "foo.bar.baz";
        mSupervisedUserContentProvider =
                Mockito.spy(Robolectric.buildContentProvider(MySupervisedUserContentProvider.class)
                                    .create(info)
                                    .get());
    }

    @After
    public void shutDown() {
        ContextUtils.getAppSharedPreferences().edit().clear().apply();
        ChromeBrowserInitializer.setForTesting(null);
    }

    @Test
    public void testShouldProceed_PermittedUrl() {
        mSupervisedUserContentProvider.setNativeSupervisedUserContentProviderForTesting(1234L);
        // Mock the native call for a permitted URL
        WebRestrictionsResult result =
                mSupervisedUserContentProvider.shouldProceed(DEFAULT_CALLING_PACKAGE, "url");
        assertThat(result.shouldProceed(), is(true));
        verify(mSupervisedUserContentProvider)
                .nativeShouldProceed(eq(1234L),
                        any(SupervisedUserContentProvider.SupervisedUserQueryReply.class),
                        eq("url"));
    }

    @Test
    public void testShouldProceed_ForbiddenUrl() {
        mSupervisedUserContentProvider.setNativeSupervisedUserContentProviderForTesting(1234L);
        // Modify the result of the native call to make this a forbidden URL
        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                invocation.<SupervisedUserQueryReply>getArgument(1).onQueryFailed(1, 2, 3, "url1",
                        "url2", "custodian", "custodianEmail", "secondCustodian",
                        "secondCustodianEmail");
                return null;
            }

        })
                .when(mSupervisedUserContentProvider)
                .nativeShouldProceed(anyLong(),
                        any(SupervisedUserContentProvider.SupervisedUserQueryReply.class),
                        anyString());
        WebRestrictionsResult result =
                mSupervisedUserContentProvider.shouldProceed(DEFAULT_CALLING_PACKAGE, "url");
        assertThat(result.shouldProceed(), is(false));
        assertThat(result.errorIntCount(), is(3));
        assertThat(result.getErrorInt(0), is(1));
        assertThat(result.getErrorInt(1), is(2));
        assertThat(result.getErrorInt(2), is(3));
        assertThat(result.errorStringCount(), is(6));
        assertThat(result.getErrorString(0), is("url1"));
        assertThat(result.getErrorString(1), is("url2"));
        assertThat(result.getErrorString(2), is("custodian"));
        assertThat(result.getErrorString(3), is("custodianEmail"));
        assertThat(result.getErrorString(4), is("secondCustodian"));
        assertThat(result.getErrorString(5), is("secondCustodianEmail"));
    }

    @Test
    public void testRequestInsert_ok() {
        mSupervisedUserContentProvider.setNativeSupervisedUserContentProviderForTesting(1234L);

        assertThat(mSupervisedUserContentProvider.requestInsert("url"), is(true));

        verify(mSupervisedUserContentProvider)
                .nativeRequestInsert(eq(1234L),
                        any(SupervisedUserContentProvider.SupervisedUserInsertReply.class),
                        eq("url"));
    }

    @Test
    public void testRequestInsert_failed() {
        mSupervisedUserContentProvider.setNativeSupervisedUserContentProviderForTesting(1234L);
        // Mock the native call to mock failure
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object args[] = invocation.getArguments();
                ((SupervisedUserContentProvider.SupervisedUserInsertReply) args[1])
                        .onInsertRequestSendComplete(false);
                return null;
            }

        })
                .when(mSupervisedUserContentProvider)
                .nativeRequestInsert(anyLong(),
                        any(SupervisedUserContentProvider.SupervisedUserInsertReply.class),
                        anyString());
        assertThat(mSupervisedUserContentProvider.requestInsert("url"), is(false));
        verify(mSupervisedUserContentProvider)
                .nativeRequestInsert(eq(1234L),
                        any(SupervisedUserContentProvider.SupervisedUserInsertReply.class),
                        eq("url"));
    }

    @Test
    public void testShouldProceed_withStartupSignedIn() throws ProcessInitException {
        // Set up a signed in user
        ChromeSigninController.get().setSignedInAccountName("Dummy");
        // Mock things called during startup
        ChromeBrowserInitializer mockBrowserInitializer = mock(ChromeBrowserInitializer.class);
        ChromeBrowserInitializer.setForTesting(mockBrowserInitializer);

        WebRestrictionsResult result =
                mSupervisedUserContentProvider.shouldProceed(DEFAULT_CALLING_PACKAGE, "url");

        assertThat(result.shouldProceed(), is(true));
        verify(mockBrowserInitializer).handleSynchronousStartup();
        verify(mSupervisedUserContentProvider)
                .nativeShouldProceed(eq(5678L),
                        any(SupervisedUserContentProvider.SupervisedUserQueryReply.class),
                        eq("url"));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testShouldProceed_notSignedIn()
            throws ProcessInitException, AccountManagerDelegateException {
        // Mock things called during startup
        ChromeBrowserInitializer mockBrowserInitializer = mock(ChromeBrowserInitializer.class);
        ChromeBrowserInitializer.setForTesting(mockBrowserInitializer);
        AccountManagerDelegate mockDelegate = mock(AccountManagerDelegate.class);
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mockDelegate);
        Account account = new Account("Google", "Dummy");
        when(mockDelegate.getAccountsSync()).thenReturn(new Account[] {account});

        WebRestrictionsResult result =
                mSupervisedUserContentProvider.shouldProceed(DEFAULT_CALLING_PACKAGE, "url");

        assertThat(result.shouldProceed(), is(true));
        verify(mockBrowserInitializer).handleSynchronousStartup();
        verify(mSupervisedUserContentProvider)
                .startForcedSigninProcessor(any(Context.class), any(Runnable.class));
        verify(mSupervisedUserContentProvider)
                .listenForChildAccountStatusChange(any(Callback.class));
        verify(mSupervisedUserContentProvider)
                .nativeShouldProceed(eq(5678L),
                        any(SupervisedUserContentProvider.SupervisedUserQueryReply.class),
                        eq("url"));

        AccountManagerFacade.resetAccountManagerFacadeForTests();
    }

    @Test
    public void testShouldProceed_cannotSignIn() throws AccountManagerDelegateException {
        // Mock things called during startup
        ChromeBrowserInitializer mockBrowserInitializer = mock(ChromeBrowserInitializer.class);
        ChromeBrowserInitializer.setForTesting(mockBrowserInitializer);
        AccountManagerDelegate mockDelegate = mock(AccountManagerDelegate.class);
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mockDelegate);
        Account account = new Account("Google", "Dummy");
        when(mockDelegate.getAccountsSync()).thenReturn(new Account[] {account});

        // Change the behavior of the forced sign-in processor to not sign in.
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                invocation.<Runnable>getArgument(1).run();
                return null;
            }
        })
                .when(mSupervisedUserContentProvider)
                .startForcedSigninProcessor(any(Context.class), any(Runnable.class));

        WebRestrictionsResult result =
                mSupervisedUserContentProvider.shouldProceed(DEFAULT_CALLING_PACKAGE, "url");

        assertThat(result.shouldProceed(), is(false));
        assertThat(result.getErrorInt(0), is(5));

        AccountManagerFacade.resetAccountManagerFacadeForTests();
    }

    @Test
    public void testShouldProceed_requestWhitelisted() {
        mSupervisedUserContentProvider.setNativeSupervisedUserContentProviderForTesting(1234L);

        // Modify the result of the native call to block any URL.
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                invocation.<SupervisedUserQueryReply>getArgument(1).onQueryFailed(1, 2, 3, "url1",
                        "url2", "custodian", "custodianEmail", "secondCustodian",
                        "secondCustodianEmail");
                return null;
            }
        })
                .when(mSupervisedUserContentProvider)
                .nativeShouldProceed(eq(1234L),
                        any(SupervisedUserContentProvider.SupervisedUserQueryReply.class),
                        anyString());

        WebRestrictionsResult allowed = mSupervisedUserContentProvider.shouldProceed(
                "com.google.android.gms", "https://accounts.google.com/reauth");
        assertThat(allowed.shouldProceed(), is(true));

        WebRestrictionsResult wrongUrl = mSupervisedUserContentProvider.shouldProceed(
                "com.google.android.gms", "http://www.example.com");
        assertThat(wrongUrl.shouldProceed(), is(false));

        WebRestrictionsResult wrongCallingPackage = mSupervisedUserContentProvider.shouldProceed(
                DEFAULT_CALLING_PACKAGE, "https://accounts.google.com/reauth");
        assertThat(wrongCallingPackage.shouldProceed(), is(false));

        WebRestrictionsResult nullCallingPackage = mSupervisedUserContentProvider.shouldProceed(
                null, "https://accounts.google.com/reauth");
        assertThat(nullCallingPackage.shouldProceed(), is(false));
    }
}
