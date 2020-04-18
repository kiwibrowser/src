package android.databinding.testapp;

import android.databinding.testapp.databinding.CustomNsAdapterBinding;
import android.test.UiThreadTest;

public class CustomNamespaceAdapterTest extends BaseDataBinderTest<CustomNsAdapterBinding>{
    public CustomNamespaceAdapterTest() {
        super(CustomNsAdapterBinding.class);
    }

    @UiThreadTest
    public void testAndroidNs() {
        initBinder();
        mBinder.setStr1("a");
        mBinder.setStr2("b");
        mBinder.executePendingBindings();
        assertEquals("a", mBinder.textView1.getText().toString());
    }

    @UiThreadTest
    public void testCustomNs() {
        initBinder();
        mBinder.setStr1("a");
        mBinder.setStr2("b");
        mBinder.executePendingBindings();
        assertEquals("b", mBinder.textView2.getText().toString());
    }
}
