// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.webcontents;

import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;

/**
 * Holds an object to be stored in {@code userDataMap} in {@link WebContents} for those
 * classes that have the lifetime of {@link WebContents} without hanging directly onto it.
 * To create an object of a class {@code MyClass}, define a static method
 * {@code fromWebContents()} where you call:
 * <code>
 * WebContentsUserData.fromWebContents(webContents, MyClass.class, MyClass::new);
 * </code>
 *
 * {@code MyClass} should have a contstructor that accepts only one parameter:
 * <code>
 * public MyClass(WebContents webContents);
 * </code>
 */
public final class WebContentsUserData {
    private final Object mObject;

    WebContentsUserData(Object object) {
        mObject = object;
    }

    Object getObject() {
        return mObject;
    }

    /**
     * Looks up the generic object of the given web contents.
     *
     * @param webContents The web contents for which to lookup the object.
     * @param key Class instance of the object used as the key.
     * @param userDataFactory Factory that creates an object of the generic class. Creates a new
     *        instance and returns it if not created yet.
     * @return The object of the given web contents. Can be null if the object was not set
     *         or the user data map is already garbage-collected.
     */
    public static <T> T fromWebContents(
            WebContents webContents, Class<T> key, UserDataFactory<T> userDataFactory) {
        return ((WebContentsImpl) webContents).getOrSetUserData(key, userDataFactory);
    }
}
