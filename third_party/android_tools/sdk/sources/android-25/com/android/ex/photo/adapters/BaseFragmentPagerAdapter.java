/*
 * Copyright (C) 2011 Google Inc.
 * Licensed to The Android Open Source Project.
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

package com.android.ex.photo.adapters;

import android.os.Parcelable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.util.LruCache;
import android.support.v4.view.PagerAdapter;
import android.util.Log;
import android.view.View;

/**
 * NOTE: This is a direct copy of {@link android.support.v4.app.FragmentPagerAdapter}
 * with four very important modifications.
 * <p>
 * <ol>
 * <li>The method {@link #makeFragmentName(int, int)} is declared "protected"
 * in our class. We need to be able to re-define the fragment's name according to data
 * only available to sub-classes.</li>
 * <li>The method {@link #isViewFromObject(View, Object)} has been reimplemented to search
 * the entire view hierarchy for the given view.</li>
 * <li>In method {@link #destroyItem(View, int, Object)}, the fragment is detached and
 * added to a cache. If the fragment is evicted from the cache, it will be deleted.
 * An album may contain thousands of photos and we want to avoid having thousands of
 * fragments.</li>
 * </ol>
 */
public abstract class BaseFragmentPagerAdapter extends PagerAdapter {
    /** The default size of {@link #mFragmentCache} */
    private static final int DEFAULT_CACHE_SIZE = 5;
    private static final String TAG = "FragmentPagerAdapter";
    private static final boolean DEBUG = false;

    private final FragmentManager mFragmentManager;
    private FragmentTransaction mCurTransaction = null;
    private Fragment mCurrentPrimaryItem = null;
    /** A cache to store detached fragments before they are removed  */
    private LruCache<String, Fragment> mFragmentCache = new FragmentCache(DEFAULT_CACHE_SIZE);

    public BaseFragmentPagerAdapter(android.support.v4.app.FragmentManager fm) {
        mFragmentManager = fm;
    }

    /**
     * Return the Fragment associated with a specified position.
     */
    public abstract Fragment getItem(int position);

    @Override
    public void startUpdate(View container) {
    }

    @Override
    public Object instantiateItem(View container, int position) {
        if (mCurTransaction == null) {
            mCurTransaction = mFragmentManager.beginTransaction();
        }

        // Do we already have this fragment?
        String name = makeFragmentName(container.getId(), position);

        // Remove item from the cache
        mFragmentCache.remove(name);

        Fragment fragment = mFragmentManager.findFragmentByTag(name);
        if (fragment != null) {
            if (DEBUG) Log.v(TAG, "Attaching item #" + position + ": f=" + fragment);
            mCurTransaction.attach(fragment);
        } else {
            fragment = getItem(position);
            if(fragment == null) {
                if (DEBUG) Log.e(TAG, "NPE workaround for getItem(). See b/7103023");
                return null;
            }
            if (DEBUG) Log.v(TAG, "Adding item #" + position + ": f=" + fragment);
            mCurTransaction.add(container.getId(), fragment,
                    makeFragmentName(container.getId(), position));
        }
        if (fragment != mCurrentPrimaryItem) {
            fragment.setMenuVisibility(false);
        }

        return fragment;
    }

    @Override
    public void destroyItem(View container, int position, Object object) {
        if (mCurTransaction == null) {
            mCurTransaction = mFragmentManager.beginTransaction();
        }
        if (DEBUG) Log.v(TAG, "Detaching item #" + position + ": f=" + object
                + " v=" + ((Fragment)object).getView());

        Fragment fragment = (Fragment) object;
        String name = fragment.getTag();
        if (name == null) {
            // We prefer to get the name directly from the fragment, but, if the fragment is
            // detached before the add transaction is committed, this could be 'null'. In
            // that case, generate a name so we can still cache the fragment.
            name = makeFragmentName(container.getId(), position);
        }

        mFragmentCache.put(name, fragment);
        mCurTransaction.detach(fragment);
    }

    @Override
    public void setPrimaryItem(View container, int position, Object object) {
        Fragment fragment = (Fragment) object;
        if (fragment != mCurrentPrimaryItem) {
            if (mCurrentPrimaryItem != null) {
                mCurrentPrimaryItem.setMenuVisibility(false);
            }
            if (fragment != null) {
                fragment.setMenuVisibility(true);
            }
            mCurrentPrimaryItem = fragment;
        }

    }

    @Override
    public void finishUpdate(View container) {
        if (mCurTransaction != null && !mFragmentManager.isDestroyed()) {
            mCurTransaction.commitAllowingStateLoss();
            mCurTransaction = null;
            mFragmentManager.executePendingTransactions();
        }
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
        // Ascend the tree to determine if the view is a child of the fragment
        View root = ((Fragment) object).getView();
        for (Object v = view; v instanceof View; v = ((View) v).getParent()) {
            if (v == root) {
                return true;
            }
        }
        return false;
    }

    @Override
    public Parcelable saveState() {
        return null;
    }

    @Override
    public void restoreState(Parcelable state, ClassLoader loader) {
    }

    /** Creates a name for the fragment */
    protected String makeFragmentName(int viewId, int index) {
        return "android:switcher:" + viewId + ":" + index;
    }

    /**
     * A cache of detached fragments.
     */
    private class FragmentCache extends LruCache<String, Fragment> {
        public FragmentCache(int size) {
            super(size);
        }

        @Override
        protected void entryRemoved(boolean evicted, String key,
                Fragment oldValue, Fragment newValue) {
            // remove the fragment if it's evicted OR it's replaced by a new fragment
            if (evicted || (newValue != null && oldValue != newValue)) {
                mCurTransaction.remove(oldValue);
            }
        }
    }
}
