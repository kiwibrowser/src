// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.util.Pair;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.webcontents.WebContentsUserData;
import org.chromium.content_public.browser.JavascriptInjector;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;

import java.lang.annotation.Annotation;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * Implementation class of the interface {@link JavascriptInjector}.
 */
@JNINamespace("content")
public class JavascriptInjectorImpl implements JavascriptInjector {
    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<JavascriptInjectorImpl> INSTANCE =
                JavascriptInjectorImpl::new;
    }

    private final Set<Object> mRetainedObjects = new HashSet<>();
    private final Map<String, Pair<Object, Class>> mInjectedObjects = new HashMap<>();
    private long mNativePtr;

    /**
     * @param webContents {@link WebContents} object.
     * @return {@link JavascriptInjector} object used for the give WebContents.
     *         Creates one if not present.
     */
    public static JavascriptInjector fromWebContents(WebContents webContents) {
        return WebContentsUserData.fromWebContents(
                webContents, JavascriptInjectorImpl.class, UserDataFactoryLazyHolder.INSTANCE);
    }

    public JavascriptInjectorImpl(WebContents webContents) {
        mNativePtr = nativeInit(webContents, mRetainedObjects);
    }

    @CalledByNative
    private void onDestroy() {
        mNativePtr = 0;
    }

    @Override
    public Map<String, Pair<Object, Class>> getInterfaces() {
        return mInjectedObjects;
    }

    @Override
    public void setAllowInspection(boolean allow) {
        if (mNativePtr != 0) nativeSetAllowInspection(mNativePtr, allow);
    }

    @Override
    public void addPossiblyUnsafeInterface(
            Object object, String name, Class<? extends Annotation> requiredAnnotation) {
        if (mNativePtr != 0 && object != null) {
            mInjectedObjects.put(name, new Pair<Object, Class>(object, requiredAnnotation));
            nativeAddInterface(mNativePtr, object, name, requiredAnnotation);
        }
    }

    @Override
    public void removeInterface(String name) {
        mInjectedObjects.remove(name);
        if (mNativePtr != 0) nativeRemoveInterface(mNativePtr, name);
    }

    private native long nativeInit(WebContents webContents, Object retainedObjects);
    private native void nativeSetAllowInspection(long nativeJavascriptInjector, boolean allow);
    private native void nativeAddInterface(
            long nativeJavascriptInjector, Object object, String name, Class requiredAnnotation);
    private native void nativeRemoveInterface(long nativeJavascriptInjector, String name);
}
