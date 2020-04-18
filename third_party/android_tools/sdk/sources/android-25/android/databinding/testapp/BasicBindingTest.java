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

import android.databinding.DataBindingUtil;
import android.databinding.OnRebindCallback;
import android.databinding.testapp.databinding.BasicBindingBinding;
import android.databinding.testapp.databinding.NoExpressionsBinding;
import android.databinding.testapp.databinding.NoDataElementBinding;
import android.databinding.testapp.databinding.JustIdBinding;
import android.test.UiThreadTest;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class BasicBindingTest extends BaseDataBinderTest<BasicBindingBinding> {
    public BasicBindingTest() {
        super(BasicBindingBinding.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initBinder();
    }

    @UiThreadTest
    public void testTextViewContentInInitialization() {
        assertAB("X", "Y");
    }

    @UiThreadTest
    public void testNullValuesInInitialization() {
        assertAB(null, null);
    }

    @UiThreadTest
    public void testSecondIsNullInInitialization() {
        assertAB(null, "y");
    }

    @UiThreadTest
    public void testFirstIsNullInInitialization() {
        assertAB("x", null);
    }

    @UiThreadTest
    public void testTextViewContent() {
        assertAB("X", "Y");
    }

    @UiThreadTest
    public void testNullValues() {
        assertAB(null, null);
    }

    @UiThreadTest
    public void testSecondIsNull() {
        assertAB(null, "y");
    }

    @UiThreadTest
    public void testFirstIsNull() {
        assertAB("x", null);
    }

    public void testStopBinding() throws Throwable {
        final NoRebind noRebind = new NoRebind();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertAB("X", "Y");
                mBinder.addOnRebindCallback(noRebind);
            }
        });
        mBinder.setA("Q");
        WaitForRun waitForRun = new WaitForRun();
        View root = mBinder.getRoot();
        root.postOnAnimation(waitForRun);
        waitForRun.waitForRun();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertEquals(1, noRebind.rebindAttempts);
                assertEquals(1, noRebind.rebindHalted);
                assertEquals(0, noRebind.rebindWillEvaluate);
                assertEquals("XY", mBinder.textView.getText().toString());
            }
        });
        mBinder.removeOnRebindCallback(noRebind);
        final AllowRebind allowRebind = new AllowRebind();
        mBinder.addOnRebindCallback(allowRebind);
        mBinder.setB("R");
        root.postOnAnimation(waitForRun);
        waitForRun.waitForRun();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertEquals(1, noRebind.rebindAttempts);
                assertEquals(1, noRebind.rebindHalted);
                assertEquals(0, noRebind.rebindWillEvaluate);
                assertEquals(1, allowRebind.rebindAttempts);
                assertEquals(0, allowRebind.rebindHalted);
                assertEquals(1, allowRebind.rebindWillEvaluate);
                assertEquals("QR", mBinder.textView.getText().toString());
            }
        });
    }

    @UiThreadTest
    public void testNoExpressionBinding() throws Throwable {
        NoExpressionsBinding binding = NoExpressionsBinding.inflate(getActivity().getLayoutInflater());
        assertNotNull(binding);
    }

    @UiThreadTest
    public void testNoDataElement() throws Throwable {
        NoDataElementBinding binding = NoDataElementBinding.inflate(getActivity().getLayoutInflater());
        assertNotNull(binding);
    }

    @UiThreadTest
    public void testJustIds() throws Throwable {
        JustIdBinding binding = JustIdBinding.inflate(getActivity().getLayoutInflater());
        assertNotNull(binding);
        assertNotNull(binding.textView);
    }

    @UiThreadTest
    public void testNoBinding() throws Throwable {
        assertNull(DataBindingUtil.inflate(getActivity().getLayoutInflater(), R.layout.plain_layout,
                null, false));
    }

    @UiThreadTest
    public void testInflation() throws Throwable {
        ViewGroup viewGroup = (ViewGroup) mBinder.getRoot();
        BasicBindingBinding binding =
                BasicBindingBinding.inflate(getActivity().getLayoutInflater(), viewGroup, true);
        assertNotNull(binding);
        assertNotNull(binding.textView);
        assertNotSame(binding.textView, mBinder.textView);
    }

    @UiThreadTest
    public void testAndroidId() throws Throwable {
        JustIdBinding binding = JustIdBinding.inflate(getActivity().getLayoutInflater());
        assertNotNull(binding);
        assertNotNull(binding.empty);
        assertTrue(binding.empty instanceof TextView);
    }

    private void assertAB(String a, String b) {
        mBinder.setA(a);
        mBinder.setB(b);
        rebindAndAssert(a + b);
    }

    private void rebindAndAssert(String text) {
        mBinder.executePendingBindings();
        assertEquals(text, mBinder.textView.getText().toString());
    }

    private class AllowRebind extends OnRebindCallback<BasicBindingBinding> {
        public int rebindAttempts;
        public int rebindHalted;
        public int rebindWillEvaluate;

        @Override
        public boolean onPreBind(BasicBindingBinding binding) {
            rebindAttempts++;
            return true;
        }

        @Override
        public void onCanceled(BasicBindingBinding binding) {
            rebindHalted++;
        }

        @Override
        public void onBound(BasicBindingBinding binding) {
            rebindWillEvaluate++;
        }
    }

    private class NoRebind extends AllowRebind {
        @Override
        public boolean onPreBind(BasicBindingBinding binding) {
            super.onPreBind(binding);
            return false;
        }
    }

    private static class WaitForRun implements Runnable {

        @Override
        public void run() {
            synchronized (this) {
                this.notifyAll();
            }
        }

        public void waitForRun() {
            synchronized (this) {
                try {
                    this.wait(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
