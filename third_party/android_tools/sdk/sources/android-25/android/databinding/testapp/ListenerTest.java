/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.testapp;

import android.databinding.ViewStubProxy;
import android.databinding.testapp.databinding.ListenersBinding;
import android.databinding.testapp.vo.ListenerBindingObject;
import android.databinding.testapp.vo.ListenerBindingObject.Inner;
import android.test.UiThreadTest;
import android.view.View;

public class ListenerTest extends BaseDataBinderTest<ListenersBinding> {
    private ListenerBindingObject mBindingObject;

    public ListenerTest() {
        super(ListenersBinding.class);
    }

    @Override
    protected void setUp() throws Exception {
        mBindingObject = new ListenerBindingObject(getActivity());
        super.setUp();
        initBinder(new Runnable() {
            @Override
            public void run() {
                mBinder.setObj(mBindingObject);
            }
        });
        ListenerBindingObject.lastClick = 0;
    }

    @UiThreadTest
    public void testInstanceClick() throws Throwable {
        View view = mBinder.click1;
        assertEquals(0, ListenerBindingObject.lastClick);
        view.callOnClick();
        assertEquals(1, ListenerBindingObject.lastClick);
    }

    @UiThreadTest
    public void testStaticClick() throws Throwable {
        View view = mBinder.click2;
        assertEquals(0, ListenerBindingObject.lastClick);
        view.callOnClick();
        assertEquals(2, ListenerBindingObject.lastClick);
    }

    @UiThreadTest
    public void testInstanceClickTwoArgs() throws Throwable {
        View view = mBinder.click3;
        assertEquals(0, ListenerBindingObject.lastClick);
        view.callOnClick();
        assertEquals(3, ListenerBindingObject.lastClick);
        assertTrue(view.isClickable());
        ListenerBindingObject.lastClick = 0;
        mBindingObject.clickable.set(false);
        mBinder.executePendingBindings();
        assertFalse(view.isClickable());
        mBindingObject.useOne.set(true);
        mBinder.executePendingBindings();
        assertFalse(view.isClickable());
        mBindingObject.clickable.set(true);
        mBinder.executePendingBindings();
        view.callOnClick();
        assertEquals(1, ListenerBindingObject.lastClick);
    }

    @UiThreadTest
    public void testStaticClickTwoArgs() throws Throwable {
        View view = mBinder.click4;
        assertEquals(0, ListenerBindingObject.lastClick);
        view.callOnClick();
        assertEquals(4, ListenerBindingObject.lastClick);
        assertTrue(view.isClickable());
        ListenerBindingObject.lastClick = 0;
        mBindingObject.clickable.set(false);
        mBinder.executePendingBindings();
        assertFalse(view.isClickable());
        view.setClickable(true);
        view.callOnClick();
        assertEquals(4, ListenerBindingObject.lastClick);
    }

    @UiThreadTest
    public void testClickExpression() throws Throwable {
        View view = mBinder.click5;
        assertEquals(0, ListenerBindingObject.lastClick);
        view.callOnClick();
        assertEquals(2, ListenerBindingObject.lastClick);
        ListenerBindingObject.lastClick = 0;
        mBindingObject.useOne.set(true);
        mBinder.executePendingBindings();
        view.callOnClick();
        assertEquals(1, ListenerBindingObject.lastClick);
    }

    @UiThreadTest
    public void testInflateListener() throws Throwable {
        ViewStubProxy viewStubProxy = mBinder.viewStub;
        assertFalse(viewStubProxy.isInflated());
        assertFalse(mBindingObject.inflateCalled);
        viewStubProxy.getViewStub().inflate();
        assertTrue(mBindingObject.inflateCalled);
        assertTrue(viewStubProxy.isInflated());
    }

    @UiThreadTest
    public void testBaseObservableClick() throws Throwable {
        View view = mBinder.click6;
        Inner inner = new Inner();
        mBinder.setObj2(inner);
        mBinder.executePendingBindings();
        assertFalse(inner.clicked);
        view.callOnClick();
        assertTrue(inner.clicked);
    }
}
