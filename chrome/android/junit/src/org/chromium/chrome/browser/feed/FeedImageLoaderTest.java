// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feed;

import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.support.test.filters.SmallTest;

import com.google.android.libraries.feed.common.functional.Consumer;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.AdditionalMatchers;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link FeedImageLoader}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class FeedImageLoaderTest {
    public static final String HTTP_STRING1 = "http://www.test1.com";
    public static final String HTTP_STRING2 = "http://www.test2.com";
    public static final String HTTP_STRING3 = "http://www.test3.com";
    public static final String ASSET_STRING = "asset://logo_avatar_anonymous";

    @Mock
    private FeedImageLoaderBridge mBridge;
    @Mock
    private Consumer<Drawable> mConsumer;
    @Mock
    private Profile mProfile;
    @Captor
    ArgumentCaptor<List<String>> mUrlListArgument;
    @Captor
    ArgumentCaptor<Callback<Bitmap>> mCallbackArgument;

    private FeedImageLoader mImageLoader;

    private class FakeConsumer implements Consumer<Drawable> {
        public Drawable mResponse = null;

        @Override
        public void accept(Drawable value) {
            mResponse = value;
        }

        public void clearResponse() {
            mResponse = null;
        }
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        doNothing().when(mBridge).init(eq(mProfile));
        mImageLoader = new FeedImageLoader(mProfile, ContextUtils.getApplicationContext(), mBridge);
        verify(mBridge, times(1)).init(eq(mProfile));
        answerFetchImage(null);
    }

    private void answerFetchImage(Bitmap bitmap) {
        Answer<Void> answer = new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                mCallbackArgument.getValue().onResult(bitmap);

                return null;
            }
        };
        doAnswer(answer).when(mBridge).fetchImage(
                mUrlListArgument.capture(), mCallbackArgument.capture());
    }

    @Test
    @SmallTest
    public void downloadImageTest() {
        List<String> urls = Arrays.asList(HTTP_STRING1);
        mImageLoader.loadDrawable(urls, mConsumer);

        verify(mBridge, times(1))
                .fetchImage(mUrlListArgument.capture(), mCallbackArgument.capture());
    }

    @Test
    @SmallTest
    public void onlyNetworkURLSendToBridgeTest() {
        List<String> urls = Arrays.asList(HTTP_STRING1, HTTP_STRING2, ASSET_STRING, HTTP_STRING3);
        mImageLoader.loadDrawable(urls, mConsumer);
        List<String> expected_urls = Arrays.asList(HTTP_STRING1, HTTP_STRING2, HTTP_STRING3);

        verify(mBridge, times(1)).fetchImage(eq(expected_urls), mCallbackArgument.capture());
    }

    @Test
    @SmallTest
    public void assetImageTest() {
        List<String> urls = Arrays.asList(ASSET_STRING);
        mImageLoader.loadDrawable(urls, mConsumer);

        verify(mConsumer, times(1)).accept(AdditionalMatchers.not(eq(null)));
    }

    @Test
    @SmallTest
    public void sendNullIfDownloadFailTest() {
        List<String> urls = Arrays.asList(HTTP_STRING1, HTTP_STRING2, HTTP_STRING3);
        mImageLoader.loadDrawable(urls, mConsumer);

        verify(mConsumer, times(1)).accept(eq(null));
    }

    @Test
    @SmallTest
    public void nullUrlListTest() {
        List<String> urls = Arrays.asList();
        mImageLoader.loadDrawable(urls, mConsumer);

        verify(mConsumer, times(1)).accept(eq(null));
    }
}
