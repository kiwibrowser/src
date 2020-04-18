// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

/**
 * This interface is implemented by FRE fragments.
 */
public interface FirstRunFragment {
    /**
     * Notifies this fragment that native has been initialized.
     */
    default void onNativeInitialized() {}

    /**
     * @return Whether the back button press was handled by this page.
     */
    default boolean interceptBackPressed() {
        return false;
    }

    /**
     * @see Fragment#getActivity().
     */
    FragmentActivity getActivity();

    /**
     * Convenience method to get {@link FirstRunPageDelegate}.
     */
    default FirstRunPageDelegate getPageDelegate() {
        return (FirstRunPageDelegate) getActivity();
    }
}
